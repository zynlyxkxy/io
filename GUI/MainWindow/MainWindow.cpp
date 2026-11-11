#include "MainWindow.h"
#include "IoGroup/IoGroup.h"
#include "io_client.h"
#include "io_define.h"
#include "istar_platform_env.h"

#include <QVBoxLayout>
#include <QDateTime>
#include <QString>
#include <iostream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // mw_config.yaml 在配置根上（PLATFORM_PATH），未设置时用 cwd
    const char *envRoot = ISTARGetEnvPlatform();
    m_yamlConfigPath = QString::fromLocal8Bit((envRoot && *envRoot) ? envRoot : ".")
                       + QStringLiteral("/mw_config.yaml");

    QWidget *central = new QWidget(this);
    central->setStyleSheet("background-color:#F3F4FA;");
    setCentralWidget(central);

    QVBoxLayout *layout = new QVBoxLayout(central);
    layout->setContentsMargins(4, 4, 4, 4);

    m_ioGroup = new IoGroup(central);
    layout->addWidget(m_ioGroup);

    setWindowTitle("IO");
    resize(1280, 860);

    // 线程安全：RPC 回调线程 → emit signal → GUI 主线程 slot
    connect(this, &MainWindow::sig_ObserverReceived,
            m_ioGroup, &IoGroup::sig_ObserverReceived,
            Qt::QueuedConnection);

    // 第一次心跳到达后触发 discover
    connect(this, &MainWindow::sig_HeartbeatArrived,
            this, &MainWindow::discoverIo, Qt::QueuedConnection);

    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, &MainWindow::connectServer);

    m_heartbeatWatchdog = new QTimer(this);
    connect(m_heartbeatWatchdog, &QTimer::timeout, this, &MainWindow::onHeartbeatWatchdog);

    connectServer();
}

MainWindow::~MainWindow()
{
    if (m_rpc)
    {
        m_rpc->ExitRun();
    }
}

void MainWindow::connectServer()
{
    if (m_rpc)
    {
        m_rpc->ExitRun();
        m_rpc.reset();
    }

    m_rpc = std::make_shared<IO_Client_Inherit>();

    ISTARResultStatus ret = m_rpc->IOClientInit(m_yamlConfigPath.toStdString());
    if (ISTAR_RESULT_SUCCESS != ret)
    {
        std::cout << "[GUI] IOClientInit failed, retrying in 2s" << std::endl;
        m_rpc.reset();
        m_reconnectTimer->start(2000);
        return;
    }

    // 只注册心跳回调，第一次心跳到了再做 discover（平台 StatusUpload 无载荷，到达时间即心跳时间）
    m_rpc->RegisterCallback(I_O_STATUS_UPLOAD_ID,
        std::function<ISTARResultStatus()>(
            [this]() -> ISTARResultStatus
    {
        m_lastHeartbeatTime = QDateTime::currentMSecsSinceEpoch();

        if (!m_ioDiscovered)
        {
            m_ioDiscovered = true;
            emit sig_HeartbeatArrived();
        }
        return ISTAR_RESULT_SUCCESS;
    }));

    m_connected = true;
    m_lastHeartbeatTime = 0;
    m_reconnectTimer->stop();
    m_heartbeatWatchdog->start(1000);

    std::cout << "[GUI] RPC client initialized, waiting for heartbeat" << std::endl;
}

void MainWindow::discoverIo()
{
    IoServerInfo serverInfo;
    ISTARResultStatus ret = m_rpc->IOGetServerInfo(serverInfo);
    if (ISTAR_RESULT_SUCCESS != ret)
    {
        std::cout << "[GUI] IOGetServerInfo failed" << std::endl;
        m_ioDiscovered = false;
        return;
    }

    std::cout << "[GUI] Discovered IO:"
              << " AI=" << serverInfo.io.AI.size()
              << " DI=" << serverInfo.io.DI.size()
              << " AO=" << serverInfo.io.AO.size()
              << " DO=" << serverInfo.io.DO.size()
              << " SettingDouble=" << serverInfo.io.SettingDouble.size()
              << " SettingBool=" << serverInfo.io.SettingBool.size()
              << " Buffer=" << serverInfo.io.Buffer.size() << std::endl;

    m_ioGroup->buildTabs(serverInfo);
    registerCallbacks();
    m_ioGroup->setRpcClient(m_rpc.get());
    m_ioDiscovered = true;

    std::cout << "[GUI] IO ready" << std::endl;
}

void MainWindow::registerCallbacks()
{
    // Observer（一次推 ACS + PLC 全部缓存）
    m_rpc->RegisterCallback(I_O_OBSERVER_DATA_ID,
        std::function<ISTARResultStatus(IoObserverData &)>(
            [this](IoObserverData &data) -> ISTARResultStatus
    {
        emit sig_ObserverReceived(data);
        return ISTAR_RESULT_SUCCESS;
    }));
}

void MainWindow::onHeartbeatWatchdog()
{
    if (!m_connected || m_lastHeartbeatTime == 0)
    {
        return;
    }

    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - m_lastHeartbeatTime > 5000)
    {
        std::cout << "[GUI] Heartbeat timeout, reconnecting" << std::endl;
        m_connected = false;
        m_ioDiscovered = false;
        m_heartbeatWatchdog->stop();

        if (m_rpc)
        {
            m_rpc->ExitRun();
            m_rpc.reset();
        }

        m_ioGroup->setRpcClient(nullptr);
        m_reconnectTimer->start(2000);
    }
}
