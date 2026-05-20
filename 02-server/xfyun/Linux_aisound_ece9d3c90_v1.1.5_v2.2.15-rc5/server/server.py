#!/usr/bin/env python3.8
import json
import os
import socket
import struct
import subprocess
import tempfile
import threading
from datetime import datetime
from functools import lru_cache

import cv2
import numpy as np

from cloud_service import CloudSyncService
from lpr_service import PlateRecognitionService


HOST = "192.168.137.50"
PORT = 8888

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR = os.path.dirname(BASE_DIR)
AIKIT_BIN = os.environ.get("AIKIT_BIN", os.path.join(ROOT_DIR, "aikit_test"))
AIKIT_LIBS_DIR = os.environ.get("AIKIT_LIBS_DIR", os.path.join(ROOT_DIR, "libs"))

PACKET_HEADER_SIZE = 5
PACKET_TYPE_IMAGE = 1
PACKET_TYPE_TEXT = 2
PACKET_TYPE_RESULT = 3
PACKET_TYPE_RAW_IMAGE = 4
PACKET_TYPE_AUDIO = 5

RECOGNIZER = PlateRecognitionService()
CLOUD_SYNC = CloudSyncService()
PULL_DB_SCRIPT = os.environ.get(
    "PULL_DB_SCRIPT", os.path.join(BASE_DIR, "pull_parking_db.sh")
)


def recv_exact(conn, size):
    chunks = []
    remaining = size
    while remaining > 0:
        data = conn.recv(remaining)
        if not data:
            return b""
        chunks.append(data)
        remaining -= len(data)
    return b"".join(chunks)


def send_result(conn, success, error="", plate_number="", confidence=0.0):
    body = json.dumps(
        {
            "success": success,
            "plate_number": plate_number,
            "confidence": confidence,
            "error": error,
        },
        ensure_ascii=False,
    ).encode("utf-8")
    packet = struct.pack(">IB", len(body), PACKET_TYPE_RESULT) + body
    conn.sendall(packet)


def send_audio(conn, file_name, audio_data, event_type="", text=""):
    header = json.dumps(
        {
            "file_name": file_name,
            "event": event_type,
            "text": text,
            "mime": "audio/wav",
        },
        ensure_ascii=False,
    ).encode("utf-8")
    payload = struct.pack(">I", len(header)) + header + audio_data
    packet = struct.pack(">IB", len(payload), PACKET_TYPE_AUDIO) + payload
    conn.sendall(packet)


def log(message):
    print("[{}] {}".format(datetime.now().strftime("%Y-%m-%d %H:%M:%S"), message))


def _env_bool(name, default=False):
    value = os.environ.get(name)
    if value is None:
        return default
    return value.strip().lower() in ("1", "true", "yes", "on")


def maybe_pull_business_db():
    """
    可选：启动 server.py 时自动拉取一次板子业务库。
    默认开启，便于只执行一条 ./server.py 即可完成准备工作。
    """
    if not _env_bool("ENABLE_BUSINESS_DB_SYNC", False):
        return
    if not _env_bool("AUTO_PULL_DB_ON_START", True):
        return

    script_path = PULL_DB_SCRIPT
    if not os.path.exists(script_path):
        log("业务库拉取脚本不存在，跳过: {}".format(script_path))
        return

    try:
        log("启动前自动拉取业务库: {}".format(script_path))
        result = subprocess.run(
            ["bash", script_path],
            cwd=BASE_DIR,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=30,
            check=True,
        )
        output = result.stdout.decode("utf-8", errors="replace").strip()
        if output:
            log("业务库拉取完成: {}".format(output.splitlines()[-1]))
    except Exception as exc:
        # 不中断主服务，避免因为拉库失败导致识别服务无法启动
        log("业务库拉取失败，继续启动识别服务: {}".format(exc))


