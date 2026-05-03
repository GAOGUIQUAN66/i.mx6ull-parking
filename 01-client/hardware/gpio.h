#ifndef BEEPER_H
#define BEEPER_H

#include <QObject>
#include <QString>

/**
 * @brief Beeper via sysfs LED
 *
 * sysfs brightness toggle
 * Default /sys/class/leds/beep/brightness
 */
class Beeper : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief On/off
     */
    enum State {
        StateOff = 0,   // off
        StateOn = 1     // on
    };

    explicit Beeper(QObject *parent = nullptr);
    ~Beeper();

    /**
     * @brief Init beeper
     * @param ledPath sysfs node
     * @return true on success
     */
    bool init(const QString &ledPath = "/sys/class/leds/beep/brightness");

    /**
     * @brief Set brightness
     * @param state on/off
     * @return true on success
     */
    bool setState(State state);

    /**
     * @brief On
     */
    bool on();

    /**
     * @brief Off
     */
    bool off();

    /**
     * @brief Pulse
     * @param durationMs
     */
    void beep(int durationMs = 200);

    /**
     * @brief Alarm bursts
     * @param count pulses
     * @param intervalMs gap
     */
    void alarm(int count = 3, int intervalMs = 200);

    /**
     * @brief Ready flag
     */
    bool isInitialized() const;

    /**
     * @brief Last error string
     */
    QString lastError() const;

private:
    QString m_ledPath;
    bool m_initialized;
    QString m_lastError;

    /**
     * @brief sysfs write
     */
    bool writeBrightness(int value);
};

#endif // GPIO_H
