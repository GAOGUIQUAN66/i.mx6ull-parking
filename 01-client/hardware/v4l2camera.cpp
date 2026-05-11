#include "v4l2camera.h"
#include <QDebug>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <errno.h>
#include <string.h>
#include <sys/select.h>
#include <cstring>

// JPEG FourCC 码 (不是标准的 V4L2_PIX_FMT_JPEG)
#define V4L2_PIX_FMT_JPEG_RAW v4l2_fourcc('J', 'P', 'E', 'G')
#define V4L2_PIX_FMT_RGB565_LE v4l2_fourcc('R', 'G', 'B', 'P')

V4L2Camera::V4L2Camera(QObject *parent)
: QObject(parent)
, m_fd(-1)
, m_width(0)
, m_height(0)
, m_format(FORMAT_YUYV)
, m_state(StateClosed)
, m_buffers(nullptr)
, m_bufferCount(0)
{
}

V4L2Camera::~V4L2Camera()
{
    close();
}

bool V4L2Camera::open(const QString &device)
{
    if (m_fd >= 0) {
        close();
    }

    // 打开设备
    m_fd = ::open(device.toUtf8().constData(), O_RDWR | O_NONBLOCK);
    if (m_fd < 0) {
        m_lastError = QString("无法打开摄像头 %1: %2").arg(device).arg(strerror(errno));
        return false;
    }

    // 查询设备能力
    struct v4l2_capability cap;
    memset(&cap, 0, sizeof(cap));
    if (ioctl(m_fd, VIDIOC_QUERYCAP, &cap) < 0) {
        m_lastError = QString("查询设备能力失败: %1").arg(strerror(errno));
        ::close(m_fd);
        m_fd = -1;
        return false;
    }

    // 打印设备信息（调试）
    qDebug() << "摄像头设备:" << device;
    qDebug() << " 驱动:" << (char*)cap.driver;
    qDebug() << " 卡名:" << (char*)cap.card;
    qDebug() << " 总线:" << (char*)cap.bus_info;
    qDebug() << " 版本:" << cap.version;
    qDebug() << " 能力:" << QString("0x%1").arg(cap.capabilities, 0, 16);

    // 检查是否支持视频捕获
    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        m_lastError = QString("设备不支持视频捕获 (能力: 0x%1)").arg(cap.capabilities, 0, 16);
        ::close(m_fd);
        m_fd = -1;
        return false;
    }

    // 检查是否支持流式IO
    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        m_lastError = QString("设备不支持流式IO (能力: 0x%1)").arg(cap.capabilities, 0, 16);
        ::close(m_fd);
        m_fd = -1;
        return false;
    }

    m_device = device;
    m_state = StateReady;
    return true;
}

void V4L2Camera::close()
{
    if (m_state == StateStreaming) {
        stopCapture();
    }

    freeBuffers();

    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }

    m_state = StateClosed;
    m_device.clear();
}

