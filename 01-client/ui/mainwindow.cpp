#include "mainwindow.h"
#include "../hardware/hardwareinit.h"
#include "../utils/globalsignals.h"
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
#define CAPTURE_DIR_PATH "/run/media/mmcblk1p1/captures"

namespace {

QPixmap resolveCapturePixmap(const QPixmap &memoryPixmap, const QString &filePath, const char *tag)
{
    if (!filePath.isEmpty()) {
        QPixmap fromFile;
        if (fromFile.load(filePath) && !fromFile.isNull()) {
            return fromFile;
        }
        qDebug() << tag << "文件加载失败，回退内存帧:" << filePath;
    }
    return memoryPixmap;
}

QPixmap loadCapturePixmapFromPath(const QString &filePath, const char *tag)
{
    if (filePath.isEmpty()) {
        return QPixmap();
    }
    QPixmap fromFile;
    if (fromFile.load(filePath) && !fromFile.isNull()) {
        return fromFile;
    }
    qDebug() << tag << "文件加载失败:" << filePath;
    return QPixmap();
}

}

MainWindow::MainWindow(HardwareInit *hardware, QWidget *parent)
    : QMainWindow(parent), m_db(nullptr), m_queryWindow(nullptr), m_settingsWindow(nullptr), m_exitDialog(nullptr), m_rechargeDialog(nullptr), m_rechargeCardLabel(nullptr), m_rechargeBalanceLabel(nullptr), m_rechargeStatusLabel(nullptr), m_rechargeCardId(), m_audioThread(nullptr), m_updateTimer(nullptr), m_videoRefreshTimer(nullptr), m_gateCloseTimer(nullptr), m_camera(nullptr), m_videoThread(nullptr), m_rfidThread(nullptr), m_networkClient(nullptr), m_waitingForResult(false), m_pendingRfidCard(), m_pendingRfidTime(), m_capturePurpose(CapturePurpose::None), m_pendingExitVehicle(), m_pendingExitRecognizedPlate(), m_pendingExitImagePath(), m_plateCooldowns(), m_pendingExitPlateNumber(), m_pendingExitEntryTime(), m_pendingExitFee(0.0), m_lastCapturePixmap(), m_recognitionBlockedUntil(), m_gateOpenUntil(), m_gateOpenReason(), m_videoPreviewActive(false), m_exitCheckoutSucceeded(false), m_hardware(hardware), m_todayEntryLabel(nullptr), m_todayExitLabel(nullptr), m_todayRevenueLabel(nullptr)
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
    else if (!ensureCaptureStorageReady())
    {
        qDebug() << "抓拍目录未就绪，入场/出场留证可能无法写入SD卡";
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
    connect(m_exitDialog, &ExitDialog::manualPassRequested, this, &MainWindow::onExitManualPass);
    connect(m_exitDialog, &ExitDialog::retryRecognizeRequested, this, &MainWindow::onExitRetryRecognize);
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

    QPushButton *rechargeBtn = new QPushButton("刷卡充值");
    rechargeBtn->setStyleSheet(QString(
                                   "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %1, stop:1 #2f63a0); "
                                   "color: white;")
                                   .arg("#27548a"));

    connect(queryBtn, &QPushButton::clicked, this, &MainWindow::onQueryClicked);
    connect(rechargeBtn, &QPushButton::clicked, this, &MainWindow::onRechargeClicked);
    connect(settingsBtn, &QPushButton::clicked, this, &MainWindow::onSettingsClicked);

    layout->addStretch();
    layout->addWidget(queryBtn);
    layout->addWidget(rechargeBtn);
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

void MainWindow::onRechargeClicked()
{
    if (!m_db || !m_db->isOpen()) {
        QMessageBox::warning(this, "提示", "数据库未打开");
        return;
    }
    if (m_exitDialog && m_exitDialog->isVisible()) {
        QMessageBox::information(this, "提示", "当前有车辆待出场结算，请稍后再充值");
        return;
    }

    ensureRechargeDialog();
    m_rechargeCardId.clear();
    if (m_rechargeCardLabel) {
        m_rechargeCardLabel->setText("--");
    }
    if (m_rechargeBalanceLabel) {
        m_rechargeBalanceLabel->setText("--");
    }
    if (m_rechargeStatusLabel) {
        m_rechargeStatusLabel->setText("请刷卡后选择充值金额");
    }

    m_rechargeDialog->show();
    m_rechargeDialog->raise();
    m_rechargeDialog->activateWindow();
}

void MainWindow::onSettingsClicked()
{
    m_settingsWindow->show();
    m_settingsWindow->raise();
    m_settingsWindow->activateWindow();
}

void MainWindow::ensureRechargeDialog()
{
    if (m_rechargeDialog) {
        return;
    }

    m_rechargeDialog = new QDialog(this);
    m_rechargeDialog->setWindowTitle("刷卡充值");
    m_rechargeDialog->setModal(false);
    m_rechargeDialog->setFixedSize(460, 260);
    m_rechargeDialog->setStyleSheet(
        "QDialog { background-color: #1a1a2e; color: white; }"
        "QLabel { color: white; font-size: 14px; }"
        "QPushButton { border: none; border-radius: 5px; padding: 8px 18px; font-size: 14px; font-weight: bold; }"
    );

    QVBoxLayout *layout = new QVBoxLayout(m_rechargeDialog);
    layout->setContentsMargins(20, 16, 20, 16);
    layout->setSpacing(12);

    QLabel *tips = new QLabel("请先刷卡，系统会自动显示卡号和余额");
    tips->setStyleSheet("color: #a0a0a0;");
    layout->addWidget(tips);

    QHBoxLayout *cardRow = new QHBoxLayout();
    cardRow->addWidget(new QLabel("当前卡号:"));
    m_rechargeCardLabel = new QLabel("--");
    m_rechargeCardLabel->setStyleSheet("color: #4ecca3; font-weight: bold; font-size: 16px;");
    cardRow->addStretch();
    cardRow->addWidget(m_rechargeCardLabel);
    layout->addLayout(cardRow);

    QHBoxLayout *balanceRow = new QHBoxLayout();
    balanceRow->addWidget(new QLabel("当前余额:"));
    m_rechargeBalanceLabel = new QLabel("--");
    m_rechargeBalanceLabel->setStyleSheet("color: #ffd93d; font-weight: bold; font-size: 16px;");
    balanceRow->addStretch();
    balanceRow->addWidget(m_rechargeBalanceLabel);
    layout->addLayout(balanceRow);

    m_rechargeStatusLabel = new QLabel("请刷卡后选择充值金额");
    m_rechargeStatusLabel->setStyleSheet("color: #a0a0a0;");
    layout->addWidget(m_rechargeStatusLabel);

    QHBoxLayout *amountRow = new QHBoxLayout();
    QPushButton *btn10 = new QPushButton("¥10");
    QPushButton *btn20 = new QPushButton("¥20");
    QPushButton *btn50 = new QPushButton("¥50");
    QPushButton *btn100 = new QPushButton("¥100");
    const QString amountBtnStyle = "background: #27548a; color: white;";
    btn10->setStyleSheet(amountBtnStyle);
    btn20->setStyleSheet(amountBtnStyle);
    btn50->setStyleSheet(amountBtnStyle);
    btn100->setStyleSheet(amountBtnStyle);
    amountRow->addWidget(btn10);
    amountRow->addWidget(btn20);
    amountRow->addWidget(btn50);
    amountRow->addWidget(btn100);
    layout->addLayout(amountRow);

    QHBoxLayout *bottomRow = new QHBoxLayout();
    bottomRow->addStretch();
    QPushButton *closeBtn = new QPushButton("关闭");
    closeBtn->setStyleSheet("background: #444; color: white;");
    bottomRow->addWidget(closeBtn);
    layout->addLayout(bottomRow);

    connect(btn10, &QPushButton::clicked, this, [this]() { rechargeByAmount(10.0); });
    connect(btn20, &QPushButton::clicked, this, [this]() { rechargeByAmount(20.0); });
    connect(btn50, &QPushButton::clicked, this, [this]() { rechargeByAmount(50.0); });
    connect(btn100, &QPushButton::clicked, this, [this]() { rechargeByAmount(100.0); });
    connect(closeBtn, &QPushButton::clicked, m_rechargeDialog, &QDialog::close);
}

void MainWindow::updateRechargeDialogInfo(const QString &cardId, double balance)
{
    if (!m_rechargeDialog || !m_rechargeDialog->isVisible()) {
        return;
    }
    m_rechargeCardId = cardId;
    if (m_rechargeCardLabel) {
        m_rechargeCardLabel->setText(cardId);
    }
    if (m_rechargeBalanceLabel) {
        m_rechargeBalanceLabel->setText(QString("¥%1").arg(balance, 0, 'f', 2));
    }
    if (m_rechargeStatusLabel) {
        m_rechargeStatusLabel->setText("请选择充值金额");
    }
}

void MainWindow::rechargeByAmount(double amount)
{
    if (!m_db || !m_db->isOpen() || !m_rechargeDialog || !m_rechargeDialog->isVisible()) {
        return;
    }
    if (m_rechargeCardId.isEmpty()) {
        if (m_rechargeStatusLabel) {
            m_rechargeStatusLabel->setText("请先刷卡");
            m_rechargeStatusLabel->setStyleSheet("color: #ffd93d;");
        }
        return;
    }
    if (!m_db->rechargeCard(m_rechargeCardId, amount)) {
        if (m_rechargeStatusLabel) {
            m_rechargeStatusLabel->setText(QString("充值失败: %1").arg(m_db->lastError()));
            m_rechargeStatusLabel->setStyleSheet("color: #ff6b6b;");
        }
        return;
    }
    const double newBalance = m_db->getCardBalance(m_rechargeCardId);
    updateRechargeDialogInfo(m_rechargeCardId, newBalance);
    if (m_rechargeStatusLabel) {
        m_rechargeStatusLabel->setText(
            QString("充值成功: +¥%1，当前余额 ¥%2").arg(amount, 0, 'f', 2).arg(newBalance, 0, 'f', 2));
        m_rechargeStatusLabel->setStyleSheet("color: #4ecca3;");
    }
}

bool MainWindow::isPendingRfidValid() const
{
    return !m_pendingRfidCard.isEmpty() &&
           m_pendingRfidTime.isValid() &&
           m_pendingRfidTime.msecsTo(QDateTime::currentDateTime()) <= RFID_VALID_WINDOW_MS;
}

void MainWindow::clearPendingRfid()
{
    m_pendingRfidCard.clear();
    m_pendingRfidTime = QDateTime();
}

void MainWindow::clearPendingExitFlow()
{
    m_capturePurpose = CapturePurpose::None;
    m_pendingExitVehicle = VehicleInfo();
    m_pendingExitRecognizedPlate.clear();
    m_pendingExitImagePath.clear();
    m_pendingExitPlateNumber.clear();
    m_pendingExitEntryTime = QDateTime();
    m_pendingExitFee = 0.0;
    m_exitCheckoutSucceeded = false;
}

QString MainWindow::normalizePlate(const QString &plateNumber)
{
    return plateNumber.trimmed().toUpper().remove(QLatin1Char(' '));
}

QString MainWindow::captureDirectoryPath() const
{
    return QString::fromUtf8(CAPTURE_DIR_PATH);
}

bool MainWindow::ensureCaptureStorageReady() const
{
    const QString sdRoot = QString::fromUtf8("/run/media/mmcblk1p1");
    if (!QDir(sdRoot).exists()) {
        qDebug() << "SD卡未挂载，无法准备抓拍目录:" << sdRoot;
        return false;
    }

    QDir dir(captureDirectoryPath());
    if (!dir.exists() && !dir.mkpath(".")) {
        qDebug() << "创建抓拍目录失败:" << dir.absolutePath();
        return false;
    }

    QFile probe(dir.filePath(QStringLiteral(".write_probe")));
    if (!probe.open(QIODevice::WriteOnly)) {
        qDebug() << "抓拍目录不可写:" << dir.absolutePath() << probe.errorString();
        return false;
    }
    probe.write("ok");
    probe.close();
    probe.remove();
    qDebug() << "抓拍目录就绪:" << dir.absolutePath();
    return true;
}

QString MainWindow::saveVehicleCaptureImage(int vehicleId, const QString &tag, const QPixmap &pixmap) const
{
    if (vehicleId <= 0 || pixmap.isNull()) {
        qDebug() << "抓拍保存跳过: vehicleId=" << vehicleId << "pixmapNull=" << pixmap.isNull();
        return QString();
    }

    if (!ensureCaptureStorageReady()) {
        return QString();
    }

    const QString baseName = QString("%1_%2_%3")
                                 .arg(tag)
                                 .arg(vehicleId)
                                 .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz"));
    const QImage image = pixmap.toImage();
    if (image.isNull()) {
        qDebug() << "抓拍图像转换失败:" << baseName;
        return QString();
    }

    struct FormatTry {
        const char *extension;
        const char *format;
        int quality;
    };
    static const FormatTry kFormats[] = {
        {"jpg", "JPEG", 90},
        {"png", "PNG", -1},
        {"bmp", "BMP", -1},
    };

    QDir dir(captureDirectoryPath());
    for (const FormatTry &fmt : kFormats) {
        const QString fullPath = dir.filePath(QString("%1.%2").arg(baseName, fmt.extension));
        const bool saved = (fmt.quality > 0)
                               ? image.save(fullPath, fmt.format, fmt.quality)
                               : image.save(fullPath, fmt.format);
        if (saved && QFileInfo::exists(fullPath) && QFileInfo(fullPath).size() > 0) {
            qDebug() << "抓拍已保存:" << fullPath << "format=" << fmt.format;
            return fullPath;
        }
        qDebug() << "抓拍保存失败:" << fullPath << "format=" << fmt.format;
    }

    return QString();
}

bool MainWindow::triggerCaptureForRecognition()
{
    if (m_waitingForResult)
    {
        qDebug() << "识别进行中，跳过抓拍";
        return false;
    }

    if (m_recognitionBlockedUntil.isValid() &&
        QDateTime::currentDateTime() < m_recognitionBlockedUntil)
    {
        qDebug() << "闸门开启冷却中，跳过抓拍";
        return false;
    }

    if (m_exitDialog && m_exitDialog->isVisible() &&
        m_capturePurpose != CapturePurpose::Exit &&
        m_capturePurpose != CapturePurpose::InvalidExit)
    {
        return false;
    }

    if ((m_queryWindow && m_queryWindow->isVisible()) ||
        (m_settingsWindow && m_settingsWindow->isVisible()))
    {
        qDebug() << "子窗口打开中，跳过抓拍";
        return false;
    }

    if (!m_networkClient || !m_networkClient->isConnected())
    {
        qDebug() << "识别服务器未连接，无法抓拍识别";
        return false;
    }

    if (!m_videoThread || !m_camera || m_camera->state() != V4L2Camera::StateStreaming)
    {
        qDebug() << "摄像头未就绪，无法抓拍";
        return false;
    }

    m_waitingForResult = true;
    m_videoThread->triggerCapture();
    return true;
}

void MainWindow::onRfidCardDetected(const QString &cardId)
{
    if (!m_db || !m_db->isOpen())
    {
        qDebug() << "RFID事件被忽略，数据库未打开";
        return;
    }

    // 先广播刷卡事件，供查询窗口等业务场景实时响应（如充值卡号自动填入）。
    emit g_signals->rfidCardDetected(cardId);

    if (m_rechargeDialog && m_rechargeDialog->isVisible())
    {
        if (!m_db->ensureCardAccount(cardId))
        {
            qDebug() << "RFID账户初始化失败, 卡号:" << cardId << "错误:" << m_db->lastError();
            return;
        }
        const double balance = m_db->getCardBalance(cardId);
        updateRechargeDialogInfo(cardId, balance >= 0 ? balance : 0.0);
        qDebug() << "充值模式收到刷卡, 卡号:" << cardId << "余额:" << balance;
        return;
    }

    if (m_waitingForResult &&
        m_capturePurpose != CapturePurpose::Exit &&
        m_capturePurpose != CapturePurpose::DuplicateEntry &&
        m_capturePurpose != CapturePurpose::InvalidExit)
    {
        qDebug() << "识别进行中，忽略刷卡, 卡号:" << cardId;
        return;
    }

    if (m_exitDialog && m_exitDialog->isVisible())
    {
        if (m_db->queryActiveVehicleByRfid(cardId).id == 0)
        {
            m_pendingRfidCard = cardId;
            m_pendingRfidTime = QDateTime::currentDateTime();
            m_capturePurpose = CapturePurpose::InvalidExit;
            qDebug() << "未入场却出场刷卡, 卡号:" << cardId;
            if (!triggerCaptureForRecognition())
            {
                clearPendingRfid();
                m_capturePurpose = CapturePurpose::None;
            }
        }
        return;
    }

    // 出场：刷卡 -> 抓拍 -> 识别车牌须与绑定车牌一致
    const VehicleInfo parkedByCard = m_db->queryActiveVehicleByRfid(cardId);
    if (parkedByCard.id != 0)
    {
        if (parkedByCard.rfidCard != cardId)
        {
            return;
        }

        const bool gateOpenForEntry = m_gateOpenUntil.isValid() &&
                                      QDateTime::currentDateTime() < m_gateOpenUntil &&
                                      m_gateOpenReason.contains(QString::fromUtf8("入场"));
        if (gateOpenForEntry)
        {
            m_pendingRfidCard = cardId;
            m_pendingRfidTime = QDateTime::currentDateTime();
            m_capturePurpose = CapturePurpose::DuplicateEntry;
            qDebug() << "重复入场刷卡, 卡号:" << cardId << "车牌:" << parkedByCard.plateNumber;
            if (!triggerCaptureForRecognition())
            {
                clearPendingRfid();
                m_capturePurpose = CapturePurpose::None;
            }
            return;
        }

        if (!m_db->ensureCardAccount(cardId))
        {
            qDebug() << "RFID账户初始化失败, 卡号:" << cardId << "错误:" << m_db->lastError();
            return;
        }
        const double balance = m_db->getCardBalance(cardId);
        updateRechargeDialogInfo(cardId, balance >= 0 ? balance : 0.0);

        m_pendingExitVehicle = parkedByCard;
        m_pendingExitPlateNumber = parkedByCard.plateNumber;
        m_pendingExitEntryTime = parkedByCard.entryTime;
        m_pendingRfidCard = cardId;
        m_pendingRfidTime = QDateTime::currentDateTime();
        m_capturePurpose = CapturePurpose::Exit;
        qDebug() << "出场刷卡触发抓拍核验, 卡号:" << cardId << "绑定车牌:" << parkedByCard.plateNumber;

        if (!triggerCaptureForRecognition())
        {
            clearPendingExitFlow();
            clearPendingRfid();
        }
        return;
    }

    // 入场：刷卡 -> 抓拍 -> 识别 -> 绑定入库
    if (m_recognitionBlockedUntil.isValid() &&
        QDateTime::currentDateTime() < m_recognitionBlockedUntil)
    {
        qDebug() << "闸门冷却中，忽略入场刷卡";
        return;
    }

    if (!m_db->ensureCardAccount(cardId))
    {
        qDebug() << "RFID账户初始化失败, 卡号:" << cardId << "错误:" << m_db->lastError();
        return;
    }
    const double balance = m_db->getCardBalance(cardId);
    updateRechargeDialogInfo(cardId, balance >= 0 ? balance : 0.0);

    clearPendingExitFlow();
    m_capturePurpose = CapturePurpose::Entry;
    m_pendingRfidCard = cardId;
    m_pendingRfidTime = QDateTime::currentDateTime();
    qDebug() << "入场刷卡触发抓拍, 卡号:" << cardId << "余额:" << balance;

    if (!triggerCaptureForRecognition())
    {
        clearPendingRfid();
        m_capturePurpose = CapturePurpose::None;
    }
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
    qDebug() << "出场结算页倒计时结束，自动关闭";
    finishExitAfterSettlement();
}

void MainWindow::onGateCloseTimeout()
{
    setGateOpened(false);
}

// ==================== 视频相关槽函数 ====================

void MainWindow::refreshVideoFrame()
{
    if (!m_videoThread || !m_videoLabel)
        return;

    const QImage frame = m_videoThread->takeLatestFrame();
    if (frame.isNull())
        return;

    if (!m_videoPreviewActive) {
        m_videoPreviewActive = true;
        m_videoLabel->setText(QString());
    }

    const QSize labelSize = m_videoLabel->size();
    if (labelSize.width() <= 0 || labelSize.height() <= 0)
        return;

    const QPixmap scaled = QPixmap::fromImage(frame).scaled(
        labelSize, Qt::KeepAspectRatio, Qt::FastTransformation);
    if (scaled.isNull())
        return;

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
        QString errorMsg = result.errorMessage.isEmpty() ? "车牌识别失败，请重试" : result.errorMessage;
        qDebug() << "识别失败, 模式:" << static_cast<int>(m_capturePurpose)
                 << "RFID:" << m_pendingRfidCard << "错误:" << errorMsg;

        if (m_capturePurpose == CapturePurpose::Exit && m_pendingExitVehicle.id != 0)
        {
            if (m_hardware)
            {
                m_hardware->alarm(3, 150);
            }
            setGateOpened(false);
            showExitVerifyDialog(
                m_pendingExitVehicle,
                QString(),
                false,
                QString("出场识别失败: %1，请重新识别或再次刷卡").arg(errorMsg));
        }
        else if (m_capturePurpose == CapturePurpose::DuplicateEntry)
        {
            rejectRfidWithBlacklist(QString(), QString::fromUtf8("重复入场刷卡(识别失败)"));
        }
        else if (m_capturePurpose == CapturePurpose::InvalidExit)
        {
            rejectRfidWithBlacklist(QString(), QString::fromUtf8("未入场却出场刷卡(识别失败)"));
        }
        else
        {
            clearPendingRfid();
            m_capturePurpose = CapturePurpose::None;
        }
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
    if (m_capturePurpose == CapturePurpose::Entry)
    {
        clearPendingRfid();
        m_capturePurpose = CapturePurpose::None;
    }
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

bool MainWindow::completeExitCheckout(const QString &reasonTag)
{
    if (!m_db || m_pendingExitVehicle.id == 0 || m_pendingRfidCard.isEmpty())
    {
        return false;
    }

    if (m_pendingExitImagePath.isEmpty() && !m_lastCapturePixmap.isNull())
    {
        m_pendingExitImagePath = saveVehicleCaptureImage(m_pendingExitVehicle.id, "exit", m_lastCapturePixmap);
        if (m_pendingExitImagePath.isEmpty())
        {
            qDebug() << "出库前补存出场抓拍失败，数据库将不写入exit_image路径";
        }
    }

    const QString plate = m_pendingExitVehicle.plateNumber;
    const QString cardId = m_pendingRfidCard;

    if (!m_db->checkoutVehicle(plate, cardId, m_pendingExitImagePath))
    {
        qDebug() << "出场结算失败:" << reasonTag << plate << cardId << m_db->lastError();
        m_exitCheckoutSucceeded = false;
        if (reasonTag != QStringLiteral("auto")) {
            showExitVerifyDialog(
                m_pendingExitVehicle,
                m_pendingExitRecognizedPlate,
                true,
                QString("车牌已核验，但结算失败: %1").arg(m_db->lastError()));
        }
        return false;
    }

    m_exitCheckoutSucceeded = true;
    qDebug() << "出场成功:" << reasonTag << plate << "扣费约:" << m_pendingExitFee;
    markPlateCooldown(plate);
    updateParkingStatus();
    updateRecentEntries();

    if (reasonTag == QStringLiteral("auto")) {
        return true;
    }

    openGateForPassage(QString("车辆 %1 已出场").arg(plate));
    closeExitDialog(true);
    m_exitCheckoutSucceeded = false;
    return true;
}

void MainWindow::showExitSettlementDialog(const VehicleInfo &vehicleInfo, const QString &recognizedPlate)
{
    if (!m_exitDialog || !m_db) {
        return;
    }

    QDateTime now = QDateTime::currentDateTime();
    qint64 durationMinutes = (vehicleInfo.entryTime.secsTo(now) + 59) / 60;
    const int duration = qMax(1, static_cast<int>(durationMinutes));
    const double fee = m_db->calculateFee(vehicleInfo.entryTime, now);

    m_pendingExitPlateNumber = vehicleInfo.plateNumber;
    m_pendingExitEntryTime = vehicleInfo.entryTime;
    m_pendingExitFee = fee;

    const QPixmap entryPixmap = loadCapturePixmapFromPath(vehicleInfo.entryImagePath, "入场抓拍");
    const QPixmap exitPixmap = resolveCapturePixmap(m_lastCapturePixmap, m_pendingExitImagePath, "出场抓拍");

    m_exitDialog->setSettlementMode(true);
    m_exitDialog->setCompareImages(entryPixmap, exitPixmap);
    m_exitDialog->setParkingInfo(vehicleInfo.plateNumber, vehicleInfo.entryTime, now, duration);
    m_exitDialog->setPlateVerifyInfo(vehicleInfo.plateNumber, recognizedPlate, true);
    m_exitDialog->setFeeInfo(fee, duration, 0.1);

    const double balanceBefore = m_db->getCardBalance(m_pendingRfidCard);
    m_exitDialog->setPaymentInfo(
        QString::fromUtf8("正在结算，请稍候..."),
        m_pendingRfidCard,
        balanceBefore);
    m_exitDialog->show();
    m_exitDialog->raise();
    m_exitDialog->activateWindow();

    if (completeExitCheckout(QStringLiteral("auto"))) {
        const double balanceAfter = m_db->getCardBalance(m_pendingRfidCard);
        m_exitDialog->setPaymentInfo(
            QString::fromUtf8("刷卡成功，已扣费 ¥%1，剩余余额 ¥%2")
                .arg(m_pendingExitFee, 0, 'f', 2)
                .arg(balanceAfter, 0, 'f', 2),
            m_pendingRfidCard,
            balanceAfter);
        m_exitDialog->startCountdown(10);
        return;
    }

    m_exitDialog->setSettlementMode(false);
    showExitVerifyDialog(
        m_pendingExitVehicle,
        recognizedPlate,
        true,
        QString::fromUtf8("车牌已核验，但结算失败: %1").arg(m_db->lastError()));
}

void MainWindow::finishExitAfterSettlement()
{
    if (m_exitCheckoutSucceeded && m_pendingExitVehicle.id != 0) {
        openGateForPassage(
            QString::fromUtf8("车辆 %1 已出场").arg(m_pendingExitVehicle.plateNumber));
    }
    m_exitCheckoutSucceeded = false;
    closeExitDialog(true);
}

void MainWindow::showExitVerifyDialog(const VehicleInfo &vehicleInfo, const QString &recognizedPlate,
                                      bool plateMatched, const QString &statusText)
{
    if (!m_exitDialog || !m_db)
    {
        return;
    }

    m_exitDialog->setSettlementMode(false);
    m_exitDialog->setVerifyMode(!plateMatched);

    QDateTime now = QDateTime::currentDateTime();
    qint64 durationMinutes = (vehicleInfo.entryTime.secsTo(now) + 59) / 60;
    int duration = qMax(1, static_cast<int>(durationMinutes));
    double fee = m_db->calculateFee(vehicleInfo.entryTime, now);

    m_pendingExitPlateNumber = vehicleInfo.plateNumber;
    m_pendingExitEntryTime = vehicleInfo.entryTime;
    m_pendingExitFee = fee;

    const QPixmap entryPixmap = loadCapturePixmapFromPath(vehicleInfo.entryImagePath, "入场抓拍");
    const QPixmap exitPixmap = resolveCapturePixmap(m_lastCapturePixmap, m_pendingExitImagePath, "出场抓拍");

    m_exitDialog->setCompareImages(entryPixmap, exitPixmap);
    m_exitDialog->setParkingInfo(vehicleInfo.plateNumber, vehicleInfo.entryTime, now, duration);
    m_exitDialog->setPlateVerifyInfo(vehicleInfo.plateNumber, recognizedPlate, plateMatched);
    m_exitDialog->setFeeInfo(fee, duration, 0.1);

    const double balance = m_db->getCardBalance(m_pendingRfidCard);
    m_exitDialog->setPaymentInfo(statusText, m_pendingRfidCard, balance);
    m_exitDialog->show();
    m_exitDialog->raise();
    m_exitDialog->activateWindow();

    if (!plateMatched)
    {
        setGateOpened(false);
    }
}

void MainWindow::closeExitDialog(bool clearPending)
{
    if (m_exitDialog && m_exitDialog->isVisible())
    {
        m_exitDialog->stopCountdown();
        m_exitDialog->hide();
    }

    if (clearPending)
    {
        clearPendingRfid();
        clearPendingExitFlow();
    }
}

void MainWindow::onExitManualPass()
{
    qDebug() << "人工核查放行, 车牌:" << m_pendingExitVehicle.plateNumber;
    completeExitCheckout("manual");
}

void MainWindow::onExitRetryRecognize()
{
    if (m_pendingExitVehicle.id == 0 || m_pendingRfidCard.isEmpty())
    {
        return;
    }

    m_capturePurpose = CapturePurpose::Exit;
    m_pendingExitRecognizedPlate.clear();
    qDebug() << "重新识别出场车牌, 绑定:" << m_pendingExitVehicle.plateNumber;

    if (!triggerCaptureForRecognition() && m_exitDialog)
    {
        m_exitDialog->setPaymentInfo("重新抓拍失败，请检查摄像头与识别服务", m_pendingRfidCard, -1);
    }
}

void MainWindow::processRecognitionResult(const QString &plateNumber, double confidence)
{
    Q_UNUSED(confidence);

    if (!m_db || !m_db->isOpen())
    {
        clearPendingRfid();
        clearPendingExitFlow();
        return;
    }

    if (m_capturePurpose == CapturePurpose::Exit)
    {
        processExitRecognitionResult(plateNumber);
        return;
    }

    if (m_capturePurpose == CapturePurpose::DuplicateEntry)
    {
        rejectRfidWithBlacklist(plateNumber, QString::fromUtf8("重复入场刷卡"));
        return;
    }

    if (m_capturePurpose == CapturePurpose::InvalidExit)
    {
        rejectRfidWithBlacklist(plateNumber, QString::fromUtf8("未入场却出场刷卡"));
        return;
    }

    if (isPendingRfidValid() && m_capturePurpose == CapturePurpose::Entry)
    {
        processEntryRecognitionResult(plateNumber);
    }
}

void MainWindow::rejectRfidWithBlacklist(const QString &plateNumber, const QString &reason)
{
    if (m_hardware)
    {
        m_hardware->alarm();
    }

    const QString plate = normalizePlate(plateNumber);
    if (!plate.isEmpty() && m_db)
    {
        m_db->addBlacklistPlate(plate, reason);
        qDebug() << "异常刷卡已拉黑, 车牌:" << plate << "原因:" << reason;
    }
    else
    {
        qDebug() << "异常刷卡(未识别到车牌), 原因:" << reason;
    }

    if (!plate.isEmpty())
    {
        markPlateCooldown(plate);
    }
    clearPendingRfid();
    clearPendingExitFlow();
    m_capturePurpose = CapturePurpose::None;
}

void MainWindow::processEntryRecognitionResult(const QString &plateNumber)
{
    if (isPlateInCooldown(plateNumber))
    {
        clearPendingRfid();
        m_capturePurpose = CapturePurpose::None;
        return;
    }

    const QString rfidCard = m_pendingRfidCard;

    if (m_db->queryActiveVehicleByRfid(rfidCard).id != 0)
    {
        qDebug() << "该卡已入场，请勿重复刷卡, 卡号:" << rfidCard;
        rejectRfidWithBlacklist(plateNumber, QString::fromUtf8("重复入场刷卡"));
        return;
    }

    if (m_db->queryActiveVehicle(plateNumber).id != 0)
    {
        qDebug() << "车牌已在场，忽略本次入场识别, 车牌:" << plateNumber;
        rejectRfidWithBlacklist(plateNumber, QString::fromUtf8("重复入场(车牌已在场)"));
        return;
    }

    if (m_db->isIllegalVehicle(plateNumber))
    {
        if (m_hardware)
        {
            m_hardware->alarm();
        }
        qDebug() << "入场被拒绝, 黑名单车辆:" << plateNumber;
        markPlateCooldown(plateNumber);
        clearPendingRfid();
        m_capturePurpose = CapturePurpose::None;
        return;
    }

    if (m_db->getAvailableSpaces() <= 0)
    {
        if (m_hardware)
        {
            m_hardware->alarm();
        }
        if (m_networkClient && m_networkClient->isConnected())
        {
            const QString audioName = QString("parking_full_%1.wav")
                                          .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz"));
            m_networkClient->requestAudio("parking_full", "停车场车位已满，请勿入场", audioName);
        }
        qDebug() << "入场被拒绝, 停车场已满, 车牌:" << plateNumber;
        markPlateCooldown(plateNumber);
        clearPendingRfid();
        m_capturePurpose = CapturePurpose::None;
        return;
    }

    int recordId = m_db->addVehicle(plateNumber, rfidCard);
    if (recordId < 0)
    {
        qDebug() << "入场落库失败, 车牌:" << plateNumber << "RFID:" << rfidCard << m_db->lastError();
        clearPendingRfid();
        m_capturePurpose = CapturePurpose::None;
        return;
    }

    QString entryImagePath = saveVehicleCaptureImage(recordId, "entry", m_lastCapturePixmap);
    if (!entryImagePath.isEmpty())
    {
        if (!m_db->updateVehicleEntryImage(recordId, entryImagePath))
        {
            qDebug() << "入场抓拍路径写入数据库失败:" << m_db->lastError();
        }
    }
    else
    {
        qDebug() << "入场抓拍未写入SD卡，entry_image将保持为空";
    }

    qDebug() << "入场成功, recordId:" << recordId << "车牌:" << plateNumber << "RFID:" << rfidCard;
    markPlateCooldown(plateNumber);
    openGateForPassage(QString("车辆 %1 入场").arg(plateNumber));
    clearPendingRfid();
    m_capturePurpose = CapturePurpose::None;
    updateParkingStatus();
    updateRecentEntries();
}

void MainWindow::processExitRecognitionResult(const QString &plateNumber)
{
    if (m_pendingExitVehicle.id == 0)
    {
        clearPendingExitFlow();
        clearPendingRfid();
        return;
    }

    m_pendingExitRecognizedPlate = plateNumber;
    m_pendingExitImagePath = saveVehicleCaptureImage(m_pendingExitVehicle.id, "exit", m_lastCapturePixmap);

    const QString boundPlate = normalizePlate(m_pendingExitVehicle.plateNumber);
    const QString recognized = normalizePlate(plateNumber);
    const bool matched = !recognized.isEmpty() && recognized == boundPlate;

    QDateTime now = QDateTime::currentDateTime();
    qint64 durationMinutes = (m_pendingExitVehicle.entryTime.secsTo(now) + 59) / 60;
    m_pendingExitFee = m_db->calculateFee(m_pendingExitVehicle.entryTime, now);

    if (matched)
    {
        qDebug() << "出场车牌核验通过, 车牌:" << plateNumber;
        showExitSettlementDialog(m_pendingExitVehicle, plateNumber);
        return;
    }

    qDebug() << "出场车牌不一致, 绑定:" << boundPlate << "识别:" << recognized;
    if (m_hardware)
    {
        m_hardware->alarm(5, 150);
    }
    setGateOpened(false);

    showExitVerifyDialog(
        m_pendingExitVehicle,
        plateNumber,
        false,
        "车牌与绑定信息不符，闸机已锁定，请人工核对入出场抓拍图");
}
