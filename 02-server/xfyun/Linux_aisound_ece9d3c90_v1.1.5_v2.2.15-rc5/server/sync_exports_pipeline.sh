#!/usr/bin/env bash
set -euo pipefail

# CSV同步总脚本（板子 -> Ubuntu -> 云端）
# 用法:
#   ./sync_exports_pipeline.sh
#   MODE=pull ./sync_exports_pipeline.sh
#   MODE=push ./sync_exports_pipeline.sh
#   MODE=all  ./sync_exports_pipeline.sh
#
# 可选环境变量:
#   BOARD_HOST=192.168.137.100
#   BOARD_USER=root
#   REMOTE_BOARD_EXPORT_DIR=/run/media/mmcblk1p1/exports
#   LOCAL_EXPORT_DIR=$HOME/linux/IMX6ULL/i.mx6ull-parking/data/exports
#   CLOUD_HOST=123.207.66.114
#   CLOUD_USER=ubuntu
#   REMOTE_CLOUD_EXPORT_DIR=~/parking-cloud/import_csv

MODE="${MODE:-all}"  # pull | push | all

BOARD_HOST="${BOARD_HOST:-192.168.137.100}"
BOARD_USER="${BOARD_USER:-root}"
REMOTE_BOARD_EXPORT_DIR="${REMOTE_BOARD_EXPORT_DIR:-/run/media/mmcblk1p1/exports}"

LOCAL_EXPORT_DIR="${LOCAL_EXPORT_DIR:-$HOME/linux/IMX6ULL/i.mx6ull-parking/data/exports}"

CLOUD_HOST="${CLOUD_HOST:-123.207.66.114}"
CLOUD_USER="${CLOUD_USER:-ubuntu}"
REMOTE_CLOUD_EXPORT_DIR="${REMOTE_CLOUD_EXPORT_DIR:-~/parking-cloud/import_csv}"

do_pull() {
    echo "[STEP] Pull CSV from board..."
    echo "[INFO] Board: ${BOARD_USER}@${BOARD_HOST}"
    echo "[INFO] Remote board dir: ${REMOTE_BOARD_EXPORT_DIR}"
    echo "[INFO] Local dir       : ${LOCAL_EXPORT_DIR}"
    mkdir -p "${LOCAL_EXPORT_DIR}"
    scp -r "${BOARD_USER}@${BOARD_HOST}:${REMOTE_BOARD_EXPORT_DIR}/." "${LOCAL_EXPORT_DIR}/"
    echo "[DONE] Pull completed."
}

do_push() {
    echo "[STEP] Push CSV to cloud..."
    echo "[INFO] Cloud: ${CLOUD_USER}@${CLOUD_HOST}"
    echo "[INFO] Local dir      : ${LOCAL_EXPORT_DIR}"
    echo "[INFO] Remote cloud dir: ${REMOTE_CLOUD_EXPORT_DIR}"
    if [ ! -d "${LOCAL_EXPORT_DIR}" ]; then
        echo "[ERROR] local export dir not found: ${LOCAL_EXPORT_DIR}"
        exit 1
    fi
    ssh "${CLOUD_USER}@${CLOUD_HOST}" "mkdir -p ${REMOTE_CLOUD_EXPORT_DIR}"
    scp -r "${LOCAL_EXPORT_DIR}/." "${CLOUD_USER}@${CLOUD_HOST}:${REMOTE_CLOUD_EXPORT_DIR}/"
    echo "[DONE] Push completed."
}

case "${MODE}" in
    pull)
        do_pull
        ;;
    push)
        do_push
        ;;
    all)
        do_pull
        do_push
        ;;
    *)
        echo "[ERROR] invalid MODE: ${MODE}, expected: pull|push|all"
        exit 1
        ;;
esac

echo "[DONE] CSV pipeline finished. MODE=${MODE}"
