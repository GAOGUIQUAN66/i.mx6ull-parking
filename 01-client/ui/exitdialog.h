#ifndef EXITDIALOG_H
#define EXITDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QDateTime>
#include <QPixmap>
#include <QTimer>

/**
 * @brief 出场结算对话框
 *
 * 显示停车信息、费用明细
 * 支持等待刷卡、倒计时超时自动关闭
 */
class ExitDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExitDialog(QWidget *parent = nullptr);
    ~ExitDialog();

    /**
     * @brief 设置出场抓拍图片
     */
    void setImage(const QPixmap &image);

    /**
     * @brief 设置停车信息
     * @param plateNumber 车牌号
     * @param entryTime 入场时间
     * @param exitTime 出场时间
     * @param duration 停车时长（分钟）
     */
    void setParkingInfo(const QString &plateNumber, const QDateTime &entryTime,
                        const QDateTime &exitTime, int duration);

    void setFeeInfo(double totalFee);
    void setPaymentInfo(const QString &statusText, const QString &cardId = QString(),
                        double balance = -1.0);
    void startCountdown(int seconds = 10);
    void stopCountdown();

    /**
     * @brief 获取车牌号
     */
    QString getPlateNumber() const;

signals:
    void cancelled();
    void timedOut();

private slots:
    void onCancelClicked();
    void onCountdownTick();

private:
    void setupUI();
    void updateCountdownLabel();

    // 图片区域
    QLabel *m_imageLabel;
    QLabel *m_imageTimeLabel;

    // 停车信息
    QLabel *m_plateLabel;
    QLabel *m_entryTimeLabel;
    QLabel *m_exitTimeLabel;
    QLabel *m_durationLabel;

    // 费用信息
    QLabel *m_totalFeeLabel;
    QLabel *m_cardLabel;
    QLabel *m_balanceLabel;
    QLabel *m_statusLabel;
    QLabel *m_countdownLabel;

    // 按钮
    QPushButton *m_closeBtn;

    // 倒计时
    QTimer *m_countdownTimer;
    int m_countdownValue;

    // 数据
    QString m_plateNumber;
};

#endif // EXITDIALOG_H
