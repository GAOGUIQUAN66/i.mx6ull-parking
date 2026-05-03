#!/usr/bin/env python3
import json
import os
import socket
import struct
import subprocess
import tempfile
from datetime import datetime

import cv2
import numpy as np

from lpr_service import PlateRecognitionService


HOST = "192.168.137.121"
PORT = 8888

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR = os.path.dirname(BASE_DIR)
SAVE_DIR = os.path.join(BASE_DIR, "pic")
AIKIT_BIN = os.path.join(ROOT_DIR, "aikit_test")
AIKIT_LIBS_DIR = os.path.join(ROOT_DIR, "libs")

PACKET_HEADER_SIZE = 5
PACKET_TYPE_IMAGE = 1
PACKET_TYPE_TEXT = 2
PACKET_TYPE_RESULT = 3
PACKET_TYPE_RAW_IMAGE = 4
PACKET_TYPE_AUDIO = 5

RECOGNIZER = PlateRecognitionService()


def detect_extension(payload):
    if payload.startswith(b"\xff\xd8\xff"):
        return ".jpg"
    if payload.startswith(b"BM"):
        return ".bmp"
    if payload.startswith(b"\x89PNG\r\n\x1a\n"):
        return ".png"
    return ".bin"


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


def save_rgb565_preview(raw_data, width, height, path):
    row_stride = width * 3
    padding = (4 - (row_stride % 4)) % 4
    image_size = (row_stride + padding) * height
    file_size = 14 + 40 + image_size

    with open(path, "wb") as f:
        f.write(b"BM")
        f.write(struct.pack("<IHHI", file_size, 0, 0, 54))
        f.write(
            struct.pack(
                "<IIIHHIIIIII", 40, width, height, 1, 24, 0, image_size, 0, 0, 0, 0
            )
        )

        for y in range(height - 1, -1, -1):
            row = bytearray()
            base = y * width * 2
            for x in range(width):
                offset = base + x * 2
                pixel = raw_data[offset] | (raw_data[offset + 1] << 8)
                r = (pixel >> 11) & 0x1F
                g = (pixel >> 5) & 0x3F
                b = pixel & 0x1F
                r = (r << 3) | (r >> 2)
                g = (g << 2) | (g >> 4)
                b = (b << 3) | (b >> 2)
                row.extend([b, g, r])
            if padding:
                row.extend(b"\x00" * padding)
            f.write(row)


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


def save_raw_frame(frame_info, base_name):
    raw_name = "{}_{}x{}_{}.raw".format(
        base_name,
        frame_info["width"],
        frame_info["height"],
        frame_info["pixel_format"].lower(),
    )
    raw_path = os.path.join(SAVE_DIR, raw_name)
    with open(raw_path, "wb") as f:
        f.write(frame_info["raw_data"])

    meta = {
        "width": frame_info["width"],
        "height": frame_info["height"],
        "pixel_format": frame_info["pixel_format"],
        "data_size": len(frame_info["raw_data"]),
        "raw_file": raw_name,
    }

    if frame_info["pixel_format"] == "RGBP":
        preview_name = "{}_preview.bmp".format(base_name)
        preview_path = os.path.join(SAVE_DIR, preview_name)
        save_rgb565_preview(
            frame_info["raw_data"], frame_info["width"], frame_info["height"], preview_path
        )
        meta["preview_file"] = preview_name
        print("保存预览: {}".format(preview_path))

    meta_path = os.path.join(SAVE_DIR, "{}.json".format(base_name))
    with open(meta_path, "w") as f:
        json.dump(meta, f, indent=2, sort_keys=True, ensure_ascii=False)

    print("保存原始帧: {}".format(raw_path))
    print("保存元信息: {}".format(meta_path))


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


def recognize_plate(image_bgr):
    result = RECOGNIZER.recognize(image_bgr)
    plate_number = result.get("plate_number", "")
    if plate_number:
        print(
            "识别成功: {} color={} resize_rate={}".format(
                plate_number,
                result.get("color"),
                result.get("resize_rate"),
            )
        )
        return True, plate_number, result

    print("识别失败: 未检测到有效车牌")
    return False, "", result


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
        env["LD_LIBRARY_PATH"] = "{}:{}".format(AIKIT_LIBS_DIR, ld_library_path).rstrip(":")

        result = subprocess.run(
            [AIKIT_BIN, text, wav_path],
            cwd=ROOT_DIR,
            env=env,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
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


def handle_recognition_result(conn, plate_number, base_name):
    send_result(conn, True, "", plate_number, 0.99)

    try:
        wav_data = generate_tts_wav(plate_number)
        wav_name = "{}_plate.wav".format(base_name)
        send_audio(conn, wav_name, wav_data, "plate_result", plate_number)
        print("返回语音文件: {}".format(wav_name))
    except Exception as exc:
        print("语音生成失败: {}".format(exc))


def handle_client(conn, addr):
    print("客户端连接: {}".format(addr[0]))
    try:
        while True:
            header = recv_exact(conn, PACKET_HEADER_SIZE)
            if not header:
                print("客户端断开: {}".format(addr[0]))
                break

            payload_size, packet_type = struct.unpack(">IB", header)
            payload = recv_exact(conn, payload_size)
            if not payload:
                print("客户端断开: {}".format(addr[0]))
                break

            print("收到数据包: type={}, size={}".format(packet_type, payload_size))
            base_name = datetime.now().strftime("%Y%m%d_%H%M%S_%f")

            try:
                if packet_type == PACKET_TYPE_IMAGE:
                    ext = detect_extension(payload)
                    image_path = os.path.join(SAVE_DIR, base_name + ext)
                    with open(image_path, "wb") as f:
                        f.write(payload)
                    print("保存图像: {}".format(image_path))

                    image_bgr = decode_image_payload(payload)
                    success, plate_number, _ = recognize_plate(image_bgr)
                    if success:
                        handle_recognition_result(conn, plate_number, base_name)
                    else:
                        print("未识别到车牌，不回传结果")
                elif packet_type == PACKET_TYPE_RAW_IMAGE:
                    frame_info = parse_raw_frame_payload(payload)
                    save_raw_frame(frame_info, base_name)
                    image_bgr = decode_raw_frame_to_bgr(frame_info)
                    success, plate_number, _ = recognize_plate(image_bgr)
                    if success:
                        handle_recognition_result(conn, plate_number, base_name)
                    else:
                        print("未识别到车牌，不回传结果")
                elif packet_type == PACKET_TYPE_TEXT:
                    text = payload.decode("utf-8", errors="replace")
                    print("收到文本: {}".format(text))
                    request = json.loads(text)
                    if request.get("type") == "tts":
                        speech_text = request.get("text", "")
                        event_type = request.get("event", "")
                        file_name = request.get("file_name", "")
                        if speech_text and file_name:
                            wav_data = generate_tts_wav(speech_text)
                            send_audio(conn, file_name, wav_data, event_type, speech_text)
                            print("返回语音文件: {} event={}".format(file_name, event_type))
                else:
                    print("未知数据包类型: {}".format(packet_type))
            except Exception as exc:
                print("处理数据包失败: {}".format(exc))
                send_result(conn, False, str(exc), "", 0.0)
    finally:
        conn.close()


def main():
    os.makedirs(SAVE_DIR, exist_ok=True)

    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind((HOST, PORT))
    server.listen(5)

    print("识别模型已加载")
    print("等待连接 {}:{} ...".format(HOST, PORT))

    while True:
        conn, addr = server.accept()
        handle_client(conn, addr)


if __name__ == "__main__":
    main()