bool V4L2Camera::setFormat(int width, int height, PixelFormat format)
{
    if (m_fd < 0) {
        m_lastError = "摄像头未打开";
        return false;
    }

    // 先枚举摄像头支持的格式（调试）
    struct v4l2_fmtdesc fmtdesc;
    memset(&fmtdesc, 0, sizeof(fmtdesc));
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    qDebug() << "摄像头支持的像素格式:";
    while (ioctl(m_fd, VIDIOC_ENUM_FMT, &fmtdesc) == 0) {
        qDebug() << "  " << QString("%1%2%3%4")
                    .arg(char(fmtdesc.pixelformat & 0xFF))
                    .arg(char((fmtdesc.pixelformat >> 8) & 0xFF))
                    .arg(char((fmtdesc.pixelformat >> 16) & 0xFF))
                    .arg(char((fmtdesc.pixelformat >> 24) & 0xFF))
                 << "-" << (char*)fmtdesc.description;
        fmtdesc.index++;
    }

    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = width;
    fmt.fmt.pix.height = height;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;

    switch (format) {
    case FORMAT_YUYV:
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
        break;
    case FORMAT_UYVY:
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_UYVY;
        break;
    case FORMAT_MJPEG:
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
        break;
    case FORMAT_JPEG:
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_JPEG_RAW;
        break;
    case FORMAT_RGB565:
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB565_LE;
        break;
    }

    if (ioctl(m_fd, VIDIOC_S_FMT, &fmt) < 0) {
        m_lastError = QString("设置格式失败: %1").arg(strerror(errno));
        return false;
    }

    // 保存实际设置的格式
    m_width = fmt.fmt.pix.width;
    m_height = fmt.fmt.pix.height;
    m_format = format;
    m_actualFormat = fmt.fmt.pix.pixelformat;

    // 打印实际格式（调试）
    qDebug() << "实际像素格式:" << QString("%1%2%3%4")
        .arg(char(m_actualFormat & 0xFF))
        .arg(char((m_actualFormat >> 8) & 0xFF))
        .arg(char((m_actualFormat >> 16) & 0xFF))
        .arg(char((m_actualFormat >> 24) & 0xFF));

    return true;
}

bool V4L2Camera::requestBuffers(int count)
{
    if (m_fd < 0) {
        m_lastError = "摄像头未打开";
        return false;
    }

    return initMmap(count);
}

bool V4L2Camera::initMmap(int count)
{
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));

    req.count = count;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(m_fd, VIDIOC_REQBUFS, &req) < 0) {
        m_lastError = QString("请求缓冲区失败: %1").arg(strerror(errno));
        return false;
    }

    if (req.count < 2) {
        m_lastError = "缓冲区数量不足";
        return false;
    }

    m_bufferCount = req.count;
    m_buffers = new void*[m_bufferCount];

    for (int i = 0; i < m_bufferCount; ++i) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (ioctl(m_fd, VIDIOC_QUERYBUF, &buf) < 0) {
            m_lastError = QString("查询缓冲区失败: %1").arg(strerror(errno));
            freeBuffers();
            return false;
        }

        m_buffers[i] = mmap(nullptr, buf.length, PROT_READ | PROT_WRITE,
            MAP_SHARED, m_fd, buf.m.offset);

        if (m_buffers[i] == MAP_FAILED) {
            m_lastError = QString("内存映射失败: %1").arg(strerror(errno));
            freeBuffers();
            return false;
        }
    }

    return true;
}

void V4L2Camera::freeBuffers()
{
    if (m_buffers) {
        for (int i = 0; i < m_bufferCount; ++i) {
            if (m_buffers[i] && m_buffers[i] != MAP_FAILED) {
                struct v4l2_buffer buf;
                memset(&buf, 0, sizeof(buf));
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                buf.index = i;
                if (ioctl(m_fd, VIDIOC_QUERYBUF, &buf) == 0) {
                    munmap(m_buffers[i], buf.length);
                }
            }
        }
        delete[] m_buffers;
        m_buffers = nullptr;
    }
    m_bufferCount = 0;
}

bool V4L2Camera::startCapture()
{
    if (m_fd < 0) {
        m_lastError = "摄像头未打开";
        return false;
    }

    for (int i = 0; i < m_bufferCount; ++i) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (ioctl(m_fd, VIDIOC_QBUF, &buf) < 0) {
            m_lastError = QString("入队缓冲区失败: %1").arg(strerror(errno));
            return false;
        }
    }

    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(m_fd, VIDIOC_STREAMON, &type) < 0) {
        m_lastError = QString("启动流失败: %1").arg(strerror(errno));
        return false;
    }

    m_state = StateStreaming;
    return true;
}

void V4L2Camera::stopCapture()
{
    if (m_fd >= 0) {
        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ioctl(m_fd, VIDIOC_STREAMOFF, &type);
    }
    m_state = StateReady;
}

