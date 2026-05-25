#include "exitdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>

#define COLOR_BG_DARK "#1a1a2e"
#define COLOR_BG_PANEL "#16213e"
#define COLOR_ACCENT "#e94560"
#define COLOR_SUCCESS "#4ecca3"
#define COLOR_WARNING "#ffd93d"
#define COLOR_TEXT_GRAY "#a0a0a0"

static const QSize kThumbSize(200, 150);

ExitDialog::ExitDialog(QWidget *parent)
    : QDialog(parent)
    , m_entryImageLabel(nullptr)
    , m_exitImageLabel(nullptr)
    , m_entryCaptionLabel(nullptr)
    , m_exitCaptionLabel(nullptr)
    , m_verifyLabel(nullptr)
    , m_plateLabel(nullptr)
    , m_entryTimeLabel(nullptr)
    , m_exitTimeLabel(nullptr)
    , m_durationLabel(nullptr)
    , m_totalFeeLabel(nullptr)
    , m_feeRuleLabel(nullptr)
    , m_cardLabel(nullptr)
    , m_balanceLabel(nullptr)
    , m_statusLabel(nullptr)
    , m_countdownLabel(nullptr)
    , m_retryBtn(nullptr)
    , m_manualPassBtn(nullptr)
    , m_closeBtn(nullptr)
    , m_countdownTimer(new QTimer(this))
    , m_countdownValue(0)
    , m_settlementMode(false)
    , m_manualReview(false)
    , m_titleLabel(nullptr)
{
    setWindowTitle(QString::fromUtf8("车辆出场核验"));
    setFixedSize(900, 560);
    setStyleSheet(QString(
                      "QDialog { background: %1; }"
                      "QLabel { color: white; }"
                      "QPushButton { "
                      "   border: none; "
                      "   border-radius: 10px; "
                      "   padding: 10px 24px; "
                      "   font-size: 15px; "
                      "   font-weight: bold; "
                      "}"
                      "QPushButton:hover { opacity: 0.9; }")
                      .arg(COLOR_BG_DARK));

    m_countdownTimer->setInterval(1000);
    connect(m_countdownTimer, &QTimer::timeout, this, &ExitDialog::onCountdownTick);

    setupUI();
}

ExitDialog::~ExitDialog()
{
}

void ExitDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(20, 15, 20, 15);

    m_titleLabel = new QLabel(QString::fromUtf8("车辆出场核验"));
    m_titleLabel->setStyleSheet(QString(
                                  "font-size: 22px; font-weight: bold; color: white; "
                                  "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %1, stop:1 #3db892); "
                                  "padding: 12px; border-radius: 10px;")
                                  .arg(COLOR_SUCCESS));
    m_titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_titleLabel);

    QHBoxLayout *contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(16);

    QVBoxLayout *imageLayout = new QVBoxLayout();
    imageLayout->setSpacing(12);

    const QString captureLabelStyle = QString(
        "font-size: 14px; color: %1; font-weight: bold;").arg(COLOR_TEXT_GRAY);
    const QString captureImageStyle = QString(
        "background: %1; border: 2px solid #333; border-radius: 8px;").arg(COLOR_BG_PANEL);

    auto addCaptureRow = [&](const QString &title, QLabel *&titleLabel, QLabel *&imageLabel) -> QHBoxLayout * {
        QHBoxLayout *row = new QHBoxLayout();
        row->setSpacing(14);
        titleLabel = new QLabel(title);
        titleLabel->setStyleSheet(captureLabelStyle);
        titleLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        titleLabel->setMinimumWidth(96);
        imageLabel = new QLabel();
        imageLabel->setFixedSize(kThumbSize);
        imageLabel->setStyleSheet(captureImageStyle);
        imageLabel->setAlignment(Qt::AlignCenter);
        row->addWidget(titleLabel);
        row->addStretch();
        row->addWidget(imageLabel, 0, Qt::AlignRight | Qt::AlignVCenter);
        return row;
    };

    imageLayout->addLayout(addCaptureRow(QString::fromUtf8("入场抓拍："), m_entryCaptionLabel, m_entryImageLabel));
    m_entryImageLabel->setText(QString::fromUtf8("无入场图"));

    imageLayout->addLayout(addCaptureRow(QString::fromUtf8("出场抓拍："), m_exitCaptionLabel, m_exitImageLabel));
    m_exitImageLabel->setText(QString::fromUtf8("无出场图"));

    m_verifyLabel = new QLabel("车牌核验: --");
    m_verifyLabel->setWordWrap(true);
    m_verifyLabel->setStyleSheet(QString(
                                     "font-size: 13px; font-weight: bold; color: %1; "
                                     "background: rgba(233, 69, 96, 0.15); border-radius: 8px; padding: 8px;")
                                     .arg(COLOR_ACCENT));
    imageLayout->addWidget(m_verifyLabel);
    contentLayout->addLayout(imageLayout);

    QVBoxLayout *infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(10);

    QGroupBox *parkingGroup = new QGroupBox("车辆信息");
    parkingGroup->setStyleSheet(
        "QGroupBox { background: rgba(255,255,255,0.05); border-radius: 10px; padding: 12px; border: none; }"
        "QGroupBox::title { color: white; font-size: 16px; font-weight: bold; }");

    QVBoxLayout *parkingLayout = new QVBoxLayout(parkingGroup);
    parkingLayout->setSpacing(8);

    auto addRow = [](const QString &title, QLabel *&valueLabel, const QString &valueStyle) -> QHBoxLayout * {
        QHBoxLayout *row = new QHBoxLayout();
        QLabel *t = new QLabel(title);
        t->setStyleSheet(QString("font-size: 14px; color: %1;").arg(COLOR_TEXT_GRAY));
        t->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        valueLabel = new QLabel("--");
        valueLabel->setStyleSheet(valueStyle);
        valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        row->addWidget(t);
        row->addStretch();
        row->addWidget(valueLabel);
        return row;
    };

    parkingLayout->addLayout(addRow(QString::fromUtf8("车牌号"), m_plateLabel,
                                    QString("font-size: 22px; font-weight: bold; color: %1;").arg(COLOR_SUCCESS)));
    parkingLayout->addLayout(addRow(QString::fromUtf8("入场时间"), m_entryTimeLabel, "font-size: 14px; color: white;"));
    parkingLayout->addLayout(addRow(QString::fromUtf8("当前时间"), m_exitTimeLabel, "font-size: 14px; color: white;"));
    parkingLayout->addLayout(addRow("停车时长", m_durationLabel,
                                    QString("font-size: 16px; font-weight: bold; color: %1;").arg(COLOR_WARNING)));
    infoLayout->addWidget(parkingGroup);

    QGroupBox *feeGroup = new QGroupBox("结算信息");
    feeGroup->setStyleSheet(
        "QGroupBox { background: rgba(255,255,255,0.05); border-radius: 10px; padding: 12px; border: none; }"
        "QGroupBox::title { color: white; font-size: 16px; font-weight: bold; }");

    QVBoxLayout *feeLayout = new QVBoxLayout(feeGroup);
    feeLayout->setSpacing(8);

    QHBoxLayout *feeRow = new QHBoxLayout();
    QLabel *feeTitle = new QLabel("应付金额");
    feeTitle->setStyleSheet(QString("font-size: 14px; color: %1;").arg(COLOR_TEXT_GRAY));
    m_totalFeeLabel = new QLabel("¥0.00");
    m_totalFeeLabel->setStyleSheet(QString("font-size: 17px; font-weight: bold; color: %1;").arg(COLOR_ACCENT));
    m_totalFeeLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    feeRow->addWidget(feeTitle);
    feeRow->addStretch();
    feeRow->addWidget(m_totalFeeLabel);

    QHBoxLayout *ruleRow = new QHBoxLayout();
    QLabel *ruleTitle = new QLabel("计费规则");
    ruleTitle->setStyleSheet(QString("font-size: 14px; color: %1;").arg(COLOR_TEXT_GRAY));
    m_feeRuleLabel = new QLabel("按 0.10 元/分钟");
    m_feeRuleLabel->setStyleSheet("font-size: 13px; color: white;");
    m_feeRuleLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    ruleRow->addWidget(ruleTitle);
    ruleRow->addStretch();
    ruleRow->addWidget(m_feeRuleLabel);

    QHBoxLayout *cardRow = new QHBoxLayout();
    QLabel *cardTitle = new QLabel("刷卡卡号");
    cardTitle->setStyleSheet(QString("font-size: 14px; color: %1;").arg(COLOR_TEXT_GRAY));
    m_cardLabel = new QLabel("--");
    m_cardLabel->setStyleSheet("font-size: 13px; color: white;");
    m_cardLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_cardLabel->setWordWrap(true);
    cardRow->addWidget(cardTitle);
    cardRow->addStretch();
    cardRow->addWidget(m_cardLabel);

    QHBoxLayout *balanceRow = new QHBoxLayout();
    QLabel *balanceTitle = new QLabel("卡内余额");
    balanceTitle->setStyleSheet(QString("font-size: 14px; color: %1;").arg(COLOR_TEXT_GRAY));
    m_balanceLabel = new QLabel("--");
    m_balanceLabel->setStyleSheet("font-size: 14px; color: white;");
    m_balanceLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    balanceRow->addWidget(balanceTitle);
    balanceRow->addStretch();
    balanceRow->addWidget(m_balanceLabel);

    m_statusLabel = new QLabel("等待核验");
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setMinimumHeight(44);
    m_statusLabel->setStyleSheet(QString(
                                     "font-size: 13px; font-weight: bold; color: %1; "
                                     "background: rgba(78, 204, 163, 0.12); border-radius: 8px; padding: 8px;")
                                     .arg(COLOR_SUCCESS));

    m_countdownLabel = new QLabel(QString::fromUtf8("剩余自动关闭时间: 10 秒"));
    m_countdownLabel->setAlignment(Qt::AlignCenter);
    m_countdownLabel->setStyleSheet(QString("font-size: 15px; color: %1; margin-top: 8px;").arg(COLOR_WARNING));
    m_countdownLabel->setVisible(false);

    feeLayout->addLayout(feeRow);
    feeLayout->addLayout(ruleRow);
    feeLayout->addLayout(cardRow);
    feeLayout->addLayout(balanceRow);
    feeLayout->addWidget(m_statusLabel);
    feeLayout->addWidget(m_countdownLabel);

    infoLayout->addWidget(feeGroup, 1);
    contentLayout->addLayout(infoLayout, 1);
    mainLayout->addLayout(contentLayout, 1);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    m_retryBtn = new QPushButton("重新识别");
    m_retryBtn->setStyleSheet(
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #555, stop:1 #333); color: white;");
    connect(m_retryBtn, &QPushButton::clicked, this, &ExitDialog::onRetryRecognizeClicked);

    m_manualPassBtn = new QPushButton("人工核查放行");
    m_manualPassBtn->setStyleSheet(
        QString("background: %1; color: white;").arg(COLOR_WARNING));
    connect(m_manualPassBtn, &QPushButton::clicked, this, &ExitDialog::onManualPassClicked);

    m_closeBtn = new QPushButton("关闭");
    m_closeBtn->setStyleSheet(
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #333, stop:1 #222); "
        "color: white; border: 2px solid #555;");
    connect(m_closeBtn, &QPushButton::clicked, this, &ExitDialog::onCancelClicked);

    buttonLayout->addStretch();
    buttonLayout->addWidget(m_retryBtn);
    buttonLayout->addWidget(m_manualPassBtn);
    buttonLayout->addWidget(m_closeBtn);
    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);

    setVerifyMode(false);
}

