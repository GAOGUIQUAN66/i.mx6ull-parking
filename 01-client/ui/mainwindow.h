#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QListWidget>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHash>
#include <QPixmap>

#include "../database/database.h"
#include "../hardware/v4l2camera.h"
#include "../rfid/rfidthread.h"
#include "../video/videothread.h"
#include "../network/networkclient.h"
#include "../audio/audiothread.h"
#include "exitdialog.h"
#include "querywindow.h"
#include "settingswindow.h"

class HardwareInit;

/**
 * @brief Main shell
 *
 * Video, occupancy, gate, recent activity
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(HardwareInit *hardware = nullptr, QWidget *parent = nullptr);
    ~MainWindow();

    /**
     * @brief Init camera pipeline
     */
    bool initHardware();

private slots:
    void updateTime();
    void onQueryClicked();
    void onSettingsClicked();
    void onRfidCardDetected(const QString &cardId);
    void onRfidReadError(const QString &error);
    void onRecognitionTimer();
    void updateParkingStatus();
    void updateRecentEntries();
    void onQueryBack();
    void onSettingsBack();

    // Video / network slots
    void refreshVideoFrame();
    void onCaptureDone(const QImage &image, const QByteArray &rawFrame);
    void onRecognizeResultReady(const NetworkClient::RecognizeResult &result);
    void onNetworkConnected();
    void onNetworkDisconnected();
    void onNetworkError(const QString &error);
    void onExitDialogCancelled();
    void onExitDialogTimedOut();
    void onGateCloseTimeout();
    void onAudioFileReady(const QString &fileName, const QByteArray &audioData);
    void onAudioPlaybackStarted(const QString &filePath);
    void onAudioPlaybackFinished(const QString &filePath);
    void onAudioPlaybackError(const QString &filePath, const QString &error);

private:
    bool isPlateInCooldown(const QString &plateNumber) const;
    void markPlateCooldown(const QString &plateNumber);
    void showExitDialog(const VehicleInfo &vehicleInfo);
    void closeExitDialog(bool resumeRecognition = true);
    void setGateOpened(bool opened, const QString &reason = QString());
    void openGateForPassage(const QString &reason);
    void updateGateStatusDisplay();
    QString audioDirectoryPath() const;
    void setupUI();
    void setupMenuBar();
    void setupVideoThread();
    void setupRfidThread();
    void setupNetwork();
    void processRecognitionResult(const QString &plateNumber, double confidence);
    void resetCaptureFlow();
    QGroupBox* createVideoPanel();
    QGroupBox* createStatusPanel();
    QGroupBox* createGatePanel();
    QGroupBox* createRecentPanel();
    QWidget* createBottomBar();

    // Header
    QLabel *m_titleLabel;
    QLabel *m_timeLabel;

    // Video preview
    QLabel *m_videoLabel;

    // Parking status
    QLabel *m_totalSpacesLabel;
    QLabel *m_parkedLabel;
    QLabel *m_availableLabel;
    QProgressBar *m_occupancyBar;

    // Gate
    QLabel *m_gateStatusLabel;

    // Recent records
    QListWidget *m_recentList;

    // Timers
    QTimer *m_updateTimer;
    QTimer *m_videoRefreshTimer;
    QTimer *m_recognitionTimer;
    QTimer *m_gateCloseTimer;

    // State库
    Database *m_db;

    // Child windows
    QueryWindow *m_queryWindow;
    SettingsWindow *m_settingsWindow;
    ExitDialog *m_exitDialog;
    AudioThread *m_audioThread;

    // Capture
    V4L2Camera *m_camera;
    VideoThread *m_videoThread;
    RfidThread *m_rfidThread;

    // TCP client
    NetworkClient *m_networkClient;
    bool m_waitingForResult;
    QString m_pendingRfidCard;
    QDateTime m_pendingRfidTime;
    QHash<QString, QDateTime> m_plateCooldowns;
    QString m_pendingExitPlateNumber;
    QDateTime m_pendingExitEntryTime;
    double m_pendingExitFee;
    QPixmap m_lastCapturePixmap;
    QDateTime m_recognitionBlockedUntil;
    QDateTime m_gateOpenUntil;
    QString m_gateOpenReason;

    // Board hardware ref
    HardwareInit *m_hardware;
};

#endif // MAINWINDOW_H
