#include "rfidthread.h"
#include "../hardware/serialport.h"

#include <QDebug>
#include <QRegExp>

RfidThread::RfidThread(QObject *parent)
    : QThread(parent)
    , m_serialPort(nullptr)
    , m_running(false)
{
}

RfidThread::~RfidThread()
{
    stop();
    wait();
}

void RfidThread::setSerialPort(SerialPort *serialPort)
{
    m_serialPort = serialPort;
}

void RfidThread::stop()
{
    m_running = false;
}

void RfidThread::run()
{
    if (!m_serialPort || !m_serialPort->isOpen()) {
        emit readError("RFID串口未打开");
        return;
    }

    m_running = true;
    qDebug() << "RfidThread: 开始监听RFID串口";

    while (m_running) {
        char buffer[64] = {0};
        int bytesRead = m_serialPort->read(buffer, sizeof(buffer), 200);
        if (bytesRead < 0) {
            emit readError(m_serialPort->lastError());
            msleep(200);
            continue;
        }

        if (bytesRead == 0) {
            continue;
        }

        m_buffer.append(QString::fromUtf8(buffer, bytesRead));

        while (true) {
            QString cardId = extractCardId(m_buffer);
            if (cardId.isEmpty()) {
                break;
            }

            QDateTime now = QDateTime::currentDateTime();
            if (cardId == m_lastCardId && m_lastCardTime.isValid() &&
                m_lastCardTime.msecsTo(now) < 1500) {
                continue;
            }

            m_lastCardId = cardId;
            m_lastCardTime = now;
            qDebug() << "RfidThread: 检测到卡号" << cardId;
            emit cardDetected(cardId);
        }

        if (m_buffer.size() > 64) {
            m_buffer = m_buffer.right(16);
        }
    }

    qDebug() << "RfidThread: 停止监听";
}

QString RfidThread::extractCardId(QString &buffer)
{
    // 模块格式: "记卡：0008038796@" —— 必须是 @ 前紧邻的 10 位数字，避免从乱码里拼出假卡号
    static const QRegExp kCardPattern(QStringLiteral("(\\d{10})@"));
    const int pos = kCardPattern.indexIn(buffer);
    if (pos < 0) {
        return QString();
    }

    const QString cardId = kCardPattern.cap(1);
    buffer.remove(0, pos + kCardPattern.matchedLength());
    return cardId;
}
