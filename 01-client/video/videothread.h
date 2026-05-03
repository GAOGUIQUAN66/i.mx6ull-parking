#ifndef VIDEOTHREAD_H
#define VIDEOTHREAD_H

#include <QThread>
#include <QImage>
#include <QMutex>

class V4L2Camera;

/**
 * @brief Capture worker
 *
 * Pulls frames for UI + LPR uplink
 */
class VideoThread : public QThread
{
    Q_OBJECT

public:
    explicit VideoThread(QObject *parent = nullptr);
    ~VideoThread();

    /**
     * @brief Attach camera
     */
    void setCamera(V4L2Camera *camera);

    /**
     * @brief STREAMOFF
     */
    void stop();

    /**
     * @brief Snapshot
     */
    void triggerCapture();

    /**
     * @brief Latest RGB frame
     */
    QImage takeLatestFrame();

signals:
    /**
     * @brief Preview frame
     * @param image rgb
     */
    void frameReady(const QImage &image);

    /**
     * @brief Snapshot ready
     * @param image preview
     * @param rawFrame mmap payload
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
