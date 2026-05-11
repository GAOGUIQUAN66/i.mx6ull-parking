#include "networkclient.h"
#include <QDebug>
#include <QBuffer>
#include <QDataStream>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

// 协议定义
#define PACKET_HEADER_SIZE 5  // 4字节长度 + 1字节类型
#define PACKET_TYPE_IMAGE 1
#define PACKET_TYPE_TEXT 2
#define PACKET_TYPE_RESULT 3
#define PACKET_TYPE_RAW_IMAGE 4
#define PACKET_TYPE_AUDIO 5

NetworkClient::NetworkClient(QObject *parent)
    : QObject(parent)
    , m_socket(nullptr)
    , m_port(0)
    , m_expectedSize(0)
    , m_autoReconnect(true)
    , m_reconnectInterval(3000)
{
    m_socket = new QTcpSocket(this);
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(true);

    connect(m_socket, &QTcpSocket::connected, this, &NetworkClient::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &NetworkClient::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &NetworkClient::onReadyRead);
    // Qt 5.7 兼容：使用 error 信号而非 errorOccurred
    connect(m_socket, static_cast<void(QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error),
            this, &NetworkClient::onError);
    connect(m_reconnectTimer, &QTimer::timeout, this, &NetworkClient::onReconnectTimer);
}

NetworkClient::~NetworkClient()
{
    disconnect();
}

bool NetworkClient::connectToServer(const QString &host, quint16 port)
{
    m_host = host;
    m_port = port;

    qDebug() << "NetworkClient: 正在连接服务器" << host << ":" << port;

    m_socket->connectToHost(host, port);
    return true;
}

void NetworkClient::disconnect()
{
    m_autoReconnect = false;
    m_reconnectTimer->stop();

    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->disconnectFromHost();
    }
}

bool NetworkClient::sendImageForRecognition(const QImage &image)
{
    if (!isConnected()) {
        m_lastError = "未连接到服务器";
        return false;
    }

    QByteArray imageData;
    QBuffer buffer(&imageData);
    buffer.open(QIODevice::WriteOnly);

    QString encodedFormat = "JPEG";
    if (!image.save(&buffer, "JPEG", 90)) {
        buffer.close();
        imageData.clear();

        if (!buffer.open(QIODevice::WriteOnly)) {
            m_lastError = "图像缓冲区打开失败";
            return false;
        }

        encodedFormat = "BMP";
        if (!image.save(&buffer, "BMP")) {
            m_lastError = "图像编码失败(JPEG/BMP均失败)";
            return false;
        }
    }
    buffer.close();

    qDebug() << "NetworkClient: 图像编码成功, 格式:" << encodedFormat
             << "大小:" << imageData.size() << "字节";

    return sendRawImage(imageData);
}

bool NetworkClient::sendRawImage(const QByteArray &data)
{
    return sendPacket(data, PACKET_TYPE_IMAGE);
}

bool NetworkClient::sendRawFrameForRecognition(const QByteArray &data, int width, int height,
                                               const QString &pixelFormat)
{
    if (!isConnected()) {
        m_lastError = "未连接到服务器";
        return false;
    }

    if (data.isEmpty() || width <= 0 || height <= 0 || pixelFormat.isEmpty()) {
        m_lastError = "原始帧参数无效";
        return false;
    }

    QByteArray payload;
    QDataStream stream(&payload, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);

    QByteArray formatBytes = pixelFormat.toUtf8();
    stream << quint32(width);
    stream << quint32(height);
    stream << quint32(formatBytes.size());
    payload.append(formatBytes);
    payload.append(data);

    qDebug() << "NetworkClient: 发送原始帧"
             << "格式:" << pixelFormat
             << "尺寸:" << width << "x" << height
             << "数据大小:" << data.size() << "字节";

    return sendPacket(payload, PACKET_TYPE_RAW_IMAGE);
}

bool NetworkClient::requestAudio(const QString &eventType, const QString &text, const QString &fileName)
{
    if (!isConnected()) {
        m_lastError = "未连接到服务器";
        return false;
    }

    if (eventType.isEmpty() || text.isEmpty() || fileName.isEmpty()) {
        m_lastError = "语音请求参数无效";
        return false;
    }

    QJsonObject request;
    request.insert("type", "tts");
    request.insert("event", eventType);
    request.insert("text", text);
    request.insert("file_name", fileName);

    QByteArray payload = QJsonDocument(request).toJson(QJsonDocument::Compact);
    qDebug() << "NetworkClient: 请求语音播报"
             << "event:" << eventType
             << "file:" << fileName
             << "text:" << text;
    return sendPacket(payload, PACKET_TYPE_TEXT);
}

bool NetworkClient::sendPacket(const QByteArray &data, quint8 type)
{
    if (!isConnected()) {
        m_lastError = "未连接到服务器";
        return false;
    }

    // 构造数据包：4字节长度 + 1字节类型 + 数据
    QByteArray packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);

    quint32 dataSize = data.size();
    stream << dataSize;          // 4字节长度
    packet.append((char)type);   // 1字节类型
    packet.append(data);         // 数据内容

    qint64 written = m_socket->write(packet);
    if (written != packet.size()) {
        m_lastError = "发送数据不完整";
        return false;
    }

    m_socket->flush();
    qDebug() << "NetworkClient: 发送数据包" << dataSize << "字节, 类型:" << type;
    return true;
}

