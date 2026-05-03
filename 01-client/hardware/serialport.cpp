#include "serialport.h"
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
#include <sys/select.h>

SerialPort::SerialPort(QObject *parent)
    : QObject(parent)
    , m_fd(-1)
{
}

SerialPort::~SerialPort()
{
    close();
}

bool SerialPort::open(const QString &device, int baudRate)
{
    if (m_fd >= 0) {
        close();
    }

    // Open tty
    m_fd = ::open(device.toUtf8().constData(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (m_fd < 0) {
        m_lastError = QString("Cannot open serial %1: %2").arg(device).arg(strerror(errno));
        return false;
    }

    // Blocking mode
    int flags = fcntl(m_fd, F_GETFL, 0);
    fcntl(m_fd, F_SETFL, flags & ~O_NONBLOCK);

    m_device = device;

    if (!configure(baudRate)) {
        close();
        return false;
    }

    return true;
}

void SerialPort::close()
{
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }
    m_device.clear();
}

bool SerialPort::isOpen() const
{
    return m_fd >= 0;
}

bool SerialPort::configure(int baudRate)
{
    struct termios options;

    if (tcgetattr(m_fd, &options) != 0) {
        m_lastError = QString("tcgetattr failed: %1").arg(strerror(errno));
        return false;
    }

    // Baud
    speed_t speed;
    switch (baudRate) {
    case 1200:   speed = B1200; break;
    case 2400:   speed = B2400; break;
    case 4800:   speed = B4800; break;
    case 9600:   speed = B9600; break;
    case 19200:  speed = B19200; break;
    case 38400:  speed = B38400; break;
    case 57600:  speed = B57600; break;
    case 115200: speed = B115200; break;
    default:     speed = B9600; break;
    }

    cfsetispeed(&options, speed);
    cfsetospeed(&options, speed);

    // 8 data bits
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;

    // no parity
    options.c_cflag &= ~PARENB;

    // 1 stop
    options.c_cflag &= ~CSTOPB;

    // RX on
    options.c_cflag |= (CLOCAL | CREAD);

    // no flow ctrl
    options.c_cflag &= ~CRTSCTS;

    // raw
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_oflag &= ~OPOST;

    // no sw flow
    options.c_iflag &= ~(IXON | IXOFF | IXANY);
    options.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);

    // timeout VMIN=0 VTIME=1
    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 1;

    if (tcsetattr(m_fd, TCSANOW, &options) != 0) {
        m_lastError = QString("tcsetattr failed: %1").arg(strerror(errno));
        return false;
    }

    tcflush(m_fd, TCIOFLUSH);
    return true;
}

int SerialPort::read(char *buffer, int maxSize, int timeoutMs)
{
    if (m_fd < 0 || !buffer || maxSize <= 0) {
        return -1;
    }

    if (timeoutMs >= 0) {
        // select()
        fd_set readFds;
        FD_ZERO(&readFds);
        FD_SET(m_fd, &readFds);

        struct timeval tv;
        tv.tv_sec = timeoutMs / 1000;
        tv.tv_usec = (timeoutMs % 1000) * 1000;

        int ret = select(m_fd + 1, &readFds, nullptr, nullptr, &tv);
        if (ret <= 0) {
            return ret;  // timeout/err
        }
    }

    int bytesRead = ::read(m_fd, buffer, maxSize);
    if (bytesRead < 0) {
        m_lastError = QString("serial read failed: %1").arg(strerror(errno));
    }

    return bytesRead;
}

int SerialPort::write(const QByteArray &data)
{
    if (m_fd < 0 || data.isEmpty()) {
        return -1;
    }

    int bytesWritten = ::write(m_fd, data.constData(), data.size());
    if (bytesWritten < 0) {
        m_lastError = QString("serial write failed: %1").arg(strerror(errno));
    }

    return bytesWritten;
}

QString SerialPort::lastError() const
{
    return m_lastError;
}
