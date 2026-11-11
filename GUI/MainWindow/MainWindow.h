#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QTimer>
#include <memory>
#include "io_client.h"

class IoGroup;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void sig_ObserverReceived(const IoObserverData &data);
    void sig_HeartbeatArrived();

private:
    void connectServer();
    void discoverIo();
    void registerCallbacks();
    void onHeartbeatWatchdog();

    IoGroup *m_ioGroup = nullptr;
    std::shared_ptr<IO_Client_Inherit> m_rpc;
    QString m_yamlConfigPath;   // <配置根>/mw_config.yaml，构造时算一次
    QTimer *m_reconnectTimer = nullptr;
    QTimer *m_heartbeatWatchdog = nullptr;
    qint64 m_lastHeartbeatTime = 0;
    bool m_connected = false;
    bool m_ioDiscovered = false;
};

#endif // MAINWINDOW_H