bool NetworkClient::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

QString NetworkClient::lastError() const
{
    return m_lastError;
}

void NetworkClient::onConnected()
{
    m_autoReconnect = true;
    m_receiveBuffer.clear();
    m_expectedSize = 0;
    qDebug() << "NetworkClient: 已连接到服务器";
    emit connected();
}

void NetworkClient::onDisconnected()
{
    qDebug() << "NetworkClient: 与服务器断开连接";
    emit disconnected();

    // 自动重连
    if (m_autoReconnect && !m_host.isEmpty() && m_port > 0) {
        tryReconnect();
    }
}

void NetworkClient::onReadyRead()
{
    m_receiveBuffer.append(m_socket->readAll());
    parseReceivedData();
}

void NetworkClient::onError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    m_lastError = m_socket->errorString();
    qDebug() << "NetworkClient: 网络错误:" << m_lastError;
    emit errorOccurred(m_lastError);
}

void NetworkClient::onReconnectTimer()
{
    if (!isConnected() && m_autoReconnect) {
        qDebug() << "NetworkClient: 尝试重连...";
        m_socket->connectToHost(m_host, m_port);
    }
}

void NetworkClient::parseReceivedData()
{
    while (m_receiveBuffer.size() >= PACKET_HEADER_SIZE) {
        // 解析包头
        QDataStream stream(m_receiveBuffer);
        stream.setByteOrder(QDataStream::BigEndian);

        quint32 dataSize;
        quint8 packetType;
        stream >> dataSize;
        packetType = (quint8)m_receiveBuffer.at(4);

        // 检查是否收到完整数据包
        if (m_receiveBuffer.size() < (int)(PACKET_HEADER_SIZE + dataSize)) {
            break;  // 数据不完整，等待更多数据
        }

        // 提取数据内容
        QByteArray content = m_receiveBuffer.mid(PACKET_HEADER_SIZE, dataSize);

        // 移除已处理的数据
        m_receiveBuffer.remove(0, PACKET_HEADER_SIZE + dataSize);

        // 处理识别结果
        if (packetType == PACKET_TYPE_RESULT) {
            // 解析JSON格式结果
            QString jsonStr = QString::fromUtf8(content);
            qDebug() << "NetworkClient: 收到识别结果:" << jsonStr;

            RecognizeResult result;
            result.success = jsonStr.contains("\"success\":true") ||
                              jsonStr.contains("\"success\": true");

            // 简单解析JSON（避免依赖QJsonDocument）
            QRegularExpression plateRegex("\"plate_number\"\\s*:\\s*\"([^\"]+)\"");
            QRegularExpression confRegex("\"confidence\"\\s*:\\s*([0-9.]+)");
            QRegularExpression errorRegex("\"error\"\\s*:\\s*\"([^\"]+)\"");

            QRegularExpressionMatch match = plateRegex.match(jsonStr);
            if (match.hasMatch()) {
                result.plateNumber = match.captured(1);
            }

            match = confRegex.match(jsonStr);
            if (match.hasMatch()) {
                result.confidence = match.captured(1).toDouble();
            }

            match = errorRegex.match(jsonStr);
            if (match.hasMatch()) {
                result.errorMessage = match.captured(1);
            }

            emit recognizeResultReady(result);
        } else if (packetType == PACKET_TYPE_AUDIO) {
            if (content.size() < 4) {
                qDebug() << "NetworkClient: 音频数据包过短";
                continue;
            }

            QDataStream audioStream(content);
            audioStream.setByteOrder(QDataStream::BigEndian);

            quint32 headerSize = 0;
            audioStream >> headerSize;
            if (content.size() < 4 + static_cast<int>(headerSize)) {
                qDebug() << "NetworkClient: 音频头不完整";
                continue;
            }

            QByteArray headerBytes = content.mid(4, headerSize);
            QByteArray audioData = content.mid(4 + headerSize);
            QJsonDocument jsonDoc = QJsonDocument::fromJson(headerBytes);
            QJsonObject headerObj = jsonDoc.object();
            QString fileName = headerObj.value("file_name").toString();
            if (fileName.isEmpty()) {
                fileName = QString("audio_%1.wav").arg(QDateTime::currentMSecsSinceEpoch());
            }

            qDebug() << "NetworkClient: 收到语音文件" << fileName << "大小:" << audioData.size();
            emit audioFileReady(fileName, audioData);
        }
    }
}

void NetworkClient::tryReconnect()
{
    if (!m_reconnectTimer->isActive()) {
        qDebug() << "NetworkClient:" << m_reconnectInterval / 1000 << "秒后尝试重连";
        m_reconnectTimer->start(m_reconnectInterval);
    }
}
