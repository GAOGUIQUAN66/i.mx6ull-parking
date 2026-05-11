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
    // 新模块格式: "记卡：0008038796@"，卡号为@之前的10位数字
    // 简化逻辑：找到@，提取@之前最后10个数字
    int atPos = buffer.indexOf('@');
    if (atPos < 0) {
        return QString();
    }

    // 提取@之前的所有内容
    QString dataBeforeAt = buffer.left(atPos);

    // 提取所有数字
    QString digits;
    for (int i = dataBeforeAt.size() - 1; i >= 0 && digits.size() < 10; --i) {
        if (dataBeforeAt.at(i).isDigit()) {
            digits.prepend(dataBeforeAt.at(i));
        }
    }

    // 清空已处理的数据
    buffer.remove(0, atPos + 1);

    // 验证卡号格式（10位数字）
    if (digits.length() == 10) {
        return digits;
    }

    return QString();
}
