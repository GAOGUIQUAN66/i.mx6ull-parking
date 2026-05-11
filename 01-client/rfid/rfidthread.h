#ifndef RFIDTHREAD_H
#define RFIDTHREAD_H

#include <QThread>
#include <QDateTime>

class SerialPort;

/**
 * @brief RFID监听线程
 *
 * 串口持续读取RFID模块输出，解析 "记卡：0008038796@" 格式的卡号并发出信号。
 * 新模块格式：卡号为@之前的10位数字。
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
