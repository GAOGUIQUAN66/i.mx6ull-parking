#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QGroupBox>
#include <QTcpSocket>

/**
 * @brief 系统设置窗口
 *
 * 配置网络参数（IP、端口）
 * 费率设置
 * 测试连接功能
 */
class SettingsWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit SettingsWindow(QWidget *parent = nullptr);
    ~SettingsWindow();

    /**
     * @brief 结构体：网络配置
     */
    struct NetworkConfig
    {
        QString serverIp;
        int port;
        bool isConnected;

        NetworkConfig() : serverIp("192.168.137.121"), port(8888), isConnected(false) {}
    };

    /**
     * @brief 获取网络配置
     */
    NetworkConfig getNetworkConfig() const;

    /**
     * @brief 设置网络配置
     */
    void setNetworkConfig(const NetworkConfig &config);

    /**
     * @brief 设置当前连接状态（由外部调用）
     */
    void setActualConnectionStatus(bool connected);

signals:
    /**
     * @brief 返回主界面信号
     */
    void backToMain();

    /**
     * @brief 配置保存信号
     */
    void configSaved(const NetworkConfig &config);

private slots:
    void onTestConnection();
    void onTestConnected();
    void onTestError(QAbstractSocket::SocketError socketError);
    void onSaveClicked();
    void onDefaultClicked();
    void onBackClicked();

private:
    void setupUI();
    void updateConnectionStatus(bool connected);

    // 网络设置
    QLineEdit *m_serverIpEdit;
    QSpinBox *m_portSpin;

    // 状态指示
    QLabel *m_statusDot;
    QLabel *m_statusText;
    QPushButton *m_testBtn;

    // 按钮
    QPushButton *m_saveBtn;
    QPushButton *m_defaultBtn;

    // 测试连接用的socket
    QTcpSocket *m_testSocket;

    // 数据
    NetworkConfig m_config;
};

#endif // SETTINGSWINDOW_H
