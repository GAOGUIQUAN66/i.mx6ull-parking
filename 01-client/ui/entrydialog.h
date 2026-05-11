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
 * @brief 入场确认对话框
 *
 * 显示抓拍图片、识别结果、RFID卡号、置信度
 * 支持车牌修正输入，确认入场/取消操作
 */
class EntryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EntryDialog(QWidget *parent = nullptr);
    ~EntryDialog();

    /**
     * @brief 设置抓拍图片
     */
    void setImage(const QPixmap &image);

    /**
     * @brief 设置识别结果
     * @param plateNumber 车牌号
     * @param rfidCard RFID卡号
     * @param confidence 置信度(0-100)
     */
    void setRecognitionResult(const QString &plateNumber, const QString &rfidCard, double confidence);

    /**
     * @brief 设置入场时间
     */
    void setEntryTime(const QDateTime &time);

    /**
     * @brief 获取确认的车牌号
     */
    QString getPlateNumber() const;

    /**
     * @brief 获取RFID卡号
     */
    QString getRfidCard() const;

    /**
     * @brief 启用自动确认倒计时
     * @param seconds 倒计时秒数，0表示禁用
     */
    void startCountdown(int seconds = 10);

signals:
    /**
     * @brief 确认入场信号
     */
    void confirmed();

    /**
     * @brief 取消/重新抓拍信号
     */
    void cancelled();

private slots:
    void onConfirmClicked();
    void onCancelClicked();
    void onCountdownTick();

private:
    void setupUI();
    void updateCountdownDisplay();

    // 图片区域
    QLabel *m_imageLabel;
    QLabel *m_imageTimeLabel;

    // 识别结果区域
    QLabel *m_plateLabel;
    QLabel *m_rfidLabel;
    QProgressBar *m_confidenceBar;
    QLabel *m_confidenceLabel;

    // 车牌修正输入
    QLineEdit *m_plateEdit;

    // 按钮
    QPushButton *m_confirmBtn;
    QPushButton *m_cancelBtn;

    // 倒计时
    QLabel *m_countdownLabel;
    QTimer *m_countdownTimer;
    int m_countdownValue;

    // 数据
    QString m_rfidCard;
    QDateTime m_entryTime;
};

#endif // ENTRYDIALOG_H
