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
 * @brief Settings window
 *
 * Network host/port
 * (rates elsewhere)
 * TCP test
 */
class SettingsWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit SettingsWindow(QWidget *parent = nullptr);
    ~SettingsWindow();

    /**
     * @brief Network config
     */
    struct NetworkConfig
    {
        QString serverIp;
        int port;
        bool isConnected;

        NetworkConfig() : serverIp("192.168.137.121"), port(8888), isConnected(false) {}
    };

    /**
     * @brief Get network config
     */
    NetworkConfig getNetworkConfig() const;

    /**
     * @brief Apply network config
     */
    void setNetworkConfig(const NetworkConfig &config);

    /**
     * @brief Push live connection status
     */
    void setActualConnectionStatus(bool connected);

signals:
    /**
     * @brief Back to main
     */
    void backToMain();

    /**
     * @brief Config saved
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

    // Network widgets
    QLineEdit *m_serverIpEdit;
    QSpinBox *m_portSpin;

    // Status
    QLabel *m_statusDot;
    QLabel *m_statusText;
    QPushButton *m_testBtn;

    // Buttons
    QPushButton *m_saveBtn;
    QPushButton *m_defaultBtn;

    // Test socket
    QTcpSocket *m_testSocket;

    // State
    NetworkConfig m_config;
};

#endif // SETTINGSWINDOW_H