def decode_rgb565_to_bgr(raw_data, width, height):
    expected_size = width * height * 2
    if len(raw_data) != expected_size:
        raise ValueError(
            "RGB565 数据长度不匹配, expect={}, actual={}".format(
                expected_size, len(raw_data)
            )
        )

    pixels = np.frombuffer(raw_data, dtype=np.uint16).reshape((height, width))
    r = ((pixels >> 11) & 0x1F).astype(np.uint8)
    g = ((pixels >> 5) & 0x3F).astype(np.uint8)
    b = (pixels & 0x1F).astype(np.uint8)

    r = (r << 3) | (r >> 2)
    g = (g << 2) | (g >> 4)
    b = (b << 3) | (b >> 2)
    return np.dstack((b, g, r))


def parse_raw_frame_payload(payload):
    if len(payload) < 12:
        raise ValueError("raw payload too small")

    width, height, fmt_len = struct.unpack(">III", payload[:12])
    if len(payload) < 12 + fmt_len:
        raise ValueError("raw payload header incomplete")

    format_bytes = payload[12 : 12 + fmt_len]
    pixel_format = format_bytes.decode("utf-8", errors="replace")
    raw_data = payload[12 + fmt_len :]
    return {
        "width": width,
        "height": height,
        "pixel_format": pixel_format,
        "raw_data": raw_data,
    }


def decode_image_payload(payload):
    data = np.frombuffer(payload, dtype=np.uint8)
    image = cv2.imdecode(data, cv2.IMREAD_COLOR)
    if image is None:
        raise ValueError("图像解码失败")
    return image


def decode_raw_frame_to_bgr(frame_info):
    pixel_format = frame_info["pixel_format"]
    if pixel_format == "RGBP":
        return decode_rgb565_to_bgr(
            frame_info["raw_data"], frame_info["width"], frame_info["height"]
        )
    raise ValueError("暂不支持的原始像素格式: {}".format(pixel_format))


def decode_packet_to_bgr(packet_type, payload):
    if packet_type == PACKET_TYPE_IMAGE:
        return decode_image_payload(payload)
    if packet_type == PACKET_TYPE_RAW_IMAGE:
        frame_info = parse_raw_frame_payload(payload)
        return decode_raw_frame_to_bgr(frame_info)
    raise ValueError("不支持的图像数据包类型: {}".format(packet_type))


def recognize_plate(image_bgr):
    result = RECOGNIZER.recognize(image_bgr)
    plate_number = result.get("plate_number", "")
    confidence = float(result.get("confidence", 0.0))
    if plate_number:
        log(
            "识别成功: {} confidence={:.4f} plate_type={} resize_rate={}".format(
                plate_number,
                confidence,
                result.get("plate_type"),
                result.get("resize_rate"),
            )
        )
        return True, plate_number, result

    log("识别失败: 未检测到有效车牌")
    return False, "", result


@lru_cache(maxsize=128)
def generate_tts_wav(text):
    if not text:
        raise ValueError("文本为空，无法生成语音")

    if not os.path.exists(AIKIT_BIN):
        raise RuntimeError("未找到 aikit_test: {}".format(AIKIT_BIN))

    with tempfile.NamedTemporaryFile(delete=False, suffix=".wav") as tmp_file:
        wav_path = tmp_file.name

    try:
        env = os.environ.copy()
        ld_library_path = env.get("LD_LIBRARY_PATH", "")
        env["LD_LIBRARY_PATH"] = "{}:{}".format(AIKIT_LIBS_DIR, ld_library_path).rstrip(
            ":"
        )

        result = subprocess.run(
            [AIKIT_BIN, text, wav_path],
            cwd=ROOT_DIR,
            env=env,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=20,
        )

        if result.returncode != 0:
            raise RuntimeError(
                "aikit_test 执行失败 rc={} stdout={} stderr={}".format(
                    result.returncode,
                    result.stdout.decode("utf-8", errors="replace"),
                    result.stderr.decode("utf-8", errors="replace"),
                )
            )

        if not os.path.exists(wav_path):
            raise RuntimeError("aikit_test 未生成 wav 文件")

        with open(wav_path, "rb") as f:
            wav_data = f.read()

        if not wav_data:
            raise RuntimeError("wav 文件为空")

        return wav_data
    finally:
        if os.path.exists(wav_path):
            os.remove(wav_path)


