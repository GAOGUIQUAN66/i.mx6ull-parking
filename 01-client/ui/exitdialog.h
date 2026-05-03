#ifndef EXITDIALOG_H
#define EXITDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QDateTime>
#include <QPixmap>
#include <QTimer>

/**
 * @brief Exit payment UI
 *
 * Shows tariff + RFID prompt
 * Countdown + swipe
 */
class ExitDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExitDialog(QWidget *parent = nullptr);
    ~ExitDialog();

    /**
     * @brief Snapshot pixmap
     */
    void setImage(const QPixmap &image);

    /**
     * @brief Parking meta
     * @param plateNumber
     * @param entryTime
     * @param exitTime
     * @param duration minutes
     */
    void setParkingInfo(const QString &plateNumber, const QDateTime &entryTime,
                        const QDateTime &exitTime, int duration);

    void setFeeInfo(double totalFee);
    void setPaymentInfo(const QString &statusText, const QString &cardId = QString(),
                        double balance = -1.0);
    void startCountdown(int seconds = 10);
    void stopCountdown();

    /**
     * @brief Plate getter
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

    // Image
    QLabel *m_imageLabel;
    QLabel *m_imageTimeLabel;

    // Parking labels
    QLabel *m_plateLabel;
    QLabel *m_entryTimeLabel;
    QLabel *m_exitTimeLabel;
    QLabel *m_durationLabel;

    // Fee labels
    QLabel *m_totalFeeLabel;
    QLabel *m_cardLabel;
    QLabel *m_balanceLabel;
    QLabel *m_statusLabel;
    QLabel *m_countdownLabel;

    // Buttons
    QPushButton *m_closeBtn;

    // Timer
    QTimer *m_countdownTimer;
    int m_countdownValue;

    // State
    QString m_plateNumber;
};

#endif // EXITDIALOG_H
