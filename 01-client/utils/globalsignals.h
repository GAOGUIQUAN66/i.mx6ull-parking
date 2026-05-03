#ifndef GLOBALSIGNALS_H
#define GLOBALSIGNALS_H

#include <QObject>
#include <QString>
#include <QImage>
#include <QByteArray>

/**
 * @brief 全局信号定义
 *
 * 定义跨线程通信的信号，供各模块使用
 * 使用单例模式，确保全局唯一实例
 */
class GlobalSignals : public QObject
{
    Q_OBJECT

public:
    static GlobalSignals* instance();

private:
    explicit GlobalSignals(QObject *parent = nullptr);
    static GlobalSignals *m_instance;

signals:
    // ========== 视频相关信号 ==========

    /**
     * @brief 视频帧就绪
     * @param frame 图像数据
     */
    void videoFrameReady(const QImage &frame);

    /**
     * @brief 抓拍触发信号
     */
    void captureTriggered();

    /**
     * @brief 抓拍图像就绪（用于网络传输）
     * @param imageData 原始图像数据
     */
    void captureImageReady(const QByteArray &imageData);

    // ========== RFID相关信号 ==========

    /**
     * @brief RFID卡检测到
     * @param cardId 卡号
     */
    void rfidCardDetected(const QString &cardId);

    // ========== 车牌识别相关信号 ==========

    /**
     * @brief 车牌识别完成
     * @param plateNumber 车牌号
     * @param confidence 置信度（0-1）
     */
    void plateRecognized(const QString &plateNumber, float confidence);

    /**
     * @brief 车牌识别失败
     * @param reason 失败原因
     */
    void plateRecognitionFailed(const QString &reason);

    // ========== 语音播报相关信号 ==========

    /**
     * @brief 语音播报请求
     * @param text 待播报文本
     */
    void audioPlayRequest(const QString &text);

    /**
     * @brief 语音数据就绪
     * @param audioData PCM音频数据
     */
    void audioDataReady(const QByteArray &audioData);

    /**
     * @brief 语音播放完成
     */
    void audioPlaybackFinished();

    // ========== 数据库/业务相关信号 ==========

    /**
     * @brief 车辆入场成功
     * @param plateNumber 车牌号
     * @param entryTime 入场时间
     */
    void vehicleEntrySuccess(const QString &plateNumber, const QDateTime &entryTime);

    /**
     * @brief 车辆出场成功
     * @param plateNumber 车牌号
     * @param fee 停车费用
     */
    void vehicleExitSuccess(const QString &plateNumber, double fee);

    /**
     * @brief Parking slots更新
     * @param available 空闲车位数
     * @param total Total数
     */
    void parkingStatusUpdated(int available, int total);

    /**
     * @brief 报警触发
     * @param reason 报警原因
     */
    void alarmTriggered(const QString &reason);

    // ========== 网络相关信号 ==========

    /**
     * @brief 网络连接状态变化
     * @param connected 是否已连接
     */
    void networkConnectionChanged(bool connected);

    /**
     * @brief Network
     * @param error 错误信息
     */
    void networkError(const QString &error);
};

// 全局信号宏，方便使用
#define g_signals GlobalSignals::instance()

#endif // GLOBALSIGNALS_H
