#-------------------------------------------------
# imx6ull_parking — ARM开发板 Qt 车牌识别系统
# ATK-IMX6U / Qt5.12.9 / ARM Linux
#-------------------------------------------------

QT += core gui widgets network sql

CONFIG += c++11 console
TARGET = imx6ull_parking
TEMPLATE = app

# ── 公共模块 (utils/) ──────────────────────────────────────
SOURCES += utils/globalsignals.cpp
HEADERS += utils/globalsignals.h

# ── 硬件初始化模块 (hardware/) ──────────────────────────────────
SOURCES += \
    hardware/serialport.cpp \
    hardware/v4l2camera.cpp \
    hardware/alsaaudio.cpp \
    hardware/gpio.cpp \
    hardware/hardwareinit.cpp

HEADERS += \
    hardware/serialport.h \
    hardware/v4l2camera.h \
    hardware/alsaaudio.h \
    hardware/gpio.h \
    hardware/hardwareinit.h

# ── 数据库模块 (database/) ──────────────────────────────────────
SOURCES += database/database.cpp
HEADERS += database/database.h

# ── 界面模块 (ui/) ──────────────────────────────────────────────
SOURCES += \
	ui/mainwindow.cpp \
	ui/entrydialog.cpp \
	ui/exitdialog.cpp \
	ui/querywindow.cpp \
	ui/settingswindow.cpp

HEADERS += \
	ui/mainwindow.h \
	ui/entrydialog.h \
	ui/exitdialog.h \
	ui/querywindow.h \
	ui/settingswindow.h

# ── 视频采集模块 (video/) ──────────────────────────────────────
SOURCES += video/videothread.cpp
HEADERS += video/videothread.h

# ── RFID模块 (rfid/) ───────────────────────────────────────────
SOURCES += rfid/rfidthread.cpp
HEADERS += rfid/rfidthread.h

# ── 网络通信模块 (network/) ─────────────────────────────────────
SOURCES += network/networkclient.cpp
HEADERS += network/networkclient.h

# ── 音频播报模块 (audio/) ───────────────────────────────────────
SOURCES += audio/audiothread.cpp
HEADERS += audio/audiothread.h

# ── 主程序 ──────────────────────────────────────────────────────
SOURCES += main.cpp

# ── Include 路径 ────────────────────────────────────────────────
INCLUDEPATH += . utils hardware database ui video network rfid audio

# ── 链接库 ──────────────────────────────────────────────────────
# LIBS += -lasound  # ALSA库，暂时禁用

# ── 安装路径 ────────────────────────────────────────────────────
target.path = /usr/bin
INSTALLS += target