void ExitDialog::setLabelImage(QLabel *label, const QPixmap &image, const QString &placeholder)
{
    if (!label) {
        return;
    }
    if (!image.isNull()) {
        label->setPixmap(image.scaled(kThumbSize, Qt::KeepAspectRatio, Qt::FastTransformation));
        label->setText(QString());
        return;
    }
    label->setPixmap(QPixmap());
    label->setText(placeholder);
}

void ExitDialog::setCompareImages(const QPixmap &entryImage, const QPixmap &exitImage)
{
    setLabelImage(m_entryImageLabel, entryImage, "无入场图");
    setLabelImage(m_exitImageLabel, exitImage, "无出场图");
}

void ExitDialog::setImage(const QPixmap &image)
{
    setCompareImages(QPixmap(), image);
}

void ExitDialog::setParkingInfo(const QString &plateNumber, const QDateTime &entryTime,
                                const QDateTime &exitTime, int duration)
{
    m_plateNumber = plateNumber;
    m_plateLabel->setText(plateNumber);
    m_entryTimeLabel->setText(entryTime.toString("yyyy-MM-dd hh:mm:ss"));
    m_exitTimeLabel->setText(exitTime.toString("yyyy-MM-dd hh:mm:ss"));

    int hours = duration / 60;
    int mins = duration % 60;
    m_durationLabel->setText(QString("%1小时%2分").arg(hours).arg(mins));
}

