#include <QApplication>
#include <QDebug>

#include "ui/mainwindow.h"
#include "hardware/hardwareinit.h"
#include "database/database.h"
#include "utils/globalsignals.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    qDebug() << "========================================";
    qDebug() << "智能车库管理系统 - 启动";
    qDebug() << "========================================\n";

    // 初始化硬件
    HardwareInit hardware;
    HardwareInit::Config config;
    config.serialDevice = "/dev/ttymxc2";
    config.serialBaudRate = 9600;
    config.cameraDevice = "";  // 不在main中初始化摄像头，由MainWindow管理
    config.cameraWidth = 640;
    config.cameraHeight = 480;
    config.audioDevice = "default";
    config.audioSampleRate = 44100;
    config.audioChannels = 2;
    config.beeperPath = "/sys/class/leds/beep/brightness";

    if (!hardware.initAll(config)) {
        qDebug() << "硬件初始化失败:" << hardware.lastError();
    } else {
        qDebug() << "硬件初始化成功";
    }

    // 创建并显示主窗口，传入硬件管理对象
    MainWindow mainWindow(&hardware);
    mainWindow.show();

    qDebug() << "主窗口已显示";

    return app.exec();
}
