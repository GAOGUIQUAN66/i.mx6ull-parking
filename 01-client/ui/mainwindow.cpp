#include "mainwindow.h"
#include "../hardware/hardwareinit.h"
#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QPixmap>
#include <QProcess>

// 颜色定义
#define COLOR_BG_DARK "#1a1a2e"
#define COLOR_BG_PANEL "#16213e"
#define COLOR_ACCENT "#e94560"
#define COLOR_SUCCESS "#4ecca3"
#define COLOR_WARNING "#ffd93d"
#define COLOR_TEXT_GRAY "#a0a0a0"

// 服务器配置
#define SERVER_HOST "192.168.137.50" // Ubuntu上位机IP
#define SERVER_PORT 8888
#define RFID_VALID_WINDOW_MS 15000
#define GATE_OPEN_MS 5000
#define PLATE_COOLDOWN_MS GATE_OPEN_MS
#define AUDIO_DIR_PATH "/run/media/mmcblk1p1/audio"

MainWindow::MainWindow(HardwareInit *hardware, QWidget *parent)
    : QMainWindow(parent), m_db(nullptr), m_queryWindow(nullptr), m_settingsWindow(nullptr), m_exitDialog(nullptr), m_audioThread(nullptr), m_updateTimer(nullptr), m_videoRefreshTimer(nullptr), m_recognitionTimer(nullptr), m_gateCloseTimer(nullptr), m_camera(nullptr), m_videoThread(nullptr), m_rfidThread(nullptr), m_networkClient(nullptr), m_waitingForResult(false), m_pendingRfidCard(), m_pendingRfidTime(), m_plateCooldowns(), m_pendingExitPlateNumber(), m_pendingExitEntryTime(), m_pendingExitFee(0.0), m_lastCapturePixmap(), m_recognitionBlockedUntil(), m_gateOpenUntil(), m_gateOpenReason(), m_hardware(hardware), m_todayEntryLabel(nullptr), m_todayExitLabel(nullptr), m_todayRevenueLabel(nullptr)
{
    setWindowTitle("智能车库管理系统");
    setFixedSize(1024, 600);

    // 设置深色主题样式
    setStyleSheet(QString(
                      "QMainWindow { background-color: %1; }"
                      "QLabel { color: white; }"
                      "QPushButton { "
                      "    border: none; "
                      "    border-radius: 8px; "
                      "    padding: 12px 30px; "
                      "    font-size: 16px; "
                      "    font-weight: bold; "
                      "}"
                      "QPushButton:hover { opacity: 0.9; }"
                      "QGroupBox { "
                      "    border: 1px solid #333; "
                      "    border-radius: 10px; "
                      "    margin-top: 10px; "
                      "    padding-top: 10px; "
                      "    background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %2, stop:1 %1); "
                      "}"
                      "QGroupBox::title { "
                      "    color: %3; "
                      "    subcontrol-origin: margin; "
                      "    left: 15px; "
                      "    font-size: 16px; "
                      "}"
                      "QProgressBar { "
                      "    border: none; "
                      "    border-radius: 10px; "
                      "    background-color: #333; "
                      "    height: 20px; "
                      "    text-align: center; "
                      "}"
                      "QProgressBar::chunk { "
                      "    border-radius: 10px; "
                      "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 %4, stop:1 #45b7aa); "
                      "}"
                      "QListWidget { "
                      "    background: transparent; "
                      "    border: none; "
                      "    color: white; "
                      "}"
                      "QListWidget::item { "
                      "    background: rgba(255,255,255,0.05); "
                      "    border-radius: 5px; "
                      "    padding: 8px; "
                      "    margin: 2px 0; "
                      "}")
                      .arg(COLOR_BG_DARK, COLOR_BG_PANEL, COLOR_ACCENT, COLOR_SUCCESS));

    setupUI();
    setupMenuBar();

    // 初始化数据库
    m_db = new Database(this);
    if (!m_db->open("/run/media/mmcblk1p1/parking.db"))
    {
        qDebug() << "数据库打开失败:" << m_db->lastError();
    }

    // 初始化子窗口
    m_queryWindow = new QueryWindow(m_db, this);
    m_settingsWindow = new SettingsWindow(this);
    m_exitDialog = new ExitDialog(this);
    m_audioThread = new AudioThread(this);

    // 连接子窗口信号
    connect(m_queryWindow, &QueryWindow::backToMain, this, &MainWindow::onQueryBack);
    connect(m_settingsWindow, &SettingsWindow::backToMain, this, &MainWindow::onSettingsBack);
    connect(m_exitDialog, &ExitDialog::cancelled, this, &MainWindow::onExitDialogCancelled);
    connect(m_exitDialog, &ExitDialog::timedOut, this, &MainWindow::onExitDialogTimedOut);
    connect(m_exitDialog, &QDialog::finished, this, [this](int)
            { closeExitDialog(true); });
    connect(m_audioThread, &AudioThread::playbackStarted, this, &MainWindow::onAudioPlaybackStarted);
    connect(m_audioThread, &AudioThread::playbackFinished, this, &MainWindow::onAudioPlaybackFinished);
    connect(m_audioThread, &AudioThread::playbackError, this, &MainWindow::onAudioPlaybackError);
    m_audioThread->start();

    // 初始化视频和网络
    setupVideoThread();
    setupRfidThread();
    setupNetwork();

    // 定时器更新时间
    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &MainWindow::updateTime);
    connect(m_updateTimer, &QTimer::timeout, this, &MainWindow::updateParkingStatus);
    connect(m_updateTimer, &QTimer::timeout, this, &MainWindow::updateRecentEntries);
    connect(m_updateTimer, &QTimer::timeout, this, &MainWindow::updateOperationStats);
    m_updateTimer->start(1000);

    // 单独定时刷新视频，只取最新帧，避免主线程积压旧帧
    m_videoRefreshTimer = new QTimer(this);
    connect(m_videoRefreshTimer, &QTimer::timeout, this, &MainWindow::refreshVideoFrame);
    m_videoRefreshTimer->start(50);

    // 周期上传识别，取代手动抓拍
    m_recognitionTimer = new QTimer(this);
    connect(m_recognitionTimer, &QTimer::timeout, this, &MainWindow::onRecognitionTimer);
    m_recognitionTimer->start(1500);

    m_gateCloseTimer = new QTimer(this);
    m_gateCloseTimer->setSingleShot(true);
    connect(m_gateCloseTimer, &QTimer::timeout, this, &MainWindow::onGateCloseTimeout);

    updateTime();
    updateParkingStatus();
    updateRecentEntries();
    updateOperationStats();
}

