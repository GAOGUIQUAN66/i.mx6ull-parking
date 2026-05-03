#ifndef V4L2CAMERA_H
#define V4L2CAMERA_H

#include <QObject>
#include <QImage>
#include <QString>
#include <QByteArray>

/**
 * @brief V4L2 capture
 *
 * Linux V4L2 capture for preview + ROI
 * Tested on i.MX6 CSI sensors
 */
class V4L2Camera : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Pixel format
     */
    enum PixelFormat {
        FORMAT_YUYV,  // YUYV
        FORMAT_UYVY,  // UYVY
        FORMAT_MJPEG, // MJPEG
        FORMAT_JPEG,  // JPEG
        FORMAT_RGB565 // RGB565
    };

    /**
     * @brief Capture state
     */
    enum State {
        StateClosed,   // Closed
        StateReady,    // idle
        StateStreaming // streaming
    };

    explicit V4L2Camera(QObject *parent = nullptr);
    ~V4L2Camera();

    /**
     * @brief Open camera
     * @param device e.g. /dev/video1
     * @return true on success
     */
    bool open(const QString &device);

    /**
     * @brief Close camera
     */
    void close();

    /**
     * @brief Negotiate format
     * @param width
     * @param height
     * @param format pixel format
     * @return true on success
     */
    bool setFormat(int width, int height, PixelFormat format = FORMAT_YUYV);

    /**
     * @brief REQBUFS
     * @param count buffers
     * @return true on success
     */
    bool requestBuffers(int count = 4);

    /**
     * @brief STREAMON
     * @return true on success
     */
    bool startCapture();

    /**
     * @brief STREAMOFF
     */
    void stopCapture();

    /**
     * @brief Blocking grab
     * @param timeoutMs ms
     * @return QImage or empty
     */
    QImage grabFrame(int timeoutMs = 1000, QByteArray *rawFrame = nullptr);

    /**
     * @brief Snapshot + mmap payload
     * @param timeoutMs ms
     * @return raw bytes for uplink
     */
    QByteArray grabRawFrame(int timeoutMs = 1000);

    /**
     * @brief State accessor
     */
    State state() const;

    /**
     * @brief Width
     */
    int width() const { return m_width; }

    /**
     * @brief Height
     */
    int height() const { return m_height; }

    /**
     * @brief FourCC string
     */
    QString pixelFormatName() const;

    /**
     * @brief Last error string
     */
    QString lastError() const;

signals:
    /**
     * @brief frameReady
     */
    void frameReady(const QImage &frame);

private:
    int m_fd;           // fd
    int m_width;        // width
    int m_height;       // height
    PixelFormat m_format; // pixel format
    State m_state;      // state
    QString m_device;   // device node
    QString m_lastError;
    quint32 m_actualFormat; // negotiated fourcc

    void** m_buffers;   // mmap buffers
    int m_bufferCount;  // buffer count

    /**
     * @brief mmap setup
     */
    bool initMmap(int count);

    /**
     * @brief mmap teardown
     */
    void freeBuffers();

    /**
     * @brief YUYV->RGB888
     */
    QImage yuyvToRgb(const void *data, int width, int height);

    /**
     * @brief UYVY->RGB888
     */
    QImage uyvyToRgb(const void *data, int width, int height);

    /**
     * @brief RGB565->RGB888
     */
    QImage rgb565ToRgb(const void *data, int width, int height);
};

#endif // V4L2CAMERA_H