QImage V4L2Camera::grabFrame(int timeoutMs, QByteArray *rawFrame)
{
    QByteArray raw = grabRawFrame(timeoutMs);
    if (raw.isEmpty()) {
        return QImage();
    }

    if (rawFrame) {
        *rawFrame = raw;
    }

    // 根据实际像素格式转换
    quint32 fmt = m_actualFormat;

    // JPEG格式 (JPEG 或 MJPG)
    if (fmt == V4L2_PIX_FMT_JPEG_RAW || fmt == V4L2_PIX_FMT_MJPEG ||
        fmt == v4l2_fourcc('J', 'P', 'E', 'G') || fmt == v4l2_fourcc('J', 'P', 'G', '4')) {
        QImage image = QImage::fromData(raw);
        if (image.isNull()) {
            m_lastError = QString("JPEG帧解码失败，帧大小=%1").arg(raw.size());
            qDebug() << "V4L2Camera:" << m_lastError;
        }
        return image;
    }

    // RGB565格式
    if (fmt == v4l2_fourcc('R', 'G', 'B', 'P')) {
        return rgb565ToRgb(raw.constData(), m_width, m_height);
    }

    // YUYV格式
    if (fmt == V4L2_PIX_FMT_YUYV) {
        return yuyvToRgb(raw.constData(), m_width, m_height);
    }

    // UYVY格式
    if (fmt == V4L2_PIX_FMT_UYVY) {
        return uyvyToRgb(raw.constData(), m_width, m_height);
    }

    // 默认尝试按设置格式转换
    switch (m_format) {
    case FORMAT_YUYV:
        return yuyvToRgb(raw.constData(), m_width, m_height);
    case FORMAT_UYVY:
        return uyvyToRgb(raw.constData(), m_width, m_height);
    case FORMAT_RGB565:
        return rgb565ToRgb(raw.constData(), m_width, m_height);
    case FORMAT_JPEG:
    case FORMAT_MJPEG: {
        QImage image = QImage::fromData(raw);
        if (image.isNull()) {
            m_lastError = QString("JPEG帧解码失败，帧大小=%1").arg(raw.size());
            qDebug() << "V4L2Camera:" << m_lastError;
        }
        return image;
    }
    }

    return QImage::fromData(raw);
}

QByteArray V4L2Camera::grabRawFrame(int timeoutMs)
{
    if (m_fd < 0 || m_state != StateStreaming) {
        m_lastError = "摄像头未在采集状态";
        return QByteArray();
    }

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(m_fd, &fds);

    struct timeval tv;
    tv.tv_sec = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;

    int ret = select(m_fd + 1, &fds, nullptr, nullptr, &tv);
    if (ret < 0) {
        m_lastError = QString("select失败: %1").arg(strerror(errno));
        return QByteArray();
    }
    if (ret == 0) {
        m_lastError = "获取帧超时";
        return QByteArray();
    }

    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (ioctl(m_fd, VIDIOC_DQBUF, &buf) < 0) {
        m_lastError = QString("出队缓冲区失败: %1").arg(strerror(errno));
        return QByteArray();
    }

    QByteArray data((const char*)m_buffers[buf.index], buf.bytesused);

    if (ioctl(m_fd, VIDIOC_QBUF, &buf) < 0) {
        m_lastError = QString("重新入队失败: %1").arg(strerror(errno));
    }

    return data;
}

