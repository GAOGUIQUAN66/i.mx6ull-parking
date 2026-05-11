#include "entrydialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QDateTime>

// 颜色定义
#define COLOR_BG_DARK "#1a1a2e"
#define COLOR_BG_PANEL "#16213e"
#define COLOR_ACCENT "#e94560"
#define COLOR_SUCCESS "#4ecca3"
#define COLOR_WARNING "#ffd93d"
#define COLOR_TEXT_GRAY "#a0a0a0"

EntryDialog::EntryDialog(QWidget *parent)
    : QDialog(parent)
    , m_countdownTimer(nullptr)
    , m_countdownValue(0)
{
    setWindowTitle("车辆入场确认");
    setFixedSize(700, 520);
    setStyleSheet(QString(
        "QDialog { background: %1; }"
        "QLabel { color: white; }"
        "QPushButton { "
        "   border: none; "
        "   border-radius: 10px; "
        "   padding: 15px 50px; "
        "   font-size: 18px; "
        "   font-weight: bold; "
        "}"
        "QPushButton:hover { opacity: 0.9; }"
        "QLineEdit { "
        "   padding: 12px; "
        "   font-size: 20px; "
        "   border: 2px solid #333; "
        "   border-radius: 8px; "
        "   background: #0f0f23; "
        "   color: %2; "
        "   letter-spacing: 3px; "
        "}"
        "QLineEdit:focus { border-color: %3; }"
        "QProgressBar { "
        "   border: none; "
        "   border-radius: 6px; "
        "   background: #333; "
        "   height: 12px; "
        "   text-align: center; "
        "}"
        "QProgressBar::chunk { "
        "   border-radius: 6px; "
        "   background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 %2, stop:1 #45b7aa); "
        "}"
    ).arg(COLOR_BG_DARK, COLOR_SUCCESS, COLOR_ACCENT));

    setupUI();
}

EntryDialog::~EntryDialog()
{
    if (m_countdownTimer) {
        m_countdownTimer->stop();
        delete m_countdownTimer;
    }
}

void EntryDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 15, 20, 15);

    // 标题
    QLabel *titleLabel = new QLabel("🚗 车辆入场确认");
    titleLabel->setStyleSheet(QString(
        "font-size: 22px; font-weight: bold; color: white; "
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %1, stop:1 #c73e54); "
        "padding: 15px; border-radius: 10px;"
    ).arg(COLOR_ACCENT));
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    // 主内容区
    QHBoxLayout *contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(20);

    // 左侧图片区
    QVBoxLayout *imageLayout = new QVBoxLayout();

    m_imageLabel = new QLabel();
    m_imageLabel->setFixedSize(300, 220);
    m_imageLabel->setStyleSheet(QString(
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %1, stop:1 %2); "
        "border: 2px solid #333; border-radius: 10px;"
    ).arg(COLOR_BG_DARK, COLOR_BG_PANEL));
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setText("📸\n\n抓拍图片预览");

    m_imageTimeLabel = new QLabel("入场时间: --");
    m_imageTimeLabel->setStyleSheet(QString("font-size: 14px; color: %1;").arg(COLOR_TEXT_GRAY));
    m_imageTimeLabel->setAlignment(Qt::AlignCenter);

    imageLayout->addWidget(m_imageLabel);
    imageLayout->addWidget(m_imageTimeLabel);
    contentLayout->addLayout(imageLayout);

    // 右侧信息区
    QVBoxLayout *infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(15);

    // 识别结果组
    QGroupBox *resultGroup = new QGroupBox();
    resultGroup->setStyleSheet(QString(
        "QGroupBox { background: rgba(255,255,255,0.05); border-radius: 10px; padding: 15px; border: none; }"
    ));

    QVBoxLayout *resultLayout = new QVBoxLayout(resultGroup);

    QLabel *resultTitle = new QLabel("识别结果");
    resultTitle->setStyleSheet(QString("font-size: 16px; font-weight: bold; color: %1; border-bottom: 1px solid #333; padding-bottom: 8px;").arg(COLOR_ACCENT));

    QHBoxLayout *plateRow = new QHBoxLayout();
    QLabel *plateLabelTxt = new QLabel("车牌号码");
    plateLabelTxt->setStyleSheet(QString("font-size: 14px; color: %1;").arg(COLOR_TEXT_GRAY));
    m_plateLabel = new QLabel("--");
    m_plateLabel->setStyleSheet(QString("font-size: 24px; font-weight: bold; color: %1; letter-spacing: 2px;").arg(COLOR_SUCCESS));
    plateRow->addWidget(plateLabelTxt);
    plateRow->addStretch();
    plateRow->addWidget(m_plateLabel);

    QHBoxLayout *rfidRow = new QHBoxLayout();
    QLabel *rfidLabelTxt = new QLabel("RFID卡号");
    rfidLabelTxt->setStyleSheet(QString("font-size: 14px; color: %1;").arg(COLOR_TEXT_GRAY));
    m_rfidLabel = new QLabel("--");
    m_rfidLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: white;");
    rfidRow->addWidget(rfidLabelTxt);
    rfidRow->addStretch();
    rfidRow->addWidget(m_rfidLabel);

    // 置信度
    QHBoxLayout *confRow = new QHBoxLayout();
    QLabel *confLabelTxt = new QLabel("识别置信度");
    confLabelTxt->setStyleSheet(QString("font-size: 12px; color: %1;").arg(COLOR_TEXT_GRAY));
    m_confidenceLabel = new QLabel("0%");
    m_confidenceLabel->setStyleSheet(QString("font-size: 12px; font-weight: bold; color: %1;").arg(COLOR_SUCCESS));
    m_confidenceBar = new QProgressBar();
    m_confidenceBar->setRange(0, 100);
    m_confidenceBar->setValue(0);
    m_confidenceBar->setFixedHeight(12);
    m_confidenceBar->setFormat("");

    confRow->addWidget(confLabelTxt);
    confRow->addStretch();
    confRow->addWidget(m_confidenceLabel);

    resultLayout->addWidget(resultTitle);
    resultLayout->addLayout(plateRow);
    resultLayout->addLayout(rfidRow);
    resultLayout->addSpacing(10);
    resultLayout->addLayout(confRow);
    resultLayout->addWidget(m_confidenceBar);

    infoLayout->addWidget(resultGroup);

    // 车牌修正
    QGroupBox *correctGroup = new QGroupBox();
    correctGroup->setStyleSheet("QGroupBox { background: rgba(255,255,255,0.05); border-radius: 10px; padding: 15px; border: none; }");

    QVBoxLayout *correctLayout = new QVBoxLayout(correctGroup);
    QLabel *correctLabel = new QLabel("车牌修正（如识别有误可手动修改）");
    correctLabel->setStyleSheet(QString("font-size: 14px; color: %1;").arg(COLOR_TEXT_GRAY));
    m_plateEdit = new QLineEdit();
    m_plateEdit->setPlaceholderText("请输入车牌号");
    m_plateEdit->setAlignment(Qt::AlignCenter);

    correctLayout->addWidget(correctLabel);
    correctLayout->addWidget(m_plateEdit);

    infoLayout->addWidget(correctGroup);
    contentLayout->addLayout(infoLayout, 1);

    mainLayout->addLayout(contentLayout);

    // 倒计时
    m_countdownLabel = new QLabel("");
    m_countdownLabel->setStyleSheet(QString("font-size: 14px; color: %1;").arg(COLOR_WARNING));
    m_countdownLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_countdownLabel);

    // 按钮区
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(20);

    m_confirmBtn = new QPushButton("✓ 确认入场");
    m_confirmBtn->setStyleSheet(QString(
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %1, stop:1 #3db892); color: white;"
    ).arg(COLOR_SUCCESS));

    m_cancelBtn = new QPushButton("✗ 取消/重新抓拍");
    m_cancelBtn->setStyleSheet(QString(
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %1, stop:1 #c73e54); color: white;"
    ).arg(COLOR_ACCENT));

    connect(m_confirmBtn, &QPushButton::clicked, this, &EntryDialog::onConfirmClicked);
    connect(m_cancelBtn, &QPushButton::clicked, this, &EntryDialog::onCancelClicked);

    btnLayout->addStretch();
    btnLayout->addWidget(m_confirmBtn);
    btnLayout->addWidget(m_cancelBtn);
    btnLayout->addStretch();

    mainLayout->addLayout(btnLayout);
}