def handle_recognition_success(conn, recognition_result, base_name, source="unknown"):
    plate_number = recognition_result["plate_number"]
    confidence = float(recognition_result.get("confidence", 0.0))
    send_result(conn, True, "", plate_number, confidence)
    CLOUD_SYNC.report_plate_result(
        plate_number=plate_number,
        confidence=confidence,
        plate_type=recognition_result.get("plate_type"),
        source=source,
    )

    try:
        wav_data = generate_tts_wav(plate_number)
        wav_name = "{}_plate.wav".format(base_name)
        send_audio(conn, wav_name, wav_data, "plate_result", plate_number)
        log("返回语音文件: {}".format(wav_name))
        CLOUD_SYNC.report_tts_event(
            event_type="plate_result",
            text=plate_number,
            file_name=wav_name,
            status="success",
        )
    except Exception as exc:
        log("语音生成失败: {}".format(exc))
        CLOUD_SYNC.report_tts_event(
            event_type="plate_result",
            text=plate_number,
            file_name="",
            status="failed",
            detail=str(exc),
        )


def handle_image_packet(conn, packet_type, payload, base_name):
    image_bgr = decode_packet_to_bgr(packet_type, payload)
    success, _, recognition_result = recognize_plate(image_bgr)
    source = "image" if packet_type == PACKET_TYPE_IMAGE else "raw_image"
    if success:
        handle_recognition_success(conn, recognition_result, base_name, source=source)
        return
    send_result(conn, False, "plate not recognized", "", 0.0)
    CLOUD_SYNC.report_plate_failure(reason="plate not recognized", source=source)


def handle_text_packet(conn, payload):
    text = payload.decode("utf-8", errors="replace")
    log("收到文本: {}".format(text))
    request = json.loads(text)
    if request.get("type") != "tts":
        raise ValueError("未知文本请求类型: {}".format(request.get("type")))

    speech_text = request.get("text", "")
    event_type = request.get("event", "")
    file_name = request.get("file_name", "")
    if not speech_text:
        raise ValueError("tts 请求缺少 text")
    if not file_name:
        raise ValueError("tts 请求缺少 file_name")

    wav_data = generate_tts_wav(speech_text)
    send_audio(conn, file_name, wav_data, event_type, speech_text)
    log("返回语音文件: {} event={}".format(file_name, event_type))
    CLOUD_SYNC.report_tts_event(
        event_type=event_type,
        text=speech_text,
        file_name=file_name,
        status="success",
    )


def handle_client(conn, addr):
    peer = "{}:{}".format(addr[0], addr[1])
    log("客户端连接: {}".format(peer))
    try:
        while True:
            header = recv_exact(conn, PACKET_HEADER_SIZE)
            if not header:
                log("客户端断开: {}".format(peer))
                break

            payload_size, packet_type = struct.unpack(">IB", header)
            payload = recv_exact(conn, payload_size)
            if not payload:
                log("客户端断开: {}".format(peer))
                break

            log("收到数据包: type={}, size={}".format(packet_type, payload_size))
            base_name = datetime.now().strftime("%Y%m%d_%H%M%S_%f")

            try:
                if packet_type in (PACKET_TYPE_IMAGE, PACKET_TYPE_RAW_IMAGE):
                    handle_image_packet(conn, packet_type, payload, base_name)
                elif packet_type == PACKET_TYPE_TEXT:
                    handle_text_packet(conn, payload)
                else:
                    raise ValueError("未知数据包类型: {}".format(packet_type))

            except Exception as exc:
                log("处理数据包失败: {}".format(exc))
                send_result(conn, False, str(exc), "", 0.0)
    finally:
        conn.close()


def main():
    host = os.environ.get("SERVER_HOST", HOST)
    port = int(os.environ.get("SERVER_PORT", PORT))
    maybe_pull_business_db()
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind((host, port))
    server.listen(8)

    log("识别模型已加载")
    log("等待连接 {}:{} ...".format(host, port))

    while True:
        conn, addr = server.accept()
        worker = threading.Thread(target=handle_client, args=(conn, addr))
        worker.daemon = True
        worker.start()


if __name__ == "__main__":
    main()