void ExitDialog::setPlateVerifyInfo(const QString &boundPlate, const QString &recognizedPlate, bool matched)
{
    if (matched) {
        m_verifyLabel->setText(QString("车牌核验通过：%1").arg(boundPlate));
        m_verifyLabel->setStyleSheet(QString(
                                         "font-size: 13px; font-weight: bold; color: %1; "
                                         "background: rgba(78, 204, 163, 0.2); border-radius: 8px; padding: 8px;")
                                         .arg(COLOR_SUCCESS));
    } else {
        m_verifyLabel->setText(QString("车牌不一致！绑定 %1，识别 %2")
                                   .arg(boundPlate, recognizedPlate.isEmpty() ? "--" : recognizedPlate));
        m_verifyLabel->setStyleSheet(QString(
                                         "font-size: 13px; font-weight: bold; color: %1; "
                                         "background: rgba(233, 69, 96, 0.2); border-radius: 8px; padding: 8px;")
                                         .arg(COLOR_ACCENT));
    }
}

void ExitDialog::setSettlementMode(bool settlement)
{
    m_settlementMode = settlement;
    if (m_titleLabel) {
        m_titleLabel->setText(settlement ? QString::fromUtf8("车辆出场结算")
                                         : QString::fromUtf8("车辆出场核验"));
    }
    setWindowTitle(m_titleLabel ? m_titleLabel->text() : QString::fromUtf8("车辆出场核验"));

    if (m_verifyLabel) {
        m_verifyLabel->setVisible(!settlement);
    }

    setVerifyMode(m_manualReview);
}

void ExitDialog::setVerifyMode(bool manualReview)
{
    m_manualReview = manualReview;
    if (m_settlementMode) {
        m_retryBtn->setVisible(false);
        m_manualPassBtn->setVisible(false);
        m_countdownLabel->setVisible(true);
        return;
    }

    m_retryBtn->setVisible(manualReview);
    m_manualPassBtn->setVisible(manualReview);
    m_countdownLabel->setVisible(false);
    stopCountdown();
}

void ExitDialog::setFeeInfo(double totalFee, int durationMinutes, double unitPricePerMinute)
{
    m_totalFeeLabel->setText(QString("¥%1").arg(totalFee, 0, 'f', 2));
    if (durationMinutes >= 0) {
        m_feeRuleLabel->setText(
            QString("%1 分钟 × ¥%2/分钟 = ¥%3")
                .arg(durationMinutes)
                .arg(unitPricePerMinute, 0, 'f', 2)
                .arg(totalFee, 0, 'f', 2));
    } else {
        m_feeRuleLabel->setText(QString("按 ¥%1/分钟计费").arg(unitPricePerMinute, 0, 'f', 2));
    }
}

void ExitDialog::setPaymentInfo(const QString &statusText, const QString &cardId, double balance)
{
    m_cardLabel->setText(cardId.isEmpty() ? "--" : cardId);

    if (balance >= 0.0) {
        m_balanceLabel->setText(QString("¥%1").arg(balance, 0, 'f', 2));
    } else {
        m_balanceLabel->setText("--");
    }

    m_statusLabel->setText(statusText);
}

void ExitDialog::startCountdown(int seconds)
{
    m_countdownValue = qMax(1, seconds);
    updateCountdownLabel();
    m_countdownTimer->start();
}

void ExitDialog::stopCountdown()
{
    m_countdownTimer->stop();
}

QString ExitDialog::getPlateNumber() const
{
    return m_plateNumber;
}

void ExitDialog::onCancelClicked()
{
    stopCountdown();
    emit cancelled();
    reject();
}

void ExitDialog::onCountdownTick()
{
    if (m_countdownValue > 0) {
        --m_countdownValue;
    }
    updateCountdownLabel();
    if (m_countdownValue > 0) {
        return;
    }

    stopCountdown();
    emit timedOut();
    reject();
}

void ExitDialog::onManualPassClicked()
{
    emit manualPassRequested();
}

void ExitDialog::onRetryRecognizeClicked()
{
    emit retryRecognizeRequested();
}

void ExitDialog::updateCountdownLabel()
{
    if (!m_countdownLabel) {
        return;
    }
    m_countdownLabel->setText(
        QString::fromUtf8("剩余自动关闭时间: %1 秒").arg(qMax(0, m_countdownValue)));
}
