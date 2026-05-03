#ifndef NETWORKCLIENT_H
#define NETWORKCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QImage>
#include <QByteArray>
#include <QTimer>
#include <QJsonObject>

/**
 * @brief TCP client
 *
 * Talks to Python LPR host:
 * - uplink frames
 * - JSON results + WAV
 */
class NetworkClient : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Parsed JSON
     */
    struct RecognizeResult {
        QString plateNumber;    // plate
        double confidence;      // score
        bool success;           // ok flag
        QString errorMessage;   // error text
    };

    explicit NetworkClient(QObject *parent = nullptr);
    ~NetworkClient();

    /**
     * @brief connectToHost
     * @param host
     * @param port
     * @return queued ok
     */
    bool connectToServer(const QString &host, quint16 port);

    /**
     * @brief disconnect
     */
    void disconnect();

    /**
     * @brief Send QImage
     * @param image rgb
     * @return queued ok
     */
    bool sendImageForRecognition(const QImage &image);

    /**
     * @brief Send encoded bytes
     * @param data jpeg/bmp
     * @return queued ok
     */
    bool sendRawImage(const QByteArray &data);

    /**
     * @brief Send mmap frame
     * @param data payload
     * @param width
     * @param height
     * @param pixelFormat tag
     * @return queued ok
     */
    bool sendRawFrameForRecognition(const QByteArray &data, int width, int height,
                                    const QString &pixelFormat);

    /**
     * @brief Request TTS WAV
     * @param eventType
     * @param text utterance
     * @param fileName local name
     */
    bool requestAudio(const QString &eventType, const QString &text, const QString &fileName);

    /**
     * @brief Connected
     */
    bool isConnected() const;

    /**
     * @brief Last error string
     */
    QString lastError() const;

signals:
    /**
     * @brief connected
     */
    void connected();

    /**
     * @brief disconnect信号
     */
    void disconnected();

    /**
     * @brief recognizeResultReady
     * @param result struct
     */
    void recognizeResultReady(const RecognizeResult &result);

    /**
     * @brief socket error
     * @param error QString
     */
    void errorOccurred(const QString &error);

    /**
     * @brief WAV chunk
     * @param fileName
     * @param audioData bytes
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
     * @brief sendPacket
     * @param data payload
     * @param type packet kind
     */
    bool sendPacket(const QByteArray &data, quint8 type);

    /**
     * @brief Reassembly
     */
    void parseReceivedData();

    /**
     * @brief reconnect
     */
    void tryReconnect();

    QTcpSocket *m_socket;
    QString m_host;
    quint16 m_port;
    QString m_lastError;

    // RX buffer
    QByteArray m_receiveBuffer;
    quint32 m_expectedSize;

    // reconnect timer
    QTimer *m_reconnectTimer;
    bool m_autoReconnect;
    int m_reconnectInterval;
};

#endif // NETWORKCLIENT_H
