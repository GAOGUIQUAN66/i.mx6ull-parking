#ifndef VIDEOTHREAD_H
#define VIDEOTHREAD_H

#include <QThread>
#include <QImage>
#include <QMutex>

class V4L2Camera;

/**
 * @brief 视频采集线程
 *
 * 从V4L2摄像头采集视频帧，发送信号给主线程显示
 */
class VideoThread : public QThread
{
    Q_OBJECT

public:
    explicit VideoThread(QObject *parent = nullptr);
    ~VideoThread();

    /**
     * @brief 设置摄像头设备
     */
    void setCamera(V4L2Camera *camera);

    /**
     * @brief 停止采集
     */
    void stop();

    /**
     * @brief 触发抓拍
     */
    void triggerCapture();

    /**
     * @brief 获取最新视频帧，旧帧会被覆盖以避免显示排队
     */
    QImage takeLatestFrame();

signals:
    /**
     * @brief 视频帧就绪
     * @param image 图像数据
     */
    void frameReady(const QImage &image);

    /**
     * @brief 抓拍完成
     * @param image 抓拍的图像（用于本地显示或兜底）
     * @param rawFrame 当前帧的V4L2原始缓冲
     */
    void captureDone(const QImage &image, const QByteArray &rawFrame);

protected:
    void run() override;

private:
    V4L2Camera *m_camera;
    bool m_running;
    bool m_captureRequested;
    QMutex m_mutex;
    QImage m_latestFrame;
    QMutex m_frameMutex;
};

#endif // VIDEOTHREAD_H
