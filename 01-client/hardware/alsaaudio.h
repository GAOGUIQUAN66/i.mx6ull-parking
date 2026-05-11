#ifndef ALSAAUDIO_H
#define ALSAAUDIO_H

#include <QObject>
#include <QString>
#include <QByteArray>

// ALSA库暂时禁用，使用前向声明
// 前向声明ALSA类型
struct snd_pcm_t;

/**
 * @brief ALSA音频播放类
 *
 * 封装ALSA音频库，提供音频播放功能
 * 当前为桩实现，待ALSA库安装后替换
 */
class AlsaAudio : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 音频格式
     */
    enum Format {
        FORMAT_S16_LE,  // 16位有符号小端
        FORMAT_S24_LE,  // 24位有符号小端
        FORMAT_S32_LE   // 32位有符号小端
    };

    /**
     * @brief 播放状态
     */
    enum State {
        StateClosed,   // 已关闭
        StateReady,    // 就绪
        StatePlaying   // 正在播放
    };

    explicit AlsaAudio(QObject *parent = nullptr);
    ~AlsaAudio();

    /**
     * @brief 打开音频设备
     * @param device 设备名称，如 "default" 或 "hw:0,0"
     * @return 成功返回true
     */
    bool open(const QString &device = "default");

    /**
     * @brief 关闭音频设备
     */
    void close();

    /**
     * @brief 设置音频参数
     * @param sampleRate 采样率，如44100、48000
     * @param channels 声道数，1=单声道，2=立体声
     * @param format 音频格式
     * @return 成功返回true
     */
    bool setParams(unsigned int sampleRate, unsigned int channels, Format format = FORMAT_S16_LE);

    /**
     * @brief 播放音频数据
     * @param data 音频数据（PCM格式）
     * @return 成功返回true
     */
    bool play(const QByteArray &data);

    /**
     * @brief 播放音频数据（异步）
     * @param data 音频数据
     */
    void playAsync(const QByteArray &data);

    /**
     * @brief 停止播放
     */
    void stop();

    /**
     * @brief 等待播放完成
     * @param timeoutMs 超时时间（毫秒）
     * @return 成功返回true
     */
    bool waitFinished(int timeoutMs = 5000);

    /**
     * @brief 获取当前状态
     */
    State state() const;

    /**
     * @brief 获取最后错误信息
     */
    QString lastError() const;

    /**
     * @brief 获取采样率
     */
    unsigned int sampleRate() const { return m_sampleRate; }

    /**
     * @brief 获取声道数
     */
    unsigned int channels() const { return m_channels; }

signals:
    /**
     * @brief 播放完成信号
     */
    void playbackFinished();

    /**
     * @brief 播放错误信号
     */
    void playbackError(const QString &error);

private:
    snd_pcm_t *m_handle;       // ALSA句柄（桩）
    unsigned int m_sampleRate; // 采样率
    unsigned int m_channels;   // 声道数
    Format m_format;           // 音频格式
    State m_state;             // 当前状态
    QString m_lastError;
};

#endif // ALSAAUDIO_H
