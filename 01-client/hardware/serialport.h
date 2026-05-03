#ifndef SERIALPORT_H
#define SERIALPORT_H

#include <QString>
#include <QObject>

/**
 * @brief UART for RFID
 *
 * termios wrapper
 */
class SerialPort : public QObject
{
    Q_OBJECT

public:
    explicit SerialPort(QObject *parent = nullptr);
    ~SerialPort();

    /**
     * @brief Open tty
     * @param device node
     * @param baudRate
     * @return true on success
     */
    bool open(const QString &device, int baudRate = 9600);

    /**
     * @brief Close tty
     */
    void close();

    /**
     * @brief Is open
     */
    bool isOpen() const;

    /**
     * @brief Blocking read
     * @param buffer
     * @param maxSize
     * @param timeoutMs ms，-1表示无限等待
     * @return bytes or -1
     */
    int read(char *buffer, int maxSize, int timeoutMs = -1);

    /**
     * @brief Write
     * @param data
     * @return bytes or -1
     */
    int write(const QByteArray &data);

    /**
     * @brief Last error string
     */
    QString lastError() const;

signals:
    /**
     * @brief readyRead chunk
     * @param data
     */
    void dataReady(const QByteArray &data);

private:
    int m_fd;           // fd
    QString m_device;   // device node
    QString m_lastError;

    /**
     * @brief Apply termios
     * @param baudRate
     * @return true on success
     */
    bool configure(int baudRate);
};

#endif // SERIALPORT_H
