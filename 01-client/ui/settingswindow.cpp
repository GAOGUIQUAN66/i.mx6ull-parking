#include "settingswindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QDebug>

// 颜色定义
#define COLOR_BG_DARK "#1a1a2e"
#define COLOR_BG_PANEL "#16213e"
#define COLOR_ACCENT "#e94560"
#define COLOR_SUCCESS "#4ecca3"
#define COLOR_WARNING "#ffd93d"
#define COLOR_TEXT_GRAY "#a0a0a0"

SettingsWindow::SettingsWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_testSocket(nullptr)
{
    setWindowTitle("系统设置");
    setFixedSize(1024, 600);
    setStyleSheet(QString(
        "QMainWindow { background-color: %1; }"
        "QLabel { color: white; }"
        "QPushButton { "
        " border: none; "
        " border-radius: 5px; "
        " padding: 10px 25px; "
        " font-size: 14px; "
        " font-weight: bold; "
        "}"
        "QPushButton:hover { opacity: 0.9; }"
        "QLineEdit, QSpinBox { "
        " padding: 12px; "
        " font-size: 16px; "
        " background: #0f0f23; "
        " border: 1px solid #333; "
        " border-radius: 5px; "
        " color: white; "
        "}"
        "QLineEdit:focus, QSpinBox:focus { border-color: %2; }"
        "QSpinBox::up-button, QSpinBox::down-button { width: 20px; }")
        .arg(COLOR_BG_DARK, COLOR_SUCCESS));

    setupUI();
}

SettingsWindow::~SettingsWindow()
{
    if (m_testSocket) {
        m_testSocket->abort();
        delete m_testSocket;
    }
}

void SettingsWindow::setupUI()
{
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // 顶部栏
    QWidget *header = new QWidget();
    header->setFixedHeight(50);
    header->setStyleSheet(QString("background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %1, stop:1 #0f3460); border-bottom: 2px solid %2;").arg(COLOR_BG_PANEL, COLOR_ACCENT));

    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(20, 0, 20, 0);

    QLabel *titleLabel = new QLabel("系统设置 - 网络配置");
    titleLabel->setStyleSheet(QString("font-size: 20px; font-weight: bold; color: %1;").arg(COLOR_ACCENT));

    QPushButton *backBtn = new QPushButton("← 返回主界面");
    backBtn->setStyleSheet(QString("background: %1; color: white;").arg(COLOR_ACCENT));
    connect(backBtn, &QPushButton::clicked, this, &SettingsWindow::onBackClicked);

    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(backBtn);
    mainLayout->addWidget(header);

    // 主内容区
    QWidget *contentArea = new QWidget();
    contentArea->setStyleSheet("padding: 30px;");
    QVBoxLayout *contentLayout = new QVBoxLayout(contentArea);
    contentLayout->setSpacing(20);

    // 网络设置组
    QGroupBox *networkGroup = new QGroupBox();
    networkGroup->setStyleSheet(QString(
        "QGroupBox { "
        " background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %1, stop:1 #0f3460); "
        " border-radius: 10px; "
        " border: 1px solid #333; "
        " padding: 25px; "
        "}")
        .arg(COLOR_BG_PANEL));

    QVBoxLayout *networkLayout = new QVBoxLayout(networkGroup);
    networkLayout->setSpacing(18);

    QLabel *networkTitle = new QLabel("网络设置");
    networkTitle->setStyleSheet(QString("font-size: 18px; font-weight: bold; color: %1; border-bottom: 1px solid #333; padding-bottom: 10px;").arg(COLOR_SUCCESS));

    // 上位机IP
    QHBoxLayout *ipRow = new QHBoxLayout();
    QLabel *ipLabel = new QLabel("上位机IP:");
    ipLabel->setFixedWidth(140);
    ipLabel->setStyleSheet(QString("font-size: 16px; color: %1;").arg(COLOR_TEXT_GRAY));
    m_serverIpEdit = new QLineEdit("192.168.137.50");
    m_serverIpEdit->setFixedWidth(300);
    ipRow->addWidget(ipLabel);
    ipRow->addWidget(m_serverIpEdit);
    ipRow->addStretch();

    // 服务器端口
    QHBoxLayout *portRow = new QHBoxLayout();
    QLabel *portLabel = new QLabel("服务器端口:");
    portLabel->setFixedWidth(140);
    portLabel->setStyleSheet(QString("font-size: 16px; color: %1;").arg(COLOR_TEXT_GRAY));
    m_portSpin = new QSpinBox();
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(8888);
    m_portSpin->setFixedWidth(300);
    portRow->addWidget(portLabel);
    portRow->addWidget(m_portSpin);
    portRow->addStretch();

    // 连接状态
    QHBoxLayout *statusRow = new QHBoxLayout();
    QLabel *statusLabel = new QLabel("连接状态:");
    statusLabel->setFixedWidth(140);
    statusLabel->setStyleSheet(QString("font-size: 16px; color: %1;").arg(COLOR_TEXT_GRAY));

    QHBoxLayout *statusIndicator = new QHBoxLayout();
    m_statusDot = new QLabel();
    m_statusDot->setFixedSize(14, 14);
    m_statusDot->setStyleSheet(QString("background: %1; border-radius: 7px;").arg(COLOR_ACCENT));
    m_statusText = new QLabel("未连接");
    m_statusText->setStyleSheet(QString("font-size: 16px; color: %1;").arg(COLOR_ACCENT));

    m_testBtn = new QPushButton("测试连接");
    m_testBtn->setStyleSheet(QString("background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %1, stop:1 #f0c419); color: #000;").arg(COLOR_WARNING));
    connect(m_testBtn, &QPushButton::clicked, this, &SettingsWindow::onTestConnection);

    statusIndicator->addWidget(m_statusDot);
    statusIndicator->addWidget(m_statusText);
    statusIndicator->addSpacing(20);
    statusIndicator->addWidget(m_testBtn);

    statusRow->addWidget(statusLabel);
    statusRow->addLayout(statusIndicator);
    statusRow->addStretch();

    networkLayout->addWidget(networkTitle);
    networkLayout->addLayout(ipRow);
    networkLayout->addLayout(portRow);
    networkLayout->addLayout(statusRow);

    contentLayout->addWidget(networkGroup);
    contentLayout->addStretch();

    // 底部按钮
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(20);

    btnLayout->addStretch();

    m_defaultBtn = new QPushButton("恢复默认");
    m_defaultBtn->setStyleSheet("background: #333; border: 1px solid #555; color: white; padding: 15px 50px; font-size: 18px; border-radius: 10px;");
    connect(m_defaultBtn, &QPushButton::clicked, this, &SettingsWindow::onDefaultClicked);

    m_saveBtn = new QPushButton("保存设置");
    m_saveBtn->setStyleSheet(QString(
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %1, stop:1 #3db892); "
        "color: white; padding: 15px 50px; font-size: 18px; border-radius: 10px;")
        .arg(COLOR_SUCCESS));
    connect(m_saveBtn, &QPushButton::clicked, this, &SettingsWindow::onSaveClicked);

    btnLayout->addWidget(m_defaultBtn);
    btnLayout->addWidget(m_saveBtn);
    btnLayout->addStretch();

    contentLayout->addLayout(btnLayout);

    mainLayout->addWidget(contentArea, 1);
    setCentralWidget(centralWidget);
}

