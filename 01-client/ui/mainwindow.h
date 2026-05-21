#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QListWidget>
#include <QProgressBar>
#include <QDialog>
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
 * @brief 主窗口类
 *
 * 显示实时视频、车位状态、闸门状态、最近进场记录
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(HardwareInit *hardware = nullptr, QWidget *parent = nullptr);
    ~MainWindow();

    /**
     * @brief 初始化硬件
     */
    bool initHardware();

private slots:
    void updateTime();
    void onQueryClicked();
    void onRechargeClicked();
    void onSettingsClicked();
    void onRfidCardDetected(const QString &cardId);
    void onRfidReadError(const QString &error);
    void onRecognitionTimer();
    void updateParkingStatus();
    void updateRecentEntries();
    void onQueryBack();
    void onSettingsBack();

    // 视频相关槽函数
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
    QGroupBox* createOpsPanel();
    QGroupBox* createRecentPanel();
    QWidget* createBottomBar();
    void updateOperationStats();
    void ensureRechargeDialog();
    void updateRechargeDialogInfo(const QString &cardId, double balance);
    void rechargeByAmount(double amount);

    // 顶部区域
    QLabel *m_titleLabel;
    QLabel *m_timeLabel;

    // 视频区域
    QLabel *m_videoLabel;

    // 车位状态
    QLabel *m_totalSpacesLabel;
    QLabel *m_parkedLabel;
    QLabel *m_availableLabel;
    QProgressBar *m_occupancyBar;

    // 闸门状态
    QLabel *m_gateStatusLabel;
    QLabel *m_todayEntryLabel;
    QLabel *m_todayExitLabel;
    QLabel *m_todayRevenueLabel;

    // 最近进场
    QListWidget *m_recentList;

    // 定时器
    QTimer *m_updateTimer;
    QTimer *m_videoRefreshTimer;
    QTimer *m_recognitionTimer;
    QTimer *m_gateCloseTimer;

    // 数据库
    Database *m_db;

    // 子窗口
    QueryWindow *m_queryWindow;
    SettingsWindow *m_settingsWindow;
    ExitDialog *m_exitDialog;
    QDialog *m_rechargeDialog;
    QLabel *m_rechargeCardLabel;
    QLabel *m_rechargeBalanceLabel;
    QLabel *m_rechargeStatusLabel;
    QString m_rechargeCardId;
    AudioThread *m_audioThread;

    // 视频相关
    V4L2Camera *m_camera;
    VideoThread *m_videoThread;
    RfidThread *m_rfidThread;

    // 网络相关
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

    // 硬件管理（外部传入）
    HardwareInit *m_hardware;
};

#endif // MAINWINDOW_H
