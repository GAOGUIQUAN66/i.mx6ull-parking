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
    qDebug() << "Smart parking system - starting";
    qDebug() << "========================================\n";

    // Initialize hardware
    HardwareInit hardware;
    HardwareInit::Config config;
    config.serialDevice = "/dev/ttymxc2";
    config.serialBaudRate = 9600;
    config.cameraDevice = "";  // Camera opened in MainWindow, not here
    config.cameraWidth = 640;
    config.cameraHeight = 480;
    config.audioDevice = "default";
    config.audioSampleRate = 44100;
    config.audioChannels = 2;
    config.beeperPath = "/sys/class/leds/beep/brightness";

    if (!hardware.initAll(config)) {
        qDebug() << "Hardware init failed:" << hardware.lastError();
    } else {
        qDebug() << "Hardware init OK";
    }

    // Show main window with hardware manager
    MainWindow mainWindow(&hardware);
    mainWindow.show();

    qDebug() << "Main window shown";

    return app.exec();
}
