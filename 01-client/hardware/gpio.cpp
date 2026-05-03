#include "gpio.h"
#include <QFile>
#include <QTextStream>
#include <QThread>

Beeper::Beeper(QObject *parent)
    : QObject(parent)
    , m_initialized(false)
{
}

Beeper::~Beeper()
{
    off();
}

bool Beeper::init(const QString &ledPath)
{
    m_ledPath = ledPath;

    QFile file(m_ledPath);
    if (!file.exists()) {
        m_lastError = QString("Beeper missing: %1").arg(m_ledPath);
        return false;
    }

    m_initialized = true;

    // Start off
    off();

    return true;
}

bool Beeper::setState(State state)
{
    if (!m_initialized) {
        m_lastError = "Beeper not init";
        return false;
    }

    return writeBrightness(state);
}

bool Beeper::on()
{
    return setState(StateOn);
}

bool Beeper::off()
{
    return setState(StateOff);
}

void Beeper::beep(int durationMs)
{
    if (!m_initialized) return;

    on();
    QThread::msleep(durationMs);
    off();
}

void Beeper::alarm(int count, int intervalMs)
{
    if (!m_initialized) return;

    for (int i = 0; i < count; ++i) {
        on();
        QThread::msleep(intervalMs);
        off();
        if (i < count - 1) {
            QThread::msleep(intervalMs);
        }
    }
}

bool Beeper::isInitialized() const
{
    return m_initialized;
}

QString Beeper::lastError() const
{
    return m_lastError;
}

bool Beeper::writeBrightness(int value)
{
    QFile file(m_ledPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_lastError = QString("Cannot open beeper: %1").arg(file.errorString());
        return false;
    }

    QTextStream stream(&file);
    stream << QString::number(value);
    file.close();

    return true;
}
