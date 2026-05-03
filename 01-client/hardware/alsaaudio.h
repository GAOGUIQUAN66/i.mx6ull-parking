#ifndef ALSAAUDIO_H
#define ALSAAUDIO_H

#include <QObject>
#include <QString>
#include <QByteArray>

// ALSA disabled (forward decl)
// opaque ALSA types
struct snd_pcm_t;

/**
 * @brief ALSA playback
 *
 * Thin ALSA wrapper
 * Stub until ALSA linked
 */
class AlsaAudio : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Sample format
     */
    enum Format {
        FORMAT_S16_LE,  // S16 LE
        FORMAT_S24_LE,  // S24 LE
        FORMAT_S32_LE   // S32 LE
    };

    /**
     * @brief Playback state
     */
    enum State {
        StateClosed,   // Closed
        StateReady,    // idle
        StatePlaying   // playing
    };

    explicit AlsaAudio(QObject *parent = nullptr);
    ~AlsaAudio();

    /**
     * @brief Open PCM
     * @param device e.g. default
     * @return true on success
     */
    bool open(const QString &device = "default");

    /**
     * @brief Close PCM
     */
    void close();

    /**
     * @brief HW params
     * @param sampleRate Hz
     * @param channels
     * @param format
     * @return true on success
     */
    bool setParams(unsigned int sampleRate, unsigned int channels, Format format = FORMAT_S16_LE);

    /**
     * @brief Write PCM
     * @param data PCM bytes
     * @return true on success
     */
    bool play(const QByteArray &data);

    /**
     * @brief Write PCM（异步）
     * @param data PCM
     */
    void playAsync(const QByteArray &data);

    /**
     * @brief Stop
     */
    void stop();

    /**
     * @brief Drain
     * @param timeoutMs ms
     * @return true on success
     */
    bool waitFinished(int timeoutMs = 5000);

    /**
     * @brief State accessor
     */
    State state() const;

    /**
     * @brief Last error string
     */
    QString lastError() const;

    /**
     * @brief Sample rate
     */
    unsigned int sampleRate() const { return m_sampleRate; }

    /**
     * @brief Channels
     */
    unsigned int channels() const { return m_channels; }

signals:
    /**
     * @brief playbackFinished
     */
    void playbackFinished();

    /**
     * @brief playbackError
     */
    void playbackError(const QString &error);

private:
    snd_pcm_t *m_handle;       // ALSA handle (stub)
    unsigned int m_sampleRate; // Hz
    unsigned int m_channels;   // channels
    Format m_format;           // sample format
    State m_state;             // playback state
    QString m_lastError;
};

#endif // ALSAAUDIO_H
