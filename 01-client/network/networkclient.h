#ifndef NETWORKCLIENT_H
#define NETWORKCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QImage>
#include <QByteArray>
#include <QTimer>
#include <QJsonObject>

/**
 * @brief 网络通信客户端
 *
 * 负责与Ubuntu上位机的TCP通信：
 * - 发送抓拍图像
 * - 接收车牌识别结果
 */
class NetworkClient : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 识别结果结构体
     */
    struct RecognizeResult {
        QString plateNumber;    // 车牌号
        double confidence;      // 置信度
        bool success;           // 是否识别成功
        QString errorMessage;   // 错误信息
    };

    explicit NetworkClient(QObject *parent = nullptr);
    ~NetworkClient();

    /**
     * @brief 连接到服务器
     * @param host 服务器地址
     * @param port 端口号
     * @return 是否发起连接成功
     */
    bool connectToServer(const QString &host, quint16 port);

    /**
     * @brief 断开连接
     */
    void disconnect();

    /**
     * @brief 发送图像进行识别
     * @param image 图像数据
     * @return 是否发送成功
     */
    bool sendImageForRecognition(const QImage &image);

    /**
     * @brief 发送原始图像数据进行识别
     * @param data 原始图像数据（JPEG格式）
     * @return 是否发送成功
     */
    bool sendRawImage(const QByteArray &data);

    /**
     * @brief 发送V4L2原始帧进行识别
     * @param data 原始图像缓冲
     * @param width 图像宽度
     * @param height 图像高度
     * @param pixelFormat 实际像素格式名，如 RGBP/YUYV
     * @return 是否发送成功
     */
    bool sendRawFrameForRecognition(const QByteArray &data, int width, int height,
                                    const QString &pixelFormat);

    /**
     * @brief 请求服务器生成并下发语音WAV
     * @param eventType 事件类型，如 entry/exit_wait_card
     * @param text 需要播报的文本
     * @param fileName 建议保存的文件名
     */
    bool requestAudio(const QString &eventType, const QString &text, const QString &fileName);

    /**
     * @brief 是否已连接
     */
    bool isConnected() const;

    /**
     * @brief 获取最后错误信息
     */
    QString lastError() const;

signals:
    /**
     * @brief 连接成功信号
     */
    void connected();

    /**
     * @brief 断开连接信号
     */
    void disconnected();

    /**
     * @brief 识别结果就绪
     * @param result 识别结果
     */
    void recognizeResultReady(const RecognizeResult &result);

    /**
     * @brief 发生错误
     * @param error 错误信息
     */
    void errorOccurred(const QString &error);

    /**
     * @brief 收到语音文件
     * @param fileName 建议保存名
     * @param audioData wav字节流
     */
    void audioFileReady(const QString &fileName, const QByteArray &audioData);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError socketError);
    void onReconnectTimer();

private:
    /**
     * @brief 发送数据包
     * @param data 数据内容
     * @param type 数据类型（1=编码图像，2=文本，4=原始图像）
     */
    bool sendPacket(const QByteArray &data, quint8 type);

    /**
     * @brief 解析接收的数据
     */
    void parseReceivedData();

    /**
     * @brief 尝试重连
     */
    void tryReconnect();

    QTcpSocket *m_socket;
    QString m_host;
    quint16 m_port;
    QString m_lastError;

    // 接收缓冲区
    QByteArray m_receiveBuffer;
    quint32 m_expectedSize;

    // 重连机制
    QTimer *m_reconnectTimer;
    bool m_autoReconnect;
    int m_reconnectInterval;
};

#endif // NETWORKCLIENT_H