MainWindow::~MainWindow()
{
    // 停止视频线程
    if (m_videoThread)
    {
        m_videoThread->stop();
        m_videoThread->wait();
        delete m_videoThread;
    }

    if (m_rfidThread)
    {
        m_rfidThread->stop();
        m_rfidThread->wait();
        delete m_rfidThread;
    }

    if (m_audioThread)
    {
        m_audioThread->stop();
        m_audioThread->wait();
        delete m_audioThread;
    }

    // 关闭摄像头
    if (m_camera)
    {
        m_camera->close();
        delete m_camera;
    }

    // 断开网络
    if (m_networkClient)
    {
        m_networkClient->disconnect();
        delete m_networkClient;
    }

    if (m_db)
    {
        m_db->close();
    }
}

bool MainWindow::initHardware()
{
    // 初始化摄像头
    m_camera = new V4L2Camera(this);

    // 尝试打开摄像头设备
    QString cameraDevice = "/dev/video1";
    if (!m_camera->open(cameraDevice))
    {
        qDebug() << "摄像头打开失败:" << m_camera->lastError();
        // 尝试其他设备
        cameraDevice = "/dev/video0";
        if (!m_camera->open(cameraDevice))
        {
            qDebug() << "摄像头打开失败:" << m_camera->lastError();
            return false;
        }
    }

    // 预览优先使用无需JPEG解码的原始格式，避免板端libjpeg/插件异常导致黑屏
    if (!m_camera->setFormat(640, 480, V4L2Camera::FORMAT_RGB565))
    {
        if (!m_camera->setFormat(640, 480, V4L2Camera::FORMAT_YUYV))
        {
            if (!m_camera->setFormat(640, 480, V4L2Camera::FORMAT_JPEG))
            {
                if (!m_camera->setFormat(640, 480, V4L2Camera::FORMAT_UYVY))
                {
                    qDebug() << "设置摄像头格式失败:" << m_camera->lastError();
                    return false;
                }
            }
        }
    }

    // 申请缓冲区
    if (!m_camera->requestBuffers(4))
    {
        qDebug() << "申请缓冲区失败:" << m_camera->lastError();
        return false;
    }

    qDebug() << "摄像头初始化成功，像素格式:" << m_camera->pixelFormatName();
    return true;
}

void MainWindow::setupVideoThread()
{
    // 尝试初始化硬件摄像头
    if (!initHardware())
    {
        qDebug() << "摄像头硬件初始化失败，视频功能不可用";
        m_videoLabel->setText("摄像头连接失败\n\n请检查设备连接");
        return;
    }

    // 创建视频采集线程
    m_videoThread = new VideoThread(this);
    m_videoThread->setCamera(m_camera);

    // 连接信号
    connect(m_videoThread, &VideoThread::captureDone, this, &MainWindow::onCaptureDone);

    // 启动视频采集
    m_videoThread->start();
}

