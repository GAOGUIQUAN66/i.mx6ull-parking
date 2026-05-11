#ifndef SERIALPORT_H
#define SERIALPORT_H

#include <QString>
#include <QObject>

/**
 * @brief 串口初始化与通信类（用于RFID模块）
 *
 * 封装Linux串口操作，提供初始化、读写功能
 */
class SerialPort : public QObject
{
    Q_OBJECT

public:
    explicit SerialPort(QObject *parent = nullptr);
    ~SerialPort();

    /**
     * @brief 打开串口
     * @param device 串口设备路径，如 /dev/ttyUSB0
     * @param baudRate 波特率，默认9600
     * @return 成功返回true
     */
    bool open(const QString &device, int baudRate = 9600);

    /**
     * @brief 关闭串口
     */
    void close();

    /**
     * @brief 检查串口是否已打开
     */
    bool isOpen() const;

    /**
     * @brief 读取数据（阻塞）
     * @param buffer 数据缓冲区
     * @param maxSize 最大读取字节数
     * @param timeoutMs 超时时间（毫秒），-1表示无限等待
     * @return 实际读取字节数，-1表示错误
     */
    int read(char *buffer, int maxSize, int timeoutMs = -1);

    /**
     * @brief 写入数据
     * @param data 要写入的数据
     * @return 实际写入字节数，-1表示错误
     */
    int write(const QByteArray &data);

    /**
     * @brief 获取最后错误信息
     */
    QString lastError() const;

signals:
    /**
     * @brief 数据就绪信号
     * @param data 接收到的数据
     */
    void dataReady(const QByteArray &data);

private:
    int m_fd;           // 文件描述符
    QString m_device;   // 设备路径
    QString m_lastError;

    /**
     * @brief 配置串口参数
     * @param baudRate 波特率
     * @return 成功返回true
     */
    bool configure(int baudRate);
};

#endif // SERIALPORT_H
