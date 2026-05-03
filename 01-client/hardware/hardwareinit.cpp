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
    // Serial
    if (!config.serialDevice.isEmpty()) {
        if (!initSerial(config.serialDevice, config.serialBaudRate)) {
            emit initFinished(false);
            return false;
        }
    }

    // Init camera
    if (!config.cameraDevice.isEmpty()) {
        if (!initCamera(config.cameraDevice, config.cameraWidth, config.cameraHeight)) {
            emit initFinished(false);
            return false;
        }
    }

    // Audio
    if (!config.audioDevice.isEmpty()) {
        if (!initAudio(config.audioDevice, config.audioSampleRate, config.audioChannels)) {
            emit initFinished(false);
            return false;
        }
    }

    // Beeper
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
        m_lastError = QString("Serial init failed: %1").arg(m_serial->lastError());
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
        m_lastError = QString("Camera init failed: %1").arg(m_camera->lastError());
        delete m_camera;
        m_camera = nullptr;
        return false;
    }

    // OV5671: RGBP/JPEG/YUYV
    // Priority JPEG > RGB565 > YUYV
    // 1) JPEG
    if (m_camera->setFormat(width, height, V4L2Camera::FORMAT_JPEG)) {
        qDebug() << "Camera OK (JPEG)";
    }
    // 2) RGB565
    else if (m_camera->setFormat(width, height, V4L2Camera::FORMAT_RGB565)) {
        qDebug() << "Camera OK (RGB565)";
    }
    // 3) YUYV
    else if (m_camera->setFormat(width, height, V4L2Camera::FORMAT_YUYV)) {
        qDebug() << "Camera OK (YUYV)";
    }
    else {
        m_lastError = QString("setFormat failed: %1").arg(m_camera->lastError());
        delete m_camera;
        m_camera = nullptr;
        return false;
    }

    if (!m_camera->requestBuffers(4)) {
        m_lastError = QString("Camera REQBUFS failed: %1").arg(m_camera->lastError());
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
        m_lastError = QString("Audio open failed: %1").arg(m_audio->lastError());
        delete m_audio;
        m_audio = nullptr;
        return false;
    }

    if (!m_audio->setParams(sampleRate, channels)) {
        m_lastError = QString("Audio params failed: %1").arg(m_audio->lastError());
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
        m_lastError = QString("Beeper init failed: %1").arg(m_beeper->lastError());
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
