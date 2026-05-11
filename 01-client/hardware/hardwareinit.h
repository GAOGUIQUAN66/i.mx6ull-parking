#ifndef HARDWAREINIT_H
#define HARDWAREINIT_H

#include <QObject>
#include "serialport.h"
#include "v4l2camera.h"
#include "alsaaudio.h"
#include "gpio.h"

/**
 * @brief 硬件初始化管理类
 *
 * 统一管理所有硬件设备的初始化和状态
 */
class HardwareInit : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 硬件配置结构体
     */
    struct Config {
        // 串口配置
        QString serialDevice = "/dev/ttymxc2";
        int serialBaudRate = 115200;

        // 摄像头配置 (IMX6 CSI摄像头)
        QString cameraDevice = "/dev/video1";
        int cameraWidth = 640;
        int cameraHeight = 480;

        // 音频配置
        QString audioDevice = "default";
        unsigned int audioSampleRate = 44100;
        unsigned int audioChannels = 2;

        // 蜂鸣器配置
        QString beeperPath = "/sys/class/leds/beep/brightness";

        // 数据库配置
        QString dbPath = "/run/media/mmcblk1p1/parking.db";
    };

    explicit HardwareInit(QObject *parent = nullptr);
    ~HardwareInit();

    /**
     * @brief 初始化所有硬件
     * @param config 配置参数
     * @return 成功返回true
     */
    bool initAll(const Config &config);

    /**
     * @brief 初始化串口
     */
    bool initSerial(const QString &device, int baudRate);

    /**
     * @brief 初始化摄像头
     */
    bool initCamera(const QString &device, int width, int height);

    /**
     * @brief 初始化音频
     */
    bool initAudio(const QString &device, unsigned int sampleRate, unsigned int channels);

    /**
     * @brief 初始化蜂鸣器
     */
    bool initBeeper(const QString &path);

    /**
     * @brief 获取串口对象
     */
    SerialPort* serialPort();

    /**
     * @brief 获取摄像头对象
     */
    V4L2Camera* camera();

    /**
     * @brief 获取音频对象
     */
    AlsaAudio* audio();

    /**
     * @brief 获取蜂鸣器对象
     */
    Beeper* beeper();

    /**
     * @brief 蜂鸣器报警
     * @param count 鸣叫次数
     * @param intervalMs 间隔时间（毫秒）
     */
    void alarm(int count = 3, int intervalMs = 200);

    /**
     * @brief 停止报警
     */
    void stopAlarm();

    /**
     * @brief 获取初始化状态
     */
    bool isInitialized() const;

    /**
     * @brief 获取最后错误信息
     */
    QString lastError() const;

signals:
    /**
     * @brief 初始化完成信号
     * @param success 是否成功
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
