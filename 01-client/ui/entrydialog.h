#ifndef ENTRYDIALOG_H
#define ENTRYDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QProgressBar>
#include <QTimer>
#include <QDateTime>
#include <QPixmap>

/**
 * @brief Manual entry confirm
 *
 * Shows capture + LPR fields
 * Allows plate edit
 */
class EntryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EntryDialog(QWidget *parent = nullptr);
    ~EntryDialog();

    /**
     * @brief setImage
     */
    void setImage(const QPixmap &image);

    /**
     * @brief setRecognitionResult
     * @param plateNumber
     * @param rfidCard
     * @param confidence 0-100
     */
    void setRecognitionResult(const QString &plateNumber, const QString &rfidCard, double confidence);

    /**
     * @brief setEntryTime
     */
    void setEntryTime(const QDateTime &time);

    /**
     * @brief edited plate
     */
    QString getPlateNumber() const;

    /**
     * @brief rfid getter
     */
    QString getRfidCard() const;

    /**
     * @brief countdown
     * @param seconds or 0
     */
    void startCountdown(int seconds = 10);

signals:
    /**
     * @brief confirmed
     */
    void confirmed();

    /**
     * @brief cancelled
     */
    void cancelled();

private slots:
    void onConfirmClicked();
    void onCancelClicked();
    void onCountdownTick();

private:
    void setupUI();
    void updateCountdownDisplay();

    // Image
    QLabel *m_imageLabel;
    QLabel *m_imageTimeLabel;

    // LPR panel
    QLabel *m_plateLabel;
    QLabel *m_rfidLabel;
    QProgressBar *m_confidenceBar;
    QLabel *m_confidenceLabel;

    // Plate edit
    QLineEdit *m_plateEdit;

    // Buttons
    QPushButton *m_confirmBtn;
    QPushButton *m_cancelBtn;

    // Timer
    QLabel *m_countdownLabel;
    QTimer *m_countdownTimer;
    int m_countdownValue;

    // State
    QString m_rfidCard;
    QDateTime m_entryTime;
};

#endif // ENTRYDIALOG_H
