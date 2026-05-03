#include "alsaaudio.h"

// ALSA stub
// #include <alsa/asoundlib.h>
#include <QThread>

class AudioPlayThread : public QThread
{
    Q_OBJECT
public:
    QByteArray data;
    AlsaAudio *audio;

    void run() override {
        if (audio && !data.isEmpty()) {
            audio->play(data);
            emit audio->playbackFinished();
        }
    }
};

// Stub handle
struct snd_pcm_t { int dummy; };

AlsaAudio::AlsaAudio(QObject *parent)
    : QObject(parent)
    , m_handle(nullptr)
    , m_sampleRate(44100)
    , m_channels(2)
    , m_format(FORMAT_S16_LE)
    , m_state(StateClosed)
{
}

AlsaAudio::~AlsaAudio()
{
    close();
}

bool AlsaAudio::open(const QString &device)
{
    // Stub OK
    Q_UNUSED(device);
    m_handle = new snd_pcm_t;
    m_state = StateReady;
    return true;
    // Real impl:
    // int err = snd_pcm_open(&m_handle, device.toUtf8().constData(),
    //                        SND_PCM_STREAM_PLAYBACK, 0);
    // if (err < 0) {
    //     m_lastError = QString("无法打开音频设备 %1: %2").arg(device).arg(snd_strerror(err));
    //     return false;
    // }
    // m_state = StateReady;
    // return true;
}

void AlsaAudio::close()
{
    if (m_handle) {
        delete m_handle;
        m_handle = nullptr;
    }
    m_state = StateClosed;
}

bool AlsaAudio::setParams(unsigned int sampleRate, unsigned int channels, Format format)
{
    if (!m_handle) {
        m_lastError = "Audio not open";
        return false;
    }

    // Stub OK
    m_sampleRate = sampleRate;
    m_channels = channels;
    m_format = format;
    return true;
}

bool AlsaAudio::play(const QByteArray &data)
{
    if (!m_handle) {
        m_lastError = "Audio not open";
        return false;
    }

    m_state = StatePlaying;

    // Stub delay
    int bytesPerSample = 2;
    int frameSize = m_channels * bytesPerSample;
    int frames = data.size() / frameSize;
    int durationMs = frames * 1000 / m_sampleRate;
    QThread::msleep(durationMs);

    m_state = StateReady;
    return true;
}

void AlsaAudio::playAsync(const QByteArray &data)
{
    AudioPlayThread *thread = new AudioPlayThread();
    thread->audio = this;
    thread->data = data;
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
}

void AlsaAudio::stop()
{
    m_state = StateReady;
}

bool AlsaAudio::waitFinished(int timeoutMs)
{
    Q_UNUSED(timeoutMs);
    return true;
}

AlsaAudio::State AlsaAudio::state() const
{
    return m_state;
}

QString AlsaAudio::lastError() const
{
    return m_lastError;
}

#include "alsaaudio.moc"