void EntryDialog::setImage(const QPixmap &image)
{
    if (!image.isNull()) {
        QPixmap scaled = image.scaled(m_imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        m_imageLabel->setPixmap(scaled);
    }
}

void EntryDialog::setRecognitionResult(const QString &plateNumber, const QString &rfidCard, double confidence)
{
    m_plateLabel->setText(plateNumber);
    m_plateEdit->setText(plateNumber);
    m_rfidCard = rfidCard;
    m_rfidLabel->setText(rfidCard.isEmpty() ? "--" : rfidCard);

    int confValue = (int)(confidence * 100);
    m_confidenceBar->setValue(confValue);
    m_confidenceLabel->setText(QString("%1%").arg(confValue));
}

void EntryDialog::setEntryTime(const QDateTime &time)
{
    m_entryTime = time;
    m_imageTimeLabel->setText("入场时间: " + time.toString("yyyy-MM-dd hh:mm:ss"));
}

QString EntryDialog::getPlateNumber() const
{
    return m_plateEdit->text().trimmed();
}

QString EntryDialog::getRfidCard() const
{
    return m_rfidCard;
}

void EntryDialog::startCountdown(int seconds)
{
    if (seconds <= 0) return;

    m_countdownValue = seconds;
    m_countdownTimer = new QTimer(this);
    connect(m_countdownTimer, &QTimer::timeout, this, &EntryDialog::onCountdownTick);
    m_countdownTimer->start(1000);
    updateCountdownDisplay();
}

void EntryDialog::onConfirmClicked()
{
    if (m_countdownTimer) {
        m_countdownTimer->stop();
    }
    emit confirmed();
    accept();
}

void EntryDialog::onCancelClicked()
{
    if (m_countdownTimer) {
        m_countdownTimer->stop();
    }
    emit cancelled();
    reject();
}

void EntryDialog::onCountdownTick()
{
    m_countdownValue--;
    updateCountdownDisplay();

    if (m_countdownValue <= 0) {
        m_countdownTimer->stop();
        emit confirmed();
        accept();
    }
}

void EntryDialog::updateCountdownDisplay()
{
    m_countdownLabel->setText(QString("⏱ 自动确认倒计时: %1 秒").arg(m_countdownValue));
}