QImage V4L2Camera::yuyvToRgb(const void *data, int width, int height)
{
    QImage image(width, height, QImage::Format_RGB888);

    const unsigned char *yuyv = (const unsigned char *)data;
    unsigned char *rgb = image.bits();

    for (int i = 0; i < width * height; i += 2) {
        int y0 = yuyv[i * 2];
        int u = yuyv[i * 2 + 1];
        int y1 = yuyv[i * 2 + 2];
        int v = yuyv[i * 2 + 3];

        int d = u - 128;
        int e = v - 128;

        int r0 = y0 + ((359 * e) >> 8);
        int g0 = y0 - (((88 * d) + (183 * e)) >> 8);
        int b0 = y0 + ((454 * d) >> 8);

        int r1 = y1 + ((359 * e) >> 8);
        int g1 = y1 - (((88 * d) + (183 * e)) >> 8);
        int b1 = y1 + ((454 * d) >> 8);

        r0 = qBound(0, r0, 255);
        g0 = qBound(0, g0, 255);
        b0 = qBound(0, b0, 255);
        r1 = qBound(0, r1, 255);
        g1 = qBound(0, g1, 255);
        b1 = qBound(0, b1, 255);

        rgb[i * 3] = r0;
        rgb[i * 3 + 1] = g0;
        rgb[i * 3 + 2] = b0;
        rgb[(i + 1) * 3] = r1;
        rgb[(i + 1) * 3 + 1] = g1;
        rgb[(i + 1) * 3 + 2] = b1;
    }

    return image;
}

QImage V4L2Camera::uyvyToRgb(const void *data, int width, int height)
{
    QImage image(width, height, QImage::Format_RGB888);

    const unsigned char *uyvy = (const unsigned char *)data;
    unsigned char *rgb = image.bits();

    for (int i = 0; i < width * height; i += 2) {
        int u = uyvy[i * 2];
        int y0 = uyvy[i * 2 + 1];
        int v = uyvy[i * 2 + 2];
        int y1 = uyvy[i * 2 + 3];

        int d = u - 128;
        int e = v - 128;

        int r0 = y0 + ((359 * e) >> 8);
        int g0 = y0 - (((88 * d) + (183 * e)) >> 8);
        int b0 = y0 + ((454 * d) >> 8);

        int r1 = y1 + ((359 * e) >> 8);
        int g1 = y1 - (((88 * d) + (183 * e)) >> 8);
        int b1 = y1 + ((454 * d) >> 8);

        r0 = qBound(0, r0, 255);
        g0 = qBound(0, g0, 255);
        b0 = qBound(0, b0, 255);
        r1 = qBound(0, r1, 255);
        g1 = qBound(0, g1, 255);
        b1 = qBound(0, b1, 255);

        rgb[i * 3] = r0;
        rgb[i * 3 + 1] = g0;
        rgb[i * 3 + 2] = b0;
        rgb[(i + 1) * 3] = r1;
        rgb[(i + 1) * 3 + 1] = g1;
        rgb[(i + 1) * 3 + 2] = b1;
    }

    return image;
}

QImage V4L2Camera::rgb565ToRgb(const void *data, int width, int height)
{
    QImage image(width, height, QImage::Format_RGB888);

    const unsigned short *rgb565 = (const unsigned short *)data;
    unsigned char *rgb = image.bits();

    for (int i = 0; i < width * height; ++i) {
        unsigned short pixel = rgb565[i];
        // RGB565: R(5位) G(6位) B(5位)
        int r = (pixel >> 11) & 0x1F;  // 5位
        int g = (pixel >> 5) & 0x3F;   // 6位
        int b = pixel & 0x1F;          // 5位

        // 扩展到8位
        r = (r << 3) | (r >> 2);  // 5位 -> 8位
        g = (g << 2) | (g >> 4);  // 6位 -> 8位
        b = (b << 3) | (b >> 2);  // 5位 -> 8位

        rgb[i * 3] = r;
        rgb[i * 3 + 1] = g;
        rgb[i * 3 + 2] = b;
    }

    return image;
}

V4L2Camera::State V4L2Camera::state() const
{
    return m_state;
}

QString V4L2Camera::lastError() const
{
    return m_lastError;
}

QString V4L2Camera::pixelFormatName() const
{
    if (m_actualFormat == 0) {
        return QString();
    }
    return QString("%1%2%3%4")
        .arg(char(m_actualFormat & 0xFF))
        .arg(char((m_actualFormat >> 8) & 0xFF))
        .arg(char((m_actualFormat >> 16) & 0xFF))
        .arg(char((m_actualFormat >> 24) & 0xFF));
}