void SettingsWindow::onTestConnection()
{
    QString host = m_serverIpEdit->text().trimmed();
    quint16 port = static_cast<quint16>(m_portSpin->value());

    // 禁用按钮，显示测试中状态
    m_testBtn->setEnabled(false);
    m_testBtn->setText("测试中...");
    m_statusText->setText("正在连接...");

    // 清理旧的测试socket
    if (m_testSocket) {
        m_testSocket->abort();
        m_testSocket->deleteLater();
    }

    m_testSocket = new QTcpSocket(this);
    connect(m_testSocket, &QTcpSocket::connected, this, &SettingsWindow::onTestConnected);
    // Qt5.7兼容：使用error信号
    connect(m_testSocket, static_cast<void(QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error),
            this, &SettingsWindow::onTestError);

    // 设置超时（5秒）
    QTimer::singleShot(5000, this, [this]() {
        if (m_testSocket && m_testSocket->state() != QAbstractSocket::ConnectedState) {
            m_testSocket->abort();
            m_testBtn->setEnabled(true);
            m_testBtn->setText("测试连接");
            m_statusDot->setStyleSheet(QString("background: %1; border-radius: 7px;").arg(COLOR_ACCENT));
            m_statusText->setText("连接超时");
            m_statusText->setStyleSheet(QString("font-size: 16px; color: %1;").arg(COLOR_ACCENT));
        }
    });

    qDebug() << "测试TCP连接:" << host << ":" << port;
    m_testSocket->connectToHost(host, port);
}

void SettingsWindow::onTestConnected()
{
    qDebug() << "TCP测试连接成功";
    m_testBtn->setEnabled(true);
    m_testBtn->setText("测试连接");
    updateConnectionStatus(true);

    if (m_testSocket) {
        m_testSocket->disconnectFromHost();
    }
}

void SettingsWindow::onTestError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    QString errorString = m_testSocket ? m_testSocket->errorString() : "未知错误";
    qDebug() << "TCP测试连接失败:" << errorString;

    m_testBtn->setEnabled(true);
    m_testBtn->setText("测试连接");

    m_statusDot->setStyleSheet(QString("background: %1; border-radius: 7px;").arg(COLOR_ACCENT));
    m_statusText->setText(QString("连接失败: %1").arg(errorString));
    m_statusText->setStyleSheet(QString("font-size: 14px; color: %1;").arg(COLOR_ACCENT));
}

void SettingsWindow::onSaveClicked()
{
    m_config.serverIp = m_serverIpEdit->text().trimmed();
    m_config.port = m_portSpin->value();

    emit configSaved(m_config);
    emit backToMain();
    hide();
}

void SettingsWindow::onDefaultClicked()
{
    m_serverIpEdit->setText("192.168.137.50");
    m_portSpin->setValue(8888);
}

void SettingsWindow::onBackClicked()
{
    emit backToMain();
    hide();
}

void SettingsWindow::updateConnectionStatus(bool connected)
{
    m_config.isConnected = connected;
    if (connected)
    {
        m_statusDot->setStyleSheet(QString("background: %1; border-radius: 7px;").arg(COLOR_SUCCESS));
        m_statusText->setText("已连接");
        m_statusText->setStyleSheet(QString("font-size: 16px; color: %1;").arg(COLOR_SUCCESS));
    }
    else
    {
        m_statusDot->setStyleSheet(QString("background: %1; border-radius: 7px;").arg(COLOR_ACCENT));
        m_statusText->setText("未连接");
        m_statusText->setStyleSheet(QString("font-size: 16px; color: %1;").arg(COLOR_ACCENT));
    }
}

void SettingsWindow::setActualConnectionStatus(bool connected)
{
    updateConnectionStatus(connected);
}

SettingsWindow::NetworkConfig SettingsWindow::getNetworkConfig() const
{
    return m_config;
}

void SettingsWindow::setNetworkConfig(const NetworkConfig &config)
{
    m_config = config;
    m_serverIpEdit->setText(config.serverIp);
    m_portSpin->setValue(config.port);
    updateConnectionStatus(config.isConnected);
}
