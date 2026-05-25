#ifndef EXITDIALOG_H
#define EXITDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QDateTime>
#include <QPixmap>
#include <QTimer>

/**
 * @brief 出场结算/核验对话框
 *
 * 支持入出场抓拍对比、车牌核验异常时人工放行
 */
class ExitDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExitDialog(QWidget *parent = nullptr);
    ~ExitDialog();

    void setCompareImages(const QPixmap &entryImage, const QPixmap &exitImage);
    void setImage(const QPixmap &image);

    void setParkingInfo(const QString &plateNumber, const QDateTime &entryTime,
                        const QDateTime &exitTime, int duration);

    void setPlateVerifyInfo(const QString &boundPlate, const QString &recognizedPlate, bool matched);
    void setVerifyMode(bool manualReview);

    void setFeeInfo(double totalFee, int durationMinutes = -1, double unitPricePerMinute = 0.1);
    void setPaymentInfo(const QString &statusText, const QString &cardId = QString(),
                        double balance = -1.0);
    void setSettlementMode(bool settlement);
    void startCountdown(int seconds = 10);
    void stopCountdown();

    QString getPlateNumber() const;

signals:
    void cancelled();
    void timedOut();
    void manualPassRequested();
    void retryRecognizeRequested();

private slots:
    void onCancelClicked();
    void onCountdownTick();
    void onManualPassClicked();
    void onRetryRecognizeClicked();

private:
    void setupUI();
    void updateCountdownLabel();
    void setLabelImage(QLabel *label, const QPixmap &image, const QString &placeholder);

    QLabel *m_entryImageLabel;
    QLabel *m_exitImageLabel;
    QLabel *m_entryCaptionLabel;
    QLabel *m_exitCaptionLabel;
    QLabel *m_verifyLabel;

    QLabel *m_plateLabel;
    QLabel *m_entryTimeLabel;
    QLabel *m_exitTimeLabel;
    QLabel *m_durationLabel;

    QLabel *m_totalFeeLabel;
    QLabel *m_feeRuleLabel;
    QLabel *m_cardLabel;
    QLabel *m_balanceLabel;
    QLabel *m_statusLabel;
    QLabel *m_countdownLabel;

    QPushButton *m_retryBtn;
    QPushButton *m_manualPassBtn;
    QPushButton *m_closeBtn;

    QTimer *m_countdownTimer;
    int m_countdownValue;
    bool m_settlementMode;
    bool m_manualReview;

    QLabel *m_titleLabel;
    QString m_plateNumber;
};

#endif // EXITDIALOG_H
