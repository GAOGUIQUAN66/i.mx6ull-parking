#ifndef AUDIOTHREAD_H
#define AUDIOTHREAD_H

#include <QMutex>
#include <QQueue>
#include <QThread>
#include <QWaitCondition>

class AudioThread : public QThread
{
    Q_OBJECT

public:
    explicit AudioThread(QObject *parent = nullptr);
    ~AudioThread();

    void enqueueFile(const QString &filePath);
    void stop();

signals:
    void playbackStarted(const QString &filePath);
    void playbackFinished(const QString &filePath);
    void playbackError(const QString &filePath, const QString &error);

protected:
    void run() override;

private:
    QString takeNextFile();

    QMutex m_mutex;
    QWaitCondition m_condition;
    QQueue<QString> m_queue;
    bool m_running;
};

#endif // AUDIOTHREAD_H
