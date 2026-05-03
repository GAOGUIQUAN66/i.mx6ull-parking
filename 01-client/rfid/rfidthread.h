#ifndef RFIDTHREAD_H
#define RFIDTHREAD_H

#include <QThread>
#include <QDateTime>

class SerialPort;

/**
 * @brief RFID reader thread
 *
 * Parses UART frames into 10-digit IDs.
 * ID = last 10 digits before '@'.
 */
class RfidThread : public QThread
{
    Q_OBJECT

public:
    explicit RfidThread(QObject *parent = nullptr);
    ~RfidThread();

    void setSerialPort(SerialPort *serialPort);
    void stop();

signals:
    void cardDetected(const QString &cardId);
    void readError(const QString &error);

protected:
    void run() override;

private:
    QString extractCardId(QString &buffer);

    SerialPort *m_serialPort;
    bool m_running;
    QString m_buffer;
    QString m_lastCardId;
    QDateTime m_lastCardTime;
};

#endif // RFIDTHREAD_H
