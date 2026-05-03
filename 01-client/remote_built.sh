#!/bin/bash
#############################################################################
# ATK-IMX6U Qt 项目构建 + 部署脚本
#############################################################################
# ==================== 配置区域 ====================
PROJECT_PATH="/home/chentao/qt_pro/jiedan/imx6ull/car_lis"
PROJECT_PRO="imx6ull_parking.pro"
QMAKE_BIN="/opt/qt5.7.0/bin/qmake"
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
APP_NAME="imx6ull_parking"
# ==================== 脚本开始 ====================
set -e

echo "=========================================="
echo "  ATK-IMX6U Qt 项目构建 + 部署"
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
cd $PROJECT_PATH
rm -f moc_*.cpp moc_*.h
make clean 2>/dev/null || true
echo "✓ 清理完成"
echo ""

# 执行构建
echo "[4/5] 开始构建..."
echo "------------------------------------------"
${QMAKE_BIN} ${PROJECT_PRO} \
    -spec ${QMAKE_SPEC} \
    ${QMAKE_ARGS} \
    "QMAKE_CC=${CROSS_PREFIX}-gcc" \
    "QMAKE_CXX=${CROSS_PREFIX}-g++" \
    "QMAKE_LINK=${CROSS_PREFIX}-g++" \
    "QMAKE_STRIP=${CROSS_PREFIX}-strip"
make -j${MAKE_JOBS}
echo ""
echo "✓ 编译成功！输出文件: $PROJECT_PATH/$APP_NAME"
echo ""

# 部署到开发板
echo "[5/5] 部署到开发板 ${BOARD_USER}@${BOARD_IP}:${BOARD_DEPLOY_DIR}..."
echo "------------------------------------------"

# 确保目标目录存在
ssh ${BOARD_USER}@${BOARD_IP} "mkdir -p ${BOARD_DEPLOY_DIR}"

# 传输文件
scp ${PROJECT_PATH}/${APP_NAME} ${BOARD_USER}@${BOARD_IP}:${BOARD_DEPLOY_DIR}/

echo ""
echo "=========================================="
echo "✓ 部署完成！"
echo "  文件已传输至开发板: ${BOARD_DEPLOY_DIR}/${APP_NAME}"
echo "  如需运行，请 ssh 到开发板后执行："
echo "  ssh ${BOARD_USER}@${BOARD_IP}"
echo "  ${BOARD_DEPLOY_DIR}/${APP_NAME}"
echo "=========================================="