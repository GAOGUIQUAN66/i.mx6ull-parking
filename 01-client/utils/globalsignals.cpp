#include "globalsignals.h"

GlobalSignals* GlobalSignals::m_instance = nullptr;

GlobalSignals::GlobalSignals(QObject *parent)
    : QObject(parent)
{
}

GlobalSignals* GlobalSignals::instance()
{
    if (!m_instance) {
        m_instance = new GlobalSignals();
    }
    return m_instance;
}