void MainWindow::setupRfidThread()
{
    if (!m_hardware || !m_hardware->serialPort() || !m_hardware->serialPort()->isOpen())
    {
        qDebug() << "RFID串口未就绪，跳过RFID线程初始化";
        return;
    }

    m_rfidThread = new RfidThread(this);
    m_rfidThread->setSerialPort(m_hardware->serialPort());
    connect(m_rfidThread, &RfidThread::cardDetected, this, &MainWindow::onRfidCardDetected);
    connect(m_rfidThread, &RfidThread::readError, this, &MainWindow::onRfidReadError);
    m_rfidThread->start();
}

void MainWindow::setupNetwork()
{
    m_networkClient = new NetworkClient(this);

    // 连接网络信号
    connect(m_networkClient, &NetworkClient::connected, this, &MainWindow::onNetworkConnected);
    connect(m_networkClient, &NetworkClient::disconnected, this, &MainWindow::onNetworkDisconnected);
    connect(m_networkClient, &NetworkClient::errorOccurred, this, &MainWindow::onNetworkError);
    connect(m_networkClient, &NetworkClient::recognizeResultReady, this, &MainWindow::onRecognizeResultReady);
    connect(m_networkClient, &NetworkClient::audioFileReady, this, &MainWindow::onAudioFileReady);

    // 连接服务器
    m_networkClient->connectToServer(SERVER_HOST, SERVER_PORT);
}

