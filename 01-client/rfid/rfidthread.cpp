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
        emit readError("RFID serial not open");
        return;
    }

    m_running = true;
    qDebug() << "RfidThread: listening";

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
            qDebug() << "RfidThread: card" << cardId;
            emit cardDetected(cardId);
        }

        if (m_buffer.size() > 64) {
            m_buffer = m_buffer.right(16);
        }
    }

    qDebug() << "RfidThread: stop";
}

QString RfidThread::extractCardId(QString &buffer)
{
    // Frame e.g. prefix + 10 digits + @
    // Take last 10 digits before @
    int atPos = buffer.indexOf('@');
    if (atPos < 0) {
        return QString();
    }

    // Before @
    QString dataBeforeAt = buffer.left(atPos);

    // Digits
    QString digits;
    for (int i = dataBeforeAt.size() - 1; i >= 0 && digits.size() < 10; --i) {
        if (dataBeforeAt.at(i).isDigit()) {
            digits.prepend(dataBeforeAt.at(i));
        }
    }

    // Consume
    buffer.remove(0, atPos + 1);

    // 10-digit id
    if (digits.length() == 10) {
        return digits;
    }

    return QString();
}
