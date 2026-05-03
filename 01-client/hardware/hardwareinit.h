#ifndef HARDWAREINIT_H
#define HARDWAREINIT_H

#include <QObject>
#include "serialport.h"
#include "v4l2camera.h"
#include "alsaaudio.h"
#include "gpio.h"

/**
 * @brief Board bring-up
 *
 * Owns serial/camera/audio/beeper
 */
class HardwareInit : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Board config
     */
    struct Config {
        // UART
        QString serialDevice = "/dev/ttymxc2";
        int serialBaudRate = 115200;

        // CSI camera
        QString cameraDevice = "/dev/video1";
        int cameraWidth = 640;
        int cameraHeight = 480;

        // Audio
        QString audioDevice = "default";
        unsigned int audioSampleRate = 44100;
        unsigned int audioChannels = 2;

        // Beeper LED
        QString beeperPath = "/sys/class/leds/beep/brightness";

        // State库配置
        QString dbPath = "/run/media/mmcblk1p1/parking.db";
    };

    explicit HardwareInit(QObject *parent = nullptr);
    ~HardwareInit();

    /**
     * @brief Init subsystems
     * @param config
     * @return true on success
     */
    bool initAll(const Config &config);

    /**
     * @brief Init UART
     */
    bool initSerial(const QString &device, int baudRate);

    /**
     * @brief Init camera
     */
    bool initCamera(const QString &device, int width, int height);

    /**
     * @brief Init audio
     */
    bool initAudio(const QString &device, unsigned int sampleRate, unsigned int channels);

    /**
     * @brief Init beeper
     */
    bool initBeeper(const QString &path);

    /**
     * @brief Serial accessor
     */
    SerialPort* serialPort();

    /**
     * @brief Camera accessor
     */
    V4L2Camera* camera();

    /**
     * @brief Audio accessor
     */
    AlsaAudio* audio();

    /**
     * @brief Beeper accessor
     */
    Beeper* beeper();

    /**
     * @brief Alarm pattern
     * @param count pulses
     * @param intervalMs gap
     */
    void alarm(int count = 3, int intervalMs = 200);

    /**
     * @brief Silence alarm
     */
    void stopAlarm();

    /**
     * @brief Init OK flag
     */
    bool isInitialized() const;

    /**
     * @brief Last error string
     */
    QString lastError() const;

signals:
    /**
     * @brief initFinished
     * @param success
     */
    void initFinished(bool success);

private:
    SerialPort *m_serial;
    V4L2Camera *m_camera;
    AlsaAudio *m_audio;
    Beeper *m_beeper;
    bool m_initialized;
    QString m_lastError;
};

#endif // HARDWAREINIT_H