void MainWindow::setupUI()
{
    QWidget *centralWidget = new QWidget(this);
    centralWidget->setStyleSheet(QString("background-color: %1;").arg(COLOR_BG_DARK));

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // 顶部栏
    QHBoxLayout *headerLayout = new QHBoxLayout();
    m_titleLabel = new QLabel("智能车库管理系统");
    m_titleLabel->setStyleSheet(QString(
                                    "font-size: 24px; font-weight: bold; color: %1; padding: 5px;")
                                    .arg(COLOR_ACCENT));

    m_timeLabel = new QLabel();
    m_timeLabel->setStyleSheet("font-size: 18px; color: #a0a0a0; padding: 5px;");

    headerLayout->addWidget(m_titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(m_timeLabel);
    mainLayout->addLayout(headerLayout);

    // 主内容区
    QHBoxLayout *contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(10);

    // 视频面板
    QGroupBox *videoGroup = createVideoPanel();
    videoGroup->setFixedWidth(650);
    contentLayout->addWidget(videoGroup);

    // 右侧信息面板
    QVBoxLayout *rightLayout = new QVBoxLayout();
    rightLayout->setSpacing(10);

    // 车位状态
    QGroupBox *statusGroup = createStatusPanel();
    rightLayout->addWidget(statusGroup);

    // 闸门状态
    QGroupBox *gateGroup = createGatePanel();
    rightLayout->addWidget(gateGroup);

    // 今日运营
    QGroupBox *opsGroup = createOpsPanel();
    rightLayout->addWidget(opsGroup);

    // 最近进场
    QGroupBox *recentGroup = createRecentPanel();
    rightLayout->addWidget(recentGroup, 1);

    contentLayout->addLayout(rightLayout, 1);
    mainLayout->addLayout(contentLayout, 1);

    // 底部按钮栏
    QWidget *bottomBar = createBottomBar();
    mainLayout->addWidget(bottomBar);

    setCentralWidget(centralWidget);
}

void MainWindow::setupMenuBar()
{
    // 暂不需要菜单栏
}

QGroupBox *MainWindow::createVideoPanel()
{
    QGroupBox *group = new QGroupBox("实时视频监控");
    QVBoxLayout *layout = new QVBoxLayout(group);

    m_videoLabel = new QLabel();
    m_videoLabel->setMinimumSize(620, 420);
    m_videoLabel->setStyleSheet(QString(
                                    "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
                                    "stop:0 %1, stop:1 %2); "
                                    "border-radius: 10px; "
                                    "border: 2px solid #333;")
                                    .arg(COLOR_BG_DARK, COLOR_BG_PANEL));
    m_videoLabel->setAlignment(Qt::AlignCenter);
    m_videoLabel->setText("摄像头实时画面\n\n640 x 480\n等待视频流...");

    layout->addWidget(m_videoLabel);
    return group;
}

QGroupBox *MainWindow::createStatusPanel()
{
    QGroupBox *group = new QGroupBox("车位状态");
    QHBoxLayout *mainLayout = new QHBoxLayout(group);

    // 状态数字
    QVBoxLayout *statusLayout = new QVBoxLayout();

    QHBoxLayout *numbersLayout = new QHBoxLayout();
    numbersLayout->setSpacing(20);

    // 总车位
    QVBoxLayout *totalLayout = new QVBoxLayout();
    QLabel *totalLabel = new QLabel("总车位");
    totalLabel->setStyleSheet(QString("font-size: 12px; color: %1;").arg(COLOR_TEXT_GRAY));
    m_totalSpacesLabel = new QLabel("50");
    m_totalSpacesLabel->setStyleSheet(QString("font-size: 28px; font-weight: bold; color: %1;").arg(COLOR_SUCCESS));
    m_totalSpacesLabel->setAlignment(Qt::AlignCenter);
    totalLayout->addWidget(totalLabel);
    totalLayout->addWidget(m_totalSpacesLabel);

    // 已停放
    QVBoxLayout *parkedLayout = new QVBoxLayout();
    QLabel *parkedLabel = new QLabel("已停放");
    parkedLabel->setStyleSheet(QString("font-size: 12px; color: %1;").arg(COLOR_TEXT_GRAY));
    m_parkedLabel = new QLabel("0");
    m_parkedLabel->setStyleSheet(QString("font-size: 28px; font-weight: bold; color: %1;").arg(COLOR_WARNING));
    m_parkedLabel->setAlignment(Qt::AlignCenter);
    parkedLayout->addWidget(parkedLabel);
    parkedLayout->addWidget(m_parkedLabel);

    // 空余
    QVBoxLayout *availableLayout = new QVBoxLayout();
    QLabel *availableLabel = new QLabel("空余");
    availableLabel->setStyleSheet(QString("font-size: 12px; color: %1;").arg(COLOR_TEXT_GRAY));
    m_availableLabel = new QLabel("50");
    m_availableLabel->setStyleSheet(QString("font-size: 28px; font-weight: bold; color: %1;").arg(COLOR_SUCCESS));
    m_availableLabel->setAlignment(Qt::AlignCenter);
    availableLayout->addWidget(availableLabel);
    availableLayout->addWidget(m_availableLabel);

    numbersLayout->addLayout(totalLayout);
    numbersLayout->addLayout(parkedLayout);
    numbersLayout->addLayout(availableLayout);
    numbersLayout->addStretch();

    statusLayout->addLayout(numbersLayout);

    // 占用率进度条
    m_occupancyBar = new QProgressBar();
    m_occupancyBar->setRange(0, 100);
    m_occupancyBar->setValue(0);
    m_occupancyBar->setFormat("%p%");
    m_occupancyBar->setFixedHeight(20);
    statusLayout->addWidget(m_occupancyBar);

    mainLayout->addLayout(statusLayout);
    return group;
}

QGroupBox *MainWindow::createGatePanel()
{
    QGroupBox *group = new QGroupBox();
    QHBoxLayout *layout = new QHBoxLayout(group);

    QLabel *gateIcon = new QLabel("[GATE]");
    gateIcon->setStyleSheet("font-size: 18px; font-weight: bold; color: #e94560;");

    QVBoxLayout *infoLayout = new QVBoxLayout();
    QLabel *gateLabel = new QLabel("闸门状态");
    gateLabel->setStyleSheet(QString("font-size: 12px; color: %1;").arg(COLOR_TEXT_GRAY));
    m_gateStatusLabel = new QLabel("已关闭");
    m_gateStatusLabel->setStyleSheet(QString("font-size: 16px; font-weight: bold; color: %1;").arg(COLOR_ACCENT));
    m_gateStatusLabel->setWordWrap(true);

    infoLayout->addWidget(gateLabel);
    infoLayout->addWidget(m_gateStatusLabel);

    layout->addWidget(gateIcon);
    layout->addLayout(infoLayout);
    layout->addStretch();

    return group;
}

QGroupBox *MainWindow::createOpsPanel()
{
    QGroupBox *group = new QGroupBox("今日运营统计");
    QVBoxLayout *layout = new QVBoxLayout(group);
    layout->setSpacing(8);

    auto createRow = [](const QString &title, QLabel **valueLabel, const QString &color) {
        QHBoxLayout *row = new QHBoxLayout();
        QLabel *titleLabel = new QLabel(title);
        titleLabel->setStyleSheet(QString("font-size: 12px; color: %1;").arg(COLOR_TEXT_GRAY));
        QLabel *val = new QLabel("--");
        val->setStyleSheet(QString("font-size: 13px; color: %1; font-weight: bold;").arg(color));
        val->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        row->addWidget(titleLabel);
        row->addStretch();
        row->addWidget(val);
        *valueLabel = val;
        return row;
    };

    layout->addLayout(createRow("今日入场", &m_todayEntryLabel, COLOR_SUCCESS));
    layout->addLayout(createRow("今日出场", &m_todayExitLabel, COLOR_WARNING));
    layout->addLayout(createRow("今日收入(元)", &m_todayRevenueLabel, COLOR_ACCENT));

    return group;
}

QGroupBox *MainWindow::createRecentPanel()
{
    QGroupBox *group = new QGroupBox("近期记录");
    QVBoxLayout *layout = new QVBoxLayout(group);

    m_recentList = new QListWidget();
    m_recentList->setStyleSheet(
        "QListWidget { background: transparent; border: none; }"
        "QListWidget::item { background: rgba(255,255,255,0.05); border-radius: 5px; padding: 10px; margin: 2px 0; }"
        "QListWidget::item:selected { background: rgba(233, 69, 96, 0.3); }");

    layout->addWidget(m_recentList);
    return group;
}

QWidget *MainWindow::createBottomBar()
{
    QWidget *bar = new QWidget();
    bar->setFixedHeight(60);
    bar->setStyleSheet(QString("background-color: %1;").arg(COLOR_BG_PANEL));

    QHBoxLayout *layout = new QHBoxLayout(bar);
    layout->setSpacing(20);

    QPushButton *queryBtn = new QPushButton("查询记录");
    queryBtn->setStyleSheet(QString(
                                "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %1, stop:1 #3db892); "
                                "color: white;")
                                .arg(COLOR_SUCCESS));

    QPushButton *settingsBtn = new QPushButton("系统设置");
    settingsBtn->setStyleSheet(QString(
                                   "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %1, stop:1 #3db892); "
                                   "color: white;")
                                   .arg(COLOR_SUCCESS));

    connect(queryBtn, &QPushButton::clicked, this, &MainWindow::onQueryClicked);
    connect(settingsBtn, &QPushButton::clicked, this, &MainWindow::onSettingsClicked);

    layout->addStretch();
    layout->addWidget(queryBtn);
    layout->addWidget(settingsBtn);
    layout->addStretch();

    return bar;
}

void MainWindow::updateTime()
{
    m_timeLabel->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    updateGateStatusDisplay();
}

void MainWindow::onQueryClicked()
{
    m_queryWindow->show();
    m_queryWindow->raise();
    m_queryWindow->activateWindow();
}

void MainWindow::onSettingsClicked()
{
    m_settingsWindow->show();
    m_settingsWindow->raise();
    m_settingsWindow->activateWindow();
}

void MainWindow::onRecognitionTimer()
{
    if (m_waitingForResult)
    {
        return;
    }

    if (m_recognitionBlockedUntil.isValid() &&
        QDateTime::currentDateTime() < m_recognitionBlockedUntil)
    {
        return;
    }

    if (m_exitDialog && m_exitDialog->isVisible())
    {
        return;
    }

    if ((m_queryWindow && m_queryWindow->isVisible()) ||
        (m_settingsWindow && m_settingsWindow->isVisible()))
    {
        return;
    }

    if (!m_networkClient || !m_networkClient->isConnected())
    {
        return;
    }

    if (!m_videoThread || !m_camera || m_camera->state() != V4L2Camera::StateStreaming)
    {
        return;
    }

    m_waitingForResult = true;
    m_videoThread->triggerCapture();
}

void MainWindow::onRfidCardDetected(const QString &cardId)
{
    if (!m_db || !m_db->isOpen())
    {
        qDebug() << "RFID事件被忽略，数据库未打开";
        return;
    }

    if (!m_db->ensureCardAccount(cardId))
    {
        qDebug() << "RFID账户初始化失败, 卡号:" << cardId << "错误:" << m_db->lastError();
        return;
    }

    double balance = m_db->getCardBalance(cardId);
    if (balance < 0)
    {
        qDebug() << "RFID余额查询失败, 卡号:" << cardId << "错误:" << m_db->lastError();
        return;
    }

    m_pendingRfidCard = cardId;
    m_pendingRfidTime = QDateTime::currentDateTime();
    qDebug() << "记录出场刷卡, 卡号:" << cardId << "当前余额:" << balance;

    if (!m_exitDialog || !m_exitDialog->isVisible() || m_pendingExitPlateNumber.isEmpty())
    {
        qDebug() << "当前无待结算出场车辆，忽略本次刷卡";
        return;
    }

    m_exitDialog->setPaymentInfo("刷卡成功，正在校验余额并结算", cardId, balance);

    if (!m_db->checkoutVehicle(m_pendingExitPlateNumber, cardId))
    {
        qDebug() << "出场结算失败, 车牌:" << m_pendingExitPlateNumber
                 << "刷卡:" << cardId
                 << "错误:" << m_db->lastError();
        m_exitDialog->setPaymentInfo(QString("刷卡失败: %1").arg(m_db->lastError()), cardId, balance);
        return;
    }

    double remainBalance = m_db->getCardBalance(cardId);
    qDebug() << "出场结算成功, 车牌:" << m_pendingExitPlateNumber
             << "刷卡:" << cardId
             << "扣费:" << m_pendingExitFee
             << "剩余余额:" << remainBalance;
    m_exitDialog->stopCountdown();
    m_exitDialog->setPaymentInfo(
        QString("刷卡成功，已扣费 ¥%1，剩余余额 ¥%2")
            .arg(m_pendingExitFee, 0, 'f', 2)
            .arg(remainBalance, 0, 'f', 2),
        cardId,
        remainBalance);
    markPlateCooldown(m_pendingExitPlateNumber);
    openGateForPassage(QString("车辆 %1 已出场").arg(m_pendingExitPlateNumber));
    m_pendingExitPlateNumber.clear();
    updateParkingStatus();
    updateRecentEntries();
    QTimer::singleShot(1500, this, [this]()
                       { closeExitDialog(true); });
}

void MainWindow::onRfidReadError(const QString &error)
{
    qDebug() << "RFID读取错误:" << error;
}

void MainWindow::updateParkingStatus()
{
    if (!m_db || !m_db->isOpen())
        return;

    ParkingConfig config = m_db->getConfig();
    int parked = m_db->getParkedCount();
    int available = m_db->getAvailableSpaces();
    double rate = m_db->getOccupancyRate();

    m_totalSpacesLabel->setText(QString::number(config.totalSpaces));
    m_parkedLabel->setText(QString::number(parked));
    m_availableLabel->setText(QString::number(available));
    m_occupancyBar->setValue((int)rate);
}

void MainWindow::updateRecentEntries()
{
    if (!m_db || !m_db->isOpen())
        return;

    QList<ParkingRecord> records = m_db->getRecentRecords(5);

    m_recentList->clear();
    for (const ParkingRecord &record : records)
    {
        bool isExit = record.exitTime.isValid();
        QString actionText = isExit ? QString::fromUtf8("↗ 出场") : QString::fromUtf8("↘ 入场");
        QString timeText = isExit ? record.exitTime.toString("MM-dd hh:mm:ss")
                                  : record.entryTime.toString("MM-dd hh:mm:ss");
        QString itemText = QString("%1  %2\n%3")
                               .arg(actionText)
                               .arg(record.plateNumber)
                               .arg(timeText);

        QListWidgetItem *item = new QListWidgetItem(itemText);
        item->setForeground(QColor("#ffffff"));
        item->setBackground(isExit ? QColor("#1d5e4d") : QColor("#6a4320"));
        m_recentList->addItem(item);
    }

    // 如果没有记录，显示提示
    if (records.isEmpty())
    {
        m_recentList->addItem("暂无近期记录");
    }
}

void MainWindow::updateOperationStats()
{
    if (!m_todayEntryLabel || !m_todayExitLabel || !m_todayRevenueLabel || !m_db || !m_db->isOpen()) {
        return;
    }

    const QDate today = QDate::currentDate();
    const QList<ParkingRecord> records = m_db->queryHistory(today, today);
    int entryCount = 0;
    int exitCount = 0;
    double revenue = 0.0;
    for (const ParkingRecord &record : records) {
        if (record.entryTime.date() == today) {
            ++entryCount;
        }
        if (record.exitTime.isValid() && record.exitTime.date() == today) {
            ++exitCount;
            revenue += record.fee;
        }
    }

    m_todayEntryLabel->setText(QString::number(entryCount));
    m_todayExitLabel->setText(QString::number(exitCount));
    m_todayRevenueLabel->setText(QString::number(revenue, 'f', 2));
}

bool MainWindow::isPlateInCooldown(const QString &plateNumber) const
{
    if (!m_plateCooldowns.contains(plateNumber))
    {
        return false;
    }

    return m_plateCooldowns.value(plateNumber).msecsTo(QDateTime::currentDateTime()) < PLATE_COOLDOWN_MS;
}

void MainWindow::markPlateCooldown(const QString &plateNumber)
{
    m_plateCooldowns.insert(plateNumber, QDateTime::currentDateTime());
}

void MainWindow::onQueryBack()
{
    // 查询窗口返回
}

void MainWindow::onSettingsBack()
{
    // 设置窗口返回
}

void MainWindow::onExitDialogCancelled()
{
    qDebug() << "出场窗口已关闭";
    if (!m_pendingExitPlateNumber.isEmpty())
    {
        markPlateCooldown(m_pendingExitPlateNumber);
    }
    closeExitDialog(true);
}

void MainWindow::onExitDialogTimedOut()
{
    qDebug() << "出场等待刷卡超时";
    if (!m_pendingExitPlateNumber.isEmpty())
    {
        markPlateCooldown(m_pendingExitPlateNumber);
    }
    closeExitDialog(true);
}

void MainWindow::onGateCloseTimeout()
{
    setGateOpened(false);
}

// ==================== 视频相关槽函数 ====================

void MainWindow::refreshVideoFrame()
{
    if (!m_videoThread)
        return;

    QImage frame = m_videoThread->takeLatestFrame();
    if (frame.isNull())
        return;

    // 缩放到显示区域大小
    QPixmap pixmap = QPixmap::fromImage(frame);
    QPixmap scaled = pixmap.scaled(m_videoLabel->size(), Qt::KeepAspectRatio, Qt::FastTransformation);
    m_videoLabel->setPixmap(scaled);
}

void MainWindow::onCaptureDone(const QImage &image, const QByteArray &rawFrame)
{
    qDebug() << "抓拍完成，发送原始帧进行识别";
    if (!image.isNull())
    {
        m_lastCapturePixmap = QPixmap::fromImage(image);
    }

    if (m_networkClient && m_networkClient->isConnected())
    {
        bool sent = false;

        if (!rawFrame.isEmpty() && m_camera)
        {
            sent = m_networkClient->sendRawFrameForRecognition(
                rawFrame, m_camera->width(), m_camera->height(), m_camera->pixelFormatName());
        }

        if (!sent)
        {
            qDebug() << "发送抓拍图像失败:" << m_networkClient->lastError();
            QMessageBox::warning(this, "发送失败", m_networkClient->lastError());
            m_waitingForResult = false;
            resetCaptureFlow();
        }
    }
    else
    {
        if (!m_networkClient || !m_networkClient->isConnected())
        {
            QMessageBox::warning(this, "网络错误", "未连接到识别服务器");
        }
        m_waitingForResult = false;
        resetCaptureFlow();
    }
}

void MainWindow::onRecognizeResultReady(const NetworkClient::RecognizeResult &result)
{
    qDebug() << "收到识别结果:" << result.plateNumber << "置信度:" << result.confidence;

    if (result.success && !result.plateNumber.isEmpty())
    {
        processRecognitionResult(result.plateNumber, result.confidence);
    }
    else
    {
        // 识别失败
        QString errorMsg = result.errorMessage.isEmpty() ? "车牌识别失败，请重试" : result.errorMessage;
        qDebug() << "识别失败, RFID卡:" << m_pendingRfidCard << "错误:" << errorMsg;
    }

    m_waitingForResult = false;
}

void MainWindow::onNetworkConnected()
{
    qDebug() << "已连接到识别服务器";
    if (m_settingsWindow)
    {
        m_settingsWindow->setActualConnectionStatus(true);
    }
}

void MainWindow::onNetworkDisconnected()
{
    qDebug() << "与识别服务器断开连接";
    if (m_settingsWindow)
    {
        m_settingsWindow->setActualConnectionStatus(false);
    }
}

void MainWindow::onNetworkError(const QString &error)
{
    qDebug() << "网络错误:" << error;
}

void MainWindow::onAudioFileReady(const QString &fileName, const QByteArray &audioData)
{
    if (fileName.isEmpty() || audioData.isEmpty())
    {
        qDebug() << "音频文件无效，忽略保存";
        return;
    }

    QDir dir(audioDirectoryPath());
    if (!dir.exists() && !dir.mkpath("."))
    {
        qDebug() << "创建音频目录失败:" << dir.absolutePath();
        return;
    }

    QString fullPath = dir.filePath(fileName);
    QFile file(fullPath);
    if (!file.open(QIODevice::WriteOnly))
    {
        qDebug() << "保存音频文件失败:" << fullPath << file.errorString();
        return;
    }

    file.write(audioData);
    file.close();
    qDebug() << "音频文件已保存:" << fullPath;

    if (m_audioThread)
    {
        m_audioThread->enqueueFile(fullPath);
    }
}

void MainWindow::onAudioPlaybackStarted(const QString &filePath)
{
    qDebug() << "开始播放语音:" << filePath;
}

void MainWindow::onAudioPlaybackFinished(const QString &filePath)
{
    qDebug() << "语音播放完成:" << filePath;
}

void MainWindow::onAudioPlaybackError(const QString &filePath, const QString &error)
{
    qDebug() << "语音播放失败:" << filePath << error;
}

void MainWindow::resetCaptureFlow()
{
}

QString MainWindow::audioDirectoryPath() const
{
    return QString::fromUtf8(AUDIO_DIR_PATH);
}

void MainWindow::setGateOpened(bool opened, const QString &reason)
{
    if (!m_gateStatusLabel)
    {
        return;
    }

    if (opened)
    {
        m_gateOpenUntil = QDateTime::currentDateTime().addMSecs(GATE_OPEN_MS);
        m_gateOpenReason = reason;
        updateGateStatusDisplay();
        return;
    }

    m_gateOpenUntil = QDateTime();
    m_gateOpenReason.clear();
    m_gateStatusLabel->setText("已关闭");
    m_gateStatusLabel->setStyleSheet(QString("font-size: 16px; font-weight: bold; color: %1;").arg(COLOR_ACCENT));
}

void MainWindow::openGateForPassage(const QString &reason)
{
    m_recognitionBlockedUntil = QDateTime::currentDateTime().addMSecs(GATE_OPEN_MS);
    setGateOpened(true, reason);
    if (m_gateCloseTimer)
    {
        m_gateCloseTimer->start(GATE_OPEN_MS);
    }
}

void MainWindow::updateGateStatusDisplay()
{
    if (!m_gateStatusLabel)
    {
        return;
    }

    if (!m_gateOpenUntil.isValid())
    {
        return;
    }

    qint64 remainingMs = QDateTime::currentDateTime().msecsTo(m_gateOpenUntil);
    if (remainingMs <= 0)
    {
        m_gateStatusLabel->setText("已关闭");
        m_gateStatusLabel->setStyleSheet(QString("font-size: 16px; font-weight: bold; color: %1;").arg(COLOR_ACCENT));
        return;
    }

    int remainingSeconds = static_cast<int>((remainingMs + 999) / 1000);
    QString text = m_gateOpenReason.isEmpty()
                       ? QString("已打开 (%1秒)").arg(remainingSeconds)
                       : QString("已打开 (%1秒)\n%2").arg(remainingSeconds).arg(m_gateOpenReason);
    m_gateStatusLabel->setText(text);
    m_gateStatusLabel->setStyleSheet(QString("font-size: 16px; font-weight: bold; color: %1;").arg(COLOR_SUCCESS));
}

void MainWindow::showExitDialog(const VehicleInfo &vehicleInfo)
{
    if (!m_exitDialog)
    {
        return;
    }

    QDateTime now = QDateTime::currentDateTime();
    qint64 durationMinutes = (vehicleInfo.entryTime.secsTo(now) + 59) / 60;
    int duration = qMax(1, static_cast<int>(durationMinutes));
    double fee = m_db->calculateFee(vehicleInfo.entryTime, now);

    m_pendingExitPlateNumber = vehicleInfo.plateNumber;
    m_pendingExitEntryTime = vehicleInfo.entryTime;
    m_pendingExitFee = fee;
    m_pendingRfidCard.clear();
    m_pendingRfidTime = QDateTime();

    m_exitDialog->setImage(m_lastCapturePixmap);
    m_exitDialog->setParkingInfo(vehicleInfo.plateNumber, vehicleInfo.entryTime, now, duration);
    m_exitDialog->setFeeInfo(fee, duration, 0.1);
    m_exitDialog->setPaymentInfo("请在 10 秒内刷卡完成出场");
    m_exitDialog->startCountdown(10);
    m_exitDialog->show();
    m_exitDialog->raise();
    m_exitDialog->activateWindow();

    if (m_recognitionTimer)
    {
        m_recognitionTimer->stop();
    }

    qDebug() << "识别到在场车辆，弹出出场窗口, 车牌:" << vehicleInfo.plateNumber
             << "停车分钟:" << duration
             << "应付金额:" << fee;
}

void MainWindow::closeExitDialog(bool resumeRecognition)
{
    if (m_exitDialog && m_exitDialog->isVisible())
    {
        m_exitDialog->stopCountdown();
        m_exitDialog->hide();
    }

    m_pendingExitPlateNumber.clear();
    m_pendingExitEntryTime = QDateTime();
    m_pendingExitFee = 0.0;
    m_pendingRfidCard.clear();
    m_pendingRfidTime = QDateTime();

    if (resumeRecognition && m_recognitionTimer && !m_recognitionTimer->isActive())
    {
        m_recognitionTimer->start(1500);
    }
}

void MainWindow::processRecognitionResult(const QString &plateNumber, double confidence)
{
    Q_UNUSED(confidence);

    if (!m_db || !m_db->isOpen())
    {
        qDebug() << "识别结果被忽略，数据库未打开";
        return;
    }

    if (isPlateInCooldown(plateNumber))
    {
        qDebug() << "忽略冷却中的车牌识别结果:" << plateNumber;
        return;
    }

    VehicleInfo activeVehicle = m_db->queryActiveVehicle(plateNumber);
    if (activeVehicle.id == 0)
    {
        if (m_db->isIllegalVehicle(plateNumber))
        {
            if (m_hardware)
            {
                m_hardware->alarm();
            }
            qDebug() << "入场被拒绝, 黑名单车辆:" << plateNumber;
            markPlateCooldown(plateNumber);
            m_pendingRfidCard.clear();
            m_pendingRfidTime = QDateTime();
            return;
        }

        int recordId = m_db->addVehicle(plateNumber);
        if (recordId < 0)
        {
            qDebug() << "自动入场落库失败, 车牌:" << plateNumber
                     << "错误:" << m_db->lastError();
            m_pendingRfidCard.clear();
            m_pendingRfidTime = QDateTime();
            return;
        }

        qDebug() << "自动入场成功, recordId:" << recordId << "车牌:" << plateNumber;
        markPlateCooldown(plateNumber);
        openGateForPassage(QString("车辆 %1 入场").arg(plateNumber));
        m_pendingRfidCard.clear();
        m_pendingRfidTime = QDateTime();
        updateParkingStatus();
        updateRecentEntries();
        return;
    }

    showExitDialog(activeVehicle);
}
