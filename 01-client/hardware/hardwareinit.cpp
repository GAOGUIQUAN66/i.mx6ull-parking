#include "hardwareinit.h"
#include <QThread>
#include <QDebug>

HardwareInit::HardwareInit(QObject *parent)
    : QObject(parent)
    , m_serial(nullptr)
    , m_camera(nullptr)
    , m_audio(nullptr)
    , m_beeper(nullptr)
    , m_initialized(false)
{
}

HardwareInit::~HardwareInit()
{
    delete m_serial;
    delete m_camera;
    delete m_audio;
    delete m_beeper;
}

bool HardwareInit::initAll(const Config &config)
{
    // 初始化串口
    if (!config.serialDevice.isEmpty()) {
        if (!initSerial(config.serialDevice, config.serialBaudRate)) {
            emit initFinished(false);
            return false;
        }
    }

    // 初始化摄像头
    if (!config.cameraDevice.isEmpty()) {
        if (!initCamera(config.cameraDevice, config.cameraWidth, config.cameraHeight)) {
            emit initFinished(false);
            return false;
        }
    }

    // 初始化音频
    if (!config.audioDevice.isEmpty()) {
        if (!initAudio(config.audioDevice, config.audioSampleRate, config.audioChannels)) {
            emit initFinished(false);
            return false;
        }
    }

    // 初始化蜂鸣器
    if (!config.beeperPath.isEmpty()) {
        if (!initBeeper(config.beeperPath)) {
            emit initFinished(false);
            return false;
        }
    }

    m_initialized = true;
    emit initFinished(true);
    return true;
}

bool HardwareInit::initSerial(const QString &device, int baudRate)
{
    m_serial = new SerialPort(this);
    if (!m_serial->open(device, baudRate)) {
        m_lastError = QString("串口初始化失败: %1").arg(m_serial->lastError());
        delete m_serial;
        m_serial = nullptr;
        return false;
    }
    return true;
}

bool HardwareInit::initCamera(const QString &device, int width, int height)
{
    m_camera = new V4L2Camera(this);
    if (!m_camera->open(device)) {
        m_lastError = QString("摄像头初始化失败: %1").arg(m_camera->lastError());
        delete m_camera;
        m_camera = nullptr;
        return false;
    }

    // OV5671摄像头支持: RGBP(RGB565)、JPEG、YUYV
    // 优先级: JPEG > RGB565 > YUYV (避免UYVY转换问题)
    // 1. 优先尝试JPEG格式（Qt直接解码，颜色正确）
    if (m_camera->setFormat(width, height, V4L2Camera::FORMAT_JPEG)) {
        qDebug() << "摄像头初始化成功，使用JPEG格式";
    }
    // 2. JPEG失败则尝试RGB565
    else if (m_camera->setFormat(width, height, V4L2Camera::FORMAT_RGB565)) {
        qDebug() << "摄像头初始化成功，使用RGB565格式";
    }
    // 3. RGB565失败则尝试YUYV
    else if (m_camera->setFormat(width, height, V4L2Camera::FORMAT_YUYV)) {
        qDebug() << "摄像头初始化成功，使用YUYV格式";
    }
    else {
        m_lastError = QString("设置摄像头格式失败: %1").arg(m_camera->lastError());
        delete m_camera;
        m_camera = nullptr;
        return false;
    }

    if (!m_camera->requestBuffers(4)) {
        m_lastError = QString("申请摄像头缓冲区失败: %1").arg(m_camera->lastError());
        delete m_camera;
        m_camera = nullptr;
        return false;
    }

    return true;
}

bool HardwareInit::initAudio(const QString &device, unsigned int sampleRate, unsigned int channels)
{
    m_audio = new AlsaAudio(this);
    if (!m_audio->open(device)) {
        m_lastError = QString("音频设备初始化失败: %1").arg(m_audio->lastError());
        delete m_audio;
        m_audio = nullptr;
        return false;
    }

    if (!m_audio->setParams(sampleRate, channels)) {
        m_lastError = QString("设置音频参数失败: %1").arg(m_audio->lastError());
        delete m_audio;
        m_audio = nullptr;
        return false;
    }

    return true;
}

bool HardwareInit::initBeeper(const QString &path)
{
    m_beeper = new Beeper(this);
    if (!m_beeper->init(path)) {
        m_lastError = QString("蜂鸣器初始化失败: %1").arg(m_beeper->lastError());
        delete m_beeper;
        m_beeper = nullptr;
        return false;
    }

    return true;
}

SerialPort* HardwareInit::serialPort()
{
    return m_serial;
}

V4L2Camera* HardwareInit::camera()
{
    return m_camera;
}

AlsaAudio* HardwareInit::audio()
{
    return m_audio;
}

Beeper* HardwareInit::beeper()
{
    return m_beeper;
}

void HardwareInit::alarm(int count, int intervalMs)
{
    if (m_beeper) {
        m_beeper->alarm(count, intervalMs);
    }
}

void HardwareInit::stopAlarm()
{
    if (m_beeper) {
        m_beeper->off();
    }
}

bool HardwareInit::isInitialized() const
{
    return m_initialized;
}

QString HardwareInit::lastError() const
{
    return m_lastError;
}
