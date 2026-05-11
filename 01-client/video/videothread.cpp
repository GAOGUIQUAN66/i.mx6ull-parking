#include "videothread.h"
#include "../hardware/v4l2camera.h"
#include <QDebug>

namespace {

QByteArray mirrorRgb565Frame(const QByteArray &rawFrame, int width, int height)
{
    const int bytesPerPixel = 2;
    const int rowBytes = width * bytesPerPixel;
    if (rawFrame.size() != rowBytes * height) {
        return rawFrame;
    }

    QByteArray mirrored(rawFrame.size(), '\0');
    const char *src = rawFrame.constData();
    char *dst = mirrored.data();

    for (int y = 0; y < height; ++y) {
        const char *srcRow = src + y * rowBytes;
        char *dstRow = dst + y * rowBytes;
        for (int x = 0; x < width; ++x) {
            const int srcOffset = x * bytesPerPixel;
            const int dstOffset = (width - 1 - x) * bytesPerPixel;
            dstRow[dstOffset] = srcRow[srcOffset];
            dstRow[dstOffset + 1] = srcRow[srcOffset + 1];
        }
    }

    return mirrored;
}

QByteArray mirrorRawFrameIfNeeded(const QByteArray &rawFrame, V4L2Camera *camera)
{
    if (!camera || rawFrame.isEmpty()) {
        return rawFrame;
    }

    if (camera->pixelFormatName() == "RGBP") {
        return mirrorRgb565Frame(rawFrame, camera->width(), camera->height());
    }

    return rawFrame;
}

}

VideoThread::VideoThread(QObject *parent)
    : QThread(parent)
    , m_camera(nullptr)
    , m_running(false)
    , m_captureRequested(false)
{
}

VideoThread::~VideoThread()
{
    stop();
    wait();
}

void VideoThread::setCamera(V4L2Camera *camera)
{
    m_camera = camera;
}

void VideoThread::stop()
{
    QMutexLocker locker(&m_mutex);
    m_running = false;
}

void VideoThread::triggerCapture()
{
    QMutexLocker locker(&m_mutex);
    m_captureRequested = true;
}

QImage VideoThread::takeLatestFrame()
{
    QMutexLocker locker(&m_frameMutex);
    QImage frame = m_latestFrame;
    m_latestFrame = QImage();
    return frame;
}

void VideoThread::run()
{
    if (!m_camera) {
        qDebug() << "VideoThread: 摄像头未设置";
        return;
    }

    // 启动摄像头采集
    if (!m_camera->startCapture()) {
        qDebug() << "VideoThread: 启动采集失败:" << m_camera->lastError();
        return;
    }

    m_running = true;
    qDebug() << "VideoThread: 开始采集";

    while (m_running) {
        // 采集一帧
        QByteArray rawFrame;
        QImage frame = m_camera->grabFrame(1000, &rawFrame);
        if (!frame.isNull()) {
            frame = frame.mirrored(true, false);
        }
        if (!rawFrame.isEmpty()) {
            rawFrame = mirrorRawFrameIfNeeded(rawFrame, m_camera);
        }

        m_mutex.lock();
        bool shouldCapture = m_captureRequested;
        m_captureRequested = false;
        m_mutex.unlock();

        if (!frame.isNull()) {
            QMutexLocker locker(&m_frameMutex);
            m_latestFrame = frame;
        }

        if (shouldCapture && (!rawFrame.isEmpty() || !frame.isNull())) {
            emit captureDone(frame, rawFrame);
        }
    }

    m_camera->stopCapture();
    qDebug() << "VideoThread: 采集停止";
}
