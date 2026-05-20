#!/bin/bash
#############################################################################
# ATK-IMX6U Qt 项目构建 + 部署脚本（test）
#############################################################################
# ==================== 配置区域 ====================
PROJECT_PATH="/home/ubuntu-alientek/linux/IMX6ULL/i.mx6ull-parking/01-client"
PROJECT_PRO="imx6ull_parking.pro"
QMAKE_BIN="/home/ubuntu-alientek/qt5.12.9/qt-everywhere-src-5.12.9/arm-qt/bin/qmake"
QMAKE_SPEC="linux-arm-gnueabi-g++"
QMAKE_ARGS="-r CONFIG+=debug"
MAKE_JOBS="4"

# linaro 工具链
ARM_TOOLCHAIN="/usr/local/arm/gcc-linaro-4.9.4-2017.01-x86_64_arm-linux-gnueabihf/bin"
CROSS_PREFIX="arm-linux-gnueabihf"

# 开发板信息
BOARD_IP="192.168.137.100"
BOARD_USER="root"
BOARD_DEPLOY_DIR="/home/app"
APP_NAME="test"
BUILD_DIR="$PROJECT_PATH/build-arm"
# ==================== 脚本开始 ====================
set -e

echo "=========================================="
echo "  ATK-IMX6U Qt 项目构建 + 部署（test）"
echo "=========================================="
echo ""

# 检查项目目录
echo "[1/5] 检查项目目录..."
if [ ! -d "$PROJECT_PATH" ]; then
    echo "ERROR: 项目目录不存在: $PROJECT_PATH"
    exit 1
fi
echo "✓ 项目目录存在"
echo ""

# 设置交叉编译器路径
echo "[2/5] 设置交叉编译环境..."
export PATH=${ARM_TOOLCHAIN}:$PATH
if ! which ${CROSS_PREFIX}-g++ > /dev/null 2>&1; then
    echo "ERROR: 找不到交叉编译器: ${ARM_TOOLCHAIN}/${CROSS_PREFIX}-g++"
    exit 1
fi
echo "✓ 交叉编译器就绪: $(which ${CROSS_PREFIX}-g++)"
echo ""

# 清理旧构建
echo "[3/5] 清理旧的构建结果..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
rm -f moc_*.cpp moc_*.h
make clean 2>/dev/null || true
echo "✓ 清理完成"
echo ""

# 执行构建（输出到 build-arm）
echo "[4/5] 开始构建..."
echo "------------------------------------------"
${QMAKE_BIN} "$PROJECT_PATH/${PROJECT_PRO}" \
    -spec ${QMAKE_SPEC} \
    ${QMAKE_ARGS} \
    "QMAKE_CC=${CROSS_PREFIX}-gcc" \
    "QMAKE_CXX=${CROSS_PREFIX}-g++" \
    "QMAKE_LINK=${CROSS_PREFIX}-g++" \
    "QMAKE_STRIP=${CROSS_PREFIX}-strip"
make -j${MAKE_JOBS}
echo ""

# 固定使用 .pro 中 TARGET 对应的可执行文件，避免误传其他文件
APP_BIN="$BUILD_DIR/imx6ull_parking"
if [ ! -f "$APP_BIN" ]; then
    echo "ERROR: 未找到目标可执行文件: $APP_BIN"
    echo "请检查 qmake/make 输出，确认 TARGET=imx6ull_parking 已成功编译"
    exit 1
fi
echo "✓ 编译成功！输出文件: $APP_BIN"
echo ""

# 部署到开发板，部署名固定为 test
echo "[5/5] 部署到开发板 ${BOARD_USER}@${BOARD_IP}:${BOARD_DEPLOY_DIR}/${APP_NAME}..."
echo "------------------------------------------"

# 确保目标目录存在
ssh ${BOARD_USER}@${BOARD_IP} "mkdir -p ${BOARD_DEPLOY_DIR}"

# 传输文件并改名为 test
scp "$APP_BIN" ${BOARD_USER}@${BOARD_IP}:${BOARD_DEPLOY_DIR}/${APP_NAME}

# 本地与板端哈希对比，确认部署文件一致
LOCAL_MD5="$(md5sum "$APP_BIN" | awk '{print $1}')"
REMOTE_MD5="$(ssh ${BOARD_USER}@${BOARD_IP} "md5sum ${BOARD_DEPLOY_DIR}/${APP_NAME} | awk '{print \$1}'")"

echo ""
echo "本地MD5: ${LOCAL_MD5}"
echo "板端MD5: ${REMOTE_MD5}"
if [ "$LOCAL_MD5" = "$REMOTE_MD5" ]; then
    echo "✓ MD5一致，部署文件已确认更新"
else
    echo "⚠ MD5不一致，请检查网络/权限/磁盘空间后重试"
fi

echo ""
echo "=========================================="
echo "✓ 部署完成！"
echo "  文件已传输至开发板: ${BOARD_DEPLOY_DIR}/${APP_NAME}"
echo "  如需运行，请 ssh 到开发板后执行："
echo "  ssh ${BOARD_USER}@${BOARD_IP}"
echo "  ${BOARD_DEPLOY_DIR}/${APP_NAME}"
echo "=========================================="
