#!/usr/bin/env python3.8
import json
import os
import sqlite3
import threading
import time
import uuid
import hashlib
from datetime import datetime
from urllib import error, request


def _env_bool(name, default=False):
    value = os.environ.get(name)
    if value is None:
        return default
    return value.strip().lower() in ("1", "true", "yes", "on")


class CloudSyncService(object):
    """云端事件上报服务（SQLite outbox + 异步重试，不阻塞主流程）。"""

    def __init__(self):
        self.enabled = _env_bool("ENABLE_CLOUD_SYNC", False)
        self.base_url = os.environ.get("CLOUD_API_BASE", "").rstrip("/")
        self.api_token = os.environ.get("CLOUD_API_TOKEN", "")
        self.device_id = os.environ.get("CLOUD_DEVICE_ID", "imx6ull-node-01")
        self.timeout = float(os.environ.get("CLOUD_HTTP_TIMEOUT", "3.0"))
        self.max_retry = int(os.environ.get("CLOUD_MAX_RETRY", "3"))
        self.poll_interval = float(os.environ.get("CLOUD_POLL_INTERVAL", "1.0"))
        self.enable_business_sync = _env_bool("ENABLE_BUSINESS_DB_SYNC", False)
        self.business_db_path = os.environ.get(
            "BUSINESS_DB_PATH", "/run/media/mmcblk1p1/parking.db"
        )
        self.business_sync_interval = float(
            os.environ.get("BUSINESS_SYNC_INTERVAL", "10.0")
        )
        default_db = os.path.join(os.path.dirname(os.path.abspath(__file__)), "cloud_sync.db")
        self.db_path = os.environ.get("CLOUD_SYNC_DB", default_db)
        self.endpoint = "/api/events"

        self._db_lock = threading.Lock()
        self._init_db()
        self._worker = None
        self._business_worker = None

        if self.enabled and self.base_url:
            self._worker = threading.Thread(target=self._run, daemon=True)
            self._worker.start()
            print(
                "CloudSync: enabled, target={}, db={}".format(
                    self.base_url, self.db_path
                )
            )
            if self.enable_business_sync:
                self._business_worker = threading.Thread(
                    target=self._run_business_sync, daemon=True
                )
                self._business_worker.start()
                print(
                    "CloudSync: business sync enabled, source={}, interval={}s".format(
                        self.business_db_path, self.business_sync_interval
                    )
                )
        else:
            print("CloudSync: disabled (set ENABLE_CLOUD_SYNC=1 and CLOUD_API_BASE)")

    def report_plate_result(self, plate_number, confidence, plate_type=None, source="unknown"):
        payload = {
            "plate_number": plate_number,
            "confidence": float(confidence),
            "plate_type": plate_type,
            "source": source,
        }
        return self._enqueue("plate_result", payload)

    def report_plate_failure(self, reason, source="unknown"):
        payload = {
            "reason": str(reason),
            "source": source,
        }
        return self._enqueue("plate_failure", payload)

    def report_tts_event(self, event_type, text, file_name, status, detail=""):
        payload = {
            "event_type": event_type or "",
            "text": text or "",
            "file_name": file_name or "",
            "status": status,
            "detail": detail,
        }
        return self._enqueue("tts_event", payload)

    def _enqueue(self, event_type, payload, event_id=None):
        event = {
            "event_id": event_id or str(uuid.uuid4()),
            "event_type": event_type,
            "device_id": self.device_id,
            "timestamp": datetime.utcnow().isoformat() + "Z",
            "payload": payload,
        }
        self._insert_outbox_event(event)
        return True

    def _run(self):
        while True:
            item = self._fetch_next_pending()
            if not item:
                time.sleep(self.poll_interval)
                continue

            row_id, event_json, retry_count = item
            try:
                event = json.loads(event_json)
                self._send_with_retry(event)
            finally:
                # _send_with_retry 内会负责更新状态，这里不再处理。
                pass

    def _send_with_retry(self, event):
        event_id = event.get("event_id", "")
        outbox_row = self._find_outbox_by_event_id(event_id)
        if not outbox_row:
            return

        row_id, retry_count = outbox_row
        while True:
            try:
                self._post_event(event)
                self._mark_success(row_id)
                return
            except Exception as exc:
                retry_count += 1
                if retry_count > self.max_retry:
                    self._mark_dead(row_id, str(exc))
                    print(
                        "CloudSync: drop after retry, type={}, err={}".format(
                            event.get("event_type"), exc
                        )
                    )
                    return
                self._mark_retry(row_id, retry_count, str(exc))
                time.sleep(min(2 ** retry_count, 8))

    def _post_event(self, event):
        url = self.base_url + self.endpoint
        body = json.dumps(event, ensure_ascii=False).encode("utf-8")
        headers = {"Content-Type": "application/json"}
        if self.api_token:
            headers["Authorization"] = "Bearer {}".format(self.api_token)

        req = request.Request(url=url, data=body, headers=headers, method="POST")
        try:
            with request.urlopen(req, timeout=self.timeout) as resp:
                if resp.status < 200 or resp.status >= 300:
                    raise RuntimeError("http status {}".format(resp.status))
        except error.URLError as exc:
            raise RuntimeError("network error: {}".format(exc))

    def _connect(self):
        conn = sqlite3.connect(self.db_path)
        conn.row_factory = sqlite3.Row
        return conn

    def _init_db(self):
        with self._db_lock:
            conn = self._connect()
            try:
                conn.execute(
                    """
                    CREATE TABLE IF NOT EXISTS sync_outbox (
                        id INTEGER PRIMARY KEY AUTOINCREMENT,
                        event_id TEXT NOT NULL UNIQUE,
                        event_type TEXT NOT NULL,
                        payload_json TEXT NOT NULL,
                        status TEXT NOT NULL DEFAULT 'pending',
                        retry_count INTEGER NOT NULL DEFAULT 0,
                        last_error TEXT NOT NULL DEFAULT '',
                        created_at TEXT NOT NULL,
                        updated_at TEXT NOT NULL
                    )
                    """
                )
                conn.execute(
                    "CREATE INDEX IF NOT EXISTS idx_sync_outbox_status ON sync_outbox(status, id)"
                )
                conn.execute(
                    """
                    CREATE TABLE IF NOT EXISTS sync_row_state (
                        row_key TEXT PRIMARY KEY,
                        fingerprint TEXT NOT NULL,
                        updated_at TEXT NOT NULL
                    )
                    """
                )
                conn.commit()
            finally:
                conn.close()

    def _insert_outbox_event(self, event):
        now = datetime.utcnow().isoformat() + "Z"
        payload_json = json.dumps(event, ensure_ascii=False)
        with self._db_lock:
            conn = self._connect()
            try:
                conn.execute(
                    """
                    INSERT INTO sync_outbox (
                        event_id, event_type, payload_json, status,
                        retry_count, last_error, created_at, updated_at
                    )
                    VALUES (?, ?, ?, 'pending', 0, '', ?, ?)
                    """,
                    (event["event_id"], event["event_type"], payload_json, now, now),
                )
                conn.commit()
            except sqlite3.IntegrityError:
                # 同一个事件已存在（常见于幂等重试或业务库重复扫描）
                pass
            finally:
                conn.close()

    def _fetch_next_pending(self):
        with self._db_lock:
            conn = self._connect()
            try:
                row = conn.execute(
                    """
                    SELECT id, payload_json, retry_count
                    FROM sync_outbox
                    WHERE status = 'pending'
                    ORDER BY id ASC
                    LIMIT 1
                    """
                ).fetchone()
                if not row:
                    return None
                return row["id"], row["payload_json"], row["retry_count"]
            finally:
                conn.close()

    def _find_outbox_by_event_id(self, event_id):
        if not event_id:
            return None
        with self._db_lock:
            conn = self._connect()
            try:
                row = conn.execute(
                    """
                    SELECT id, retry_count
                    FROM sync_outbox
                    WHERE event_id = ? AND status = 'pending'
                    LIMIT 1
                    """,
                    (event_id,),
                ).fetchone()
                if not row:
                    return None
                return row["id"], row["retry_count"]
            finally:
                conn.close()

    def _mark_success(self, row_id):
        now = datetime.utcnow().isoformat() + "Z"
        with self._db_lock:
            conn = self._connect()
            try:
                conn.execute(
                    """
                    UPDATE sync_outbox
                    SET status = 'success', updated_at = ?
                    WHERE id = ?
                    """,
                    (now, row_id),
                )
                conn.commit()
            finally:
                conn.close()

    def _mark_retry(self, row_id, retry_count, last_error):
        now = datetime.utcnow().isoformat() + "Z"
        with self._db_lock:
            conn = self._connect()
            try:
                conn.execute(
                    """
                    UPDATE sync_outbox
                    SET retry_count = ?, last_error = ?, updated_at = ?
                    WHERE id = ?
                    """,
                    (retry_count, last_error[:512], now, row_id),
                )
                conn.commit()
            finally:
                conn.close()

    def _mark_dead(self, row_id, last_error):
        now = datetime.utcnow().isoformat() + "Z"
        with self._db_lock:
            conn = self._connect()
            try:
                conn.execute(
                    """
                    UPDATE sync_outbox
                    SET status = 'dead', last_error = ?, updated_at = ?
                    WHERE id = ?
                    """,
                    (last_error[:512], now, row_id),
                )
                conn.commit()
            finally:
                conn.close()

    def _run_business_sync(self):
        while True:
            try:
                self._sync_business_once()
            except Exception as exc:
                print("CloudSync: business sync error={}".format(exc))
            time.sleep(max(self.business_sync_interval, 1.0))

    def _table_columns(self, conn, table_name):
        rows = conn.execute("PRAGMA table_info({})".format(table_name)).fetchall()
        return {row[1] for row in rows}

    def _select_existing_columns(self, conn, table_name, columns):
        existing = self._table_columns(conn, table_name)
        selected = [name for name in columns if name in existing]
        if not selected:
            return []
        sql = "SELECT {} FROM {} ORDER BY id ASC".format(
            ", ".join(selected), table_name
        )
        return conn.execute(sql).fetchall(), selected

    def _sync_business_once(self):
        if not os.path.exists(self.business_db_path):
            return

        conn = sqlite3.connect(self.business_db_path)
        conn.row_factory = sqlite3.Row
        try:
            self._sync_vehicle_rows(conn)
            self._sync_history_rows(conn)
            self._sync_rfid_rows(conn)
            self._sync_parking_config(conn)
            self._sync_blacklist_rows(conn)
        finally:
            conn.close()

    def _sync_vehicle_rows(self, conn):
        columns = [
            "id",
            "plate_number",
            "rfid_card",
            "entry_time",
            "exit_time",
            "status",
            "total_fee",
            "entry_image",
            "exit_image",
        ]
        rows, selected = self._select_existing_columns(conn, "vehicle", columns)
        for row in rows:
            payload = {name: row[name] for name in selected}
            self._sync_row_event("vehicle", row["id"], "business_vehicle_upsert", payload)

    def _sync_history_rows(self, conn):
        columns = [
            "id",
            "plate_number",
            "rfid_card",
            "entry_time",
            "exit_time",
            "duration",
            "fee",
            "entry_image",
            "exit_image",
        ]
        rows, selected = self._select_existing_columns(conn, "history", columns)
        for row in rows:
            payload = {name: row[name] for name in selected}
            self._sync_row_event("history", row["id"], "business_history_upsert", payload)

    def _sync_rfid_rows(self, conn):
        rows = conn.execute(
            """
            SELECT id, rfid_card, balance, created_time
            FROM rfid_account
            ORDER BY id ASC
            """
        ).fetchall()
        for row in rows:
            payload = dict(row)
            self._sync_row_event(
                "rfid_account", row["id"], "business_rfid_account_upsert", payload
            )

    def _sync_parking_config(self, conn):
        rows = conn.execute(
            """
            SELECT id, total_spaces, hourly_rate
            FROM parking_config
            ORDER BY id ASC
            """
        ).fetchall()
        for row in rows:
            payload = dict(row)
            self._sync_row_event(
                "parking_config", row["id"], "business_parking_config_upsert", payload
            )

    def _sync_blacklist_rows(self, conn):
        rows = conn.execute(
            """
            SELECT id, plate_number, reason, added_time
            FROM blacklist
            ORDER BY id ASC
            """
        ).fetchall()
        for row in rows:
            payload = dict(row)
            self._sync_row_event("blacklist", row["id"], "business_blacklist_upsert", payload)

    def _sync_row_event(self, table_name, row_id, event_type, payload):
        row_key = "{}:{}".format(table_name, row_id)
        fingerprint = self._fingerprint_payload(payload)
        old_fingerprint = self._get_row_fingerprint(row_key)
        if old_fingerprint == fingerprint:
            return

        event_id = "business-{}-{}".format(
            row_key, hashlib.sha1(fingerprint.encode("utf-8")).hexdigest()[:16]
        )
        ok = self._enqueue(event_type, payload, event_id=event_id)
        if ok:
            self._set_row_fingerprint(row_key, fingerprint)

    def _fingerprint_payload(self, payload):
        normalized = json.dumps(payload, ensure_ascii=False, sort_keys=True, default=str)
        return hashlib.sha1(normalized.encode("utf-8")).hexdigest()

    def _get_row_fingerprint(self, row_key):
        with self._db_lock:
            conn = self._connect()
            try:
                row = conn.execute(
                    "SELECT fingerprint FROM sync_row_state WHERE row_key = ? LIMIT 1",
                    (row_key,),
                ).fetchone()
                if not row:
                    return None
                return row["fingerprint"]
            finally:
                conn.close()

    def _set_row_fingerprint(self, row_key, fingerprint):
        now = datetime.utcnow().isoformat() + "Z"
        with self._db_lock:
            conn = self._connect()
            try:
                updated = conn.execute(
                    """
                    UPDATE sync_row_state
                    SET fingerprint = ?, updated_at = ?
                    WHERE row_key = ?
                    """,
                    (fingerprint, now, row_key),
                ).rowcount
                if updated == 0:
                    conn.execute(
                        """
                        INSERT INTO sync_row_state(row_key, fingerprint, updated_at)
                        VALUES (?, ?, ?)
                        """,
                        (row_key, fingerprint, now),
                    )
                conn.commit()
            finally:
                conn.close()
