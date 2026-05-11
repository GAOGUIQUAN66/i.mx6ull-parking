#ifndef BEEPER_H
#define BEEPER_H

#include <QObject>
#include <QString>

/**
 * @brief 蜂鸣器控制类（使用Linux LED子系统）
 *
 * 通过sysfs led接口控制蜂鸣器
 * 设备节点：/sys/class/leds/beep/brightness
 */
class Beeper : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 蜂鸣器状态
     */
    enum State {
        StateOff = 0,   // 关闭
        StateOn = 1     // 开启
    };

    explicit Beeper(QObject *parent = nullptr);
    ~Beeper();

    /**
     * @brief 初始化蜂鸣器
     * @param ledPath LED设备路径，默认 /sys/class/leds/beep/brightness
     * @return 成功返回true
     */
    bool init(const QString &ledPath = "/sys/class/leds/beep/brightness");

    /**
     * @brief 设置蜂鸣器状态
     * @param state 状态
     * @return 成功返回true
     */
    bool setState(State state);

    /**
     * @brief 开启蜂鸣器
     */
    bool on();

    /**
     * @brief 关闭蜂鸣器
     */
    bool off();

    /**
     * @brief 蜂鸣器鸣叫指定时长
     * @param durationMs 持续时间（毫秒）
     */
    void beep(int durationMs = 200);

    /**
     * @brief 报警模式（多次鸣叫）
     * @param count 鸣叫次数
     * @param intervalMs 间隔时间（毫秒）
     */
    void alarm(int count = 3, int intervalMs = 200);

    /**
     * @brief 检查是否已初始化
     */
    bool isInitialized() const;

    /**
     * @brief 获取最后错误信息
     */
    QString lastError() const;

private:
    QString m_ledPath;
    bool m_initialized;
    QString m_lastError;

    /**
     * @brief 写入brightness文件
     */
    bool writeBrightness(int value);
};

#endif // GPIO_H
