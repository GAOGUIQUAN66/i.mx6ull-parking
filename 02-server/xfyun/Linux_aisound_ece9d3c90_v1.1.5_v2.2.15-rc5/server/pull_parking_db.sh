#!/usr/bin/env bash
set -euo pipefail

# 从板子拉取业务主库到本机
# 用法:
#   ./pull_parking_db.sh
#   BOARD_HOST=192.168.137.100 BOARD_USER=root ./pull_parking_db.sh
#   LOCAL_DB_PATH="$HOME/linux/IMX6ULL/i.mx6ull-parking/data/parking.db" ./pull_parking_db.sh

BOARD_HOST="${BOARD_HOST:-192.168.137.100}"
BOARD_USER="${BOARD_USER:-root}"
REMOTE_DB_PATH="${REMOTE_DB_PATH:-/run/media/mmcblk1p1/parking.db}"
LOCAL_DB_PATH="${LOCAL_DB_PATH:-$HOME/linux/IMX6ULL/i.mx6ull-parking/data/parking.db}"

LOCAL_DIR="$(dirname "$LOCAL_DB_PATH")"
TMP_PATH="${LOCAL_DB_PATH}.tmp"

echo "[INFO] Board: ${BOARD_USER}@${BOARD_HOST}"
echo "[INFO] Remote DB: ${REMOTE_DB_PATH}"
echo "[INFO] Local DB : ${LOCAL_DB_PATH}"

mkdir -p "$LOCAL_DIR"

echo "[STEP] Copy database from board ..."
scp "${BOARD_USER}@${BOARD_HOST}:${REMOTE_DB_PATH}" "$TMP_PATH"
mv -f "$TMP_PATH" "$LOCAL_DB_PATH"

echo "[STEP] Verify database ..."
if command -v sqlite3 >/dev/null 2>&1; then
    sqlite3 "$LOCAL_DB_PATH" ".tables" | tr '\n' ' '
    echo
else
    echo "[WARN] sqlite3 not found, skip table check."
fi

echo "[DONE] Database synced to local path."
echo "Next: export BUSINESS_DB_PATH=\"$LOCAL_DB_PATH\""
