#ifndef V4L2CAMERA_H
#define V4L2CAMERA_H

#include <QObject>
#include <QImage>
#include <QString>
#include <QByteArray>

/**
 * @brief V4L2摄像头初始化与采集类
 *
 * 封装Linux V4L2视频子系统接口，提供摄像头初始化和帧采集功能
 * 适配 i.MX6 CSI 接口的 OV 摄像头
 */
class V4L2Camera : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 图像格式
     */
    enum PixelFormat {
        FORMAT_YUYV,  // YUYV格式
        FORMAT_UYVY,  // UYVY格式
        FORMAT_MJPEG, // MJPEG格式
        FORMAT_JPEG,  // JPEG格式
        FORMAT_RGB565 // RGB565格式
    };

    /**
     * @brief 摄像状态
     */
    enum State {
        StateClosed,   // 已关闭
        StateReady,    // 就绪
        StateStreaming // 正在采集
    };

    explicit V4L2Camera(QObject *parent = nullptr);
    ~V4L2Camera();

    /**
     * @brief 打开摄像头
     * @param device 设备路径，如 /dev/video1
     * @return 成功返回true
     */
    bool open(const QString &device);

    /**
     * @brief 关闭摄像头
     */
    void close();

    /**
     * @brief 设置图像格式
     * @param width 宽度
     * @param height 高度
     * @param format 像素格式
     * @return 成功返回true
     */
    bool setFormat(int width, int height, PixelFormat format = FORMAT_YUYV);

    /**
     * @brief 请求缓冲区
     * @param count 缓冲区数量
     * @return 成功返回true
     */
    bool requestBuffers(int count = 4);

    /**
     * @brief 开始采集
     * @return 成功返回true
     */
    bool startCapture();

    /**
     * @brief 停止采集
     */
    void stopCapture();

    /**
     * @brief 获取一帧图像（阻塞）
     * @param timeoutMs 超时时间（毫秒）
     * @return 图像数据，失败返回空QImage
     */
    QImage grabFrame(int timeoutMs = 1000, QByteArray *rawFrame = nullptr);

    /**
     * @brief 抓拍当前帧并返回原始数据
     * @param timeoutMs 超时时间（毫秒）
     * @return 原始图像数据（用于网络传输）
     */
    QByteArray grabRawFrame(int timeoutMs = 1000);

    /**
     * @brief 获取当前状态
     */
    State state() const;

    /**
     * @brief 获取图像宽度
     */
    int width() const { return m_width; }

    /**
     * @brief 获取图像高度
     */
    int height() const { return m_height; }

    /**
     * @brief 获取实际像素格式名称
     */
    QString pixelFormatName() const;

    /**
     * @brief 获取最后错误信息
     */
    QString lastError() const;

signals:
    /**
     * @brief 帧就绪信号
     */
    void frameReady(const QImage &frame);

private:
    int m_fd;           // 文件描述符
    int m_width;        // 图像宽度
    int m_height;       // 图像高度
    PixelFormat m_format; // 像素格式
    State m_state;      // 当前状态
    QString m_device;   // 设备路径
    QString m_lastError;
    quint32 m_actualFormat; // 实际像素格式（V4L2定义）

    void** m_buffers;   // 内存映射缓冲区
    int m_bufferCount;  // 缓冲区数量

    /**
     * @brief 初始化内存映射
     */
    bool initMmap(int count);

    /**
     * @brief 释放内存映射
     */
    void freeBuffers();

    /**
     * @brief YUYV转RGB888
     */
    QImage yuyvToRgb(const void *data, int width, int height);

    /**
     * @brief UYVY转RGB888
     */
    QImage uyvyToRgb(const void *data, int width, int height);

    /**
     * @brief RGB565转RGB888
     */
    QImage rgb565ToRgb(const void *data, int width, int height);
};

#endif // V4L2CAMERA_H
