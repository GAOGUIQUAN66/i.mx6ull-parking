#include "audiothread.h"

#include <QDebug>
#include <QFileInfo>
#include <QMutexLocker>
#include <QProcess>

AudioThread::AudioThread(QObject *parent)
    : QThread(parent)
    , m_running(true)
{
}

AudioThread::~AudioThread()
{
    stop();
    wait();
}

void AudioThread::enqueueFile(const QString &filePath)
{
    QMutexLocker locker(&m_mutex);
    m_queue.enqueue(filePath);
    m_condition.wakeOne();
}

void AudioThread::stop()
{
    QMutexLocker locker(&m_mutex);
    m_running = false;
    m_condition.wakeAll();
}

void AudioThread::run()
{
    while (true) {
        QString filePath = takeNextFile();
        if (filePath.isEmpty()) {
            return;
        }

        emit playbackStarted(filePath);
        int exitCode = QProcess::execute("aplay", QStringList() << "-D" << "default" << filePath);
        if (exitCode != 0) {
            QString error = QString("aplay 播放失败，退出码 %1").arg(exitCode);
            qDebug() << "AudioThread:" << error << filePath;
            emit playbackError(filePath, error);
            continue;
        }

        emit playbackFinished(filePath);
    }
}

QString AudioThread::takeNextFile()
{
    QMutexLocker locker(&m_mutex);
    while (m_running && m_queue.isEmpty()) {
        m_condition.wait(&m_mutex);
    }

    if (!m_running && m_queue.isEmpty()) {
        return QString();
    }

    return m_queue.dequeue();
}
