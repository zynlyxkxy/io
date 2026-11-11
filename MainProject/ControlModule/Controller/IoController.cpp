#include "IoController.h"

#include <iostream>

static constexpr double VERIFY_EPSILON = 1e-12;

IoController::IoController(QObject *parent)
    : QObject(parent)
    , m_cfg(ConfigManager::instance())
{
}

IoController::~IoController()
{
}

bool IoController::init(const QString &configFilePath)
{
    std::cout << "[CTL] IoController::init begin" << std::endl;

    if (!m_cfg.load(configFilePath))
    {
        std::cout << "[CTL] ConfigManager::load failed: " << configFilePath.toStdString() << std::endl;
        return false;
    }
    std::cout << "[CTL] Config loaded OK" << std::endl;

    m_verifyTimeoutMs = m_cfg.verifyTimeoutMs();
    m_observerPeriodMs = m_cfg.observerPeriodMs();
    std::cout << "[CTL] ACS ip=" << m_cfg.acsIp().toStdString()
              << " port=" << m_cfg.acsPort()
              << " poll=" << m_cfg.acsPollMs() << std::endl;
    std::cout << "[CTL] PLC ip=" << m_cfg.plcIp().toStdString()
              << " port=" << m_cfg.plcPort()
              << " poll=" << m_cfg.plcPollMs() << std::endl;
    std::cout << "[CTL] acsAiMap=" << m_cfg.acsAiMap.size()
              << " acsDiMap=" << m_cfg.acsDiMap.size()
              << " acsAoMap=" << m_cfg.acsAoMap.size()
              << " acsDoMap=" << m_cfg.acsDoMap.size()
              << " acsBufMap=" << m_cfg.acsBufMap.size() << std::endl;
    std::cout << "[CTL] plcAiMap=" << m_cfg.plcAiMap.size()
              << " plcDiMap=" << m_cfg.plcDiMap.size()
              << " plcAoMap=" << m_cfg.plcAoMap.size()
              << " plcDoMap=" << m_cfg.plcDoMap.size()
              << " plcCmdMap=" << m_cfg.plcCmdMap.size()
              << " plcSettingDoubleMap=" << m_cfg.plcSettingDoubleMap.size()
              << " plcSettingBoolMap=" << m_cfg.plcSettingBoolMap.size() << std::endl;

    // ===== ACS =====
    m_acsModule = new AcsIoModule(m_cfg.acsIp(), m_cfg.acsPort(), this);
    m_acsModule->setPollPeriod(m_cfg.acsPollMs());

    for (auto it = m_cfg.acsAiMap.begin(); it != m_cfg.acsAiMap.end(); ++it)
    {
        m_acsModule->registerAi(it.value().name);
    }
    for (auto it = m_cfg.acsDiMap.begin(); it != m_cfg.acsDiMap.end(); ++it)
    {
        m_acsModule->registerDi(it.value().name);
    }
    for (auto it = m_cfg.acsAoMap.begin(); it != m_cfg.acsAoMap.end(); ++it)
    {
        m_acsModule->registerAi(it.value().observer);
    }
    for (auto it = m_cfg.acsDoMap.begin(); it != m_cfg.acsDoMap.end(); ++it)
    {
        m_acsModule->registerDi(it.value().observer);
    }
    for (auto it = m_cfg.acsBufMap.begin(); it != m_cfg.acsBufMap.end(); ++it)
    {
        m_acsModule->registerBuffer(it.value().bufferId);
    }

    // ===== PLC =====
    m_plcModule = new PlcIoModule(m_cfg.plcIp(), m_cfg.plcPort(), this);
    m_plcModule->setPollPeriod(m_cfg.plcPollMs());

    for (const auto &path : m_cfg.plcMachinePartPaths())
    {
        m_plcModule->addPath(path);
    }

    for (auto it = m_cfg.plcAiMap.begin(); it != m_cfg.plcAiMap.end(); ++it)
    {
        m_plcModule->registerObserver(it.value().fullPath, it.value().name);
    }
    for (auto it = m_cfg.plcDiMap.begin(); it != m_cfg.plcDiMap.end(); ++it)
    {
        m_plcModule->registerObserver(it.value().fullPath, it.value().name);
    }
    for (auto it = m_cfg.plcAoMap.begin(); it != m_cfg.plcAoMap.end(); ++it)
    {
        m_plcModule->registerObserver(it.value().fullPath, it.value().observer);
    }
    for (auto it = m_cfg.plcDoMap.begin(); it != m_cfg.plcDoMap.end(); ++it)
    {
        m_plcModule->registerObserver(it.value().fullPath, it.value().observer);
    }

    // ===== 心跳 1s =====
    m_heartbeatTimer = new QTimer(this);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &IoController::onHeartbeat);
    m_heartbeatTimer->start(1000);

    // ===== 校验定时器 100ms =====
    m_verifyTimer = new QTimer(this);
    connect(m_verifyTimer, &QTimer::timeout, this, &IoController::onVerifyCheck);
    m_verifyTimer->start(100);

    // ===== Observer 周期推送（聚合 ACS + PLC 缓存，一条消息） =====
    m_observerTimer = new QTimer(this);
    connect(m_observerTimer, &QTimer::timeout, this, &IoController::onObserverTimer);
    m_observerTimer->start(m_observerPeriodMs > 0 ? m_observerPeriodMs : 500);

    std::cout << "[CTL] IoController::init OK" << std::endl;
    return true;
}

// ===================================================================
// AI / DI 读
// ===================================================================

int IoController::getAi(int varId, bool forceRefresh, double &out)
{
    if (isPlc(varId))
    {
        auto it = m_cfg.plcAiMap.find(varId);
        if (it == m_cfg.plcAiMap.end())
        {
            return IO_ERR_NOT_FOUND;
        }
        int ret = m_plcModule->getAi(it.value().fullPath, it.value().name, out, forceRefresh);
        return ret == 0 ? IO_SUCCESS : IO_ERR_READ_FAILED;
    }
    else
    {
        auto it = m_cfg.acsAiMap.find(varId);
        if (it == m_cfg.acsAiMap.end())
        {
            return IO_ERR_NOT_FOUND;
        }
        int ret = m_acsModule->getAi(it.value().name, out, forceRefresh);
        return ret == 0 ? IO_SUCCESS : IO_ERR_READ_FAILED;
    }
}

int IoController::getDi(int varId, bool forceRefresh, bool &out)
{
    if (isPlc(varId))
    {
        auto it = m_cfg.plcDiMap.find(varId);
        if (it == m_cfg.plcDiMap.end())
        {
            return IO_ERR_NOT_FOUND;
        }
        int ret = m_plcModule->getDi(it.value().fullPath, it.value().name, out, forceRefresh);
        return ret == 0 ? IO_SUCCESS : IO_ERR_READ_FAILED;
    }
    else
    {
        auto it = m_cfg.acsDiMap.find(varId);
        if (it == m_cfg.acsDiMap.end())
        {
            return IO_ERR_NOT_FOUND;
        }
        int ret = m_acsModule->getDi(it.value().name, out, forceRefresh);
        return ret == 0 ? IO_SUCCESS : IO_ERR_READ_FAILED;
    }
}

// ===================================================================
// AO / DO 读（读绑定的 observer）
// ===================================================================

int IoController::getAo(int varId, bool forceRefresh, double &out)
{
    if (isPlc(varId))
    {
        auto it = m_cfg.plcAoMap.find(varId);
        if (it == m_cfg.plcAoMap.end())
        {
            return IO_ERR_NOT_FOUND;
        }
        int ret = m_plcModule->getAi(it.value().fullPath, it.value().observer, out, forceRefresh);
        return ret == 0 ? IO_SUCCESS : IO_ERR_READ_FAILED;
    }
    else
    {
        auto it = m_cfg.acsAoMap.find(varId);
        if (it == m_cfg.acsAoMap.end())
        {
            return IO_ERR_NOT_FOUND;
        }
        int ret = m_acsModule->getAi(it.value().observer, out, forceRefresh);
        return ret == 0 ? IO_SUCCESS : IO_ERR_READ_FAILED;
    }
}

int IoController::getDo(int varId, bool forceRefresh, bool &out)
{
    if (isPlc(varId))
    {
        auto it = m_cfg.plcDoMap.find(varId);
        if (it == m_cfg.plcDoMap.end())
        {
            return IO_ERR_NOT_FOUND;
        }
        int ret = m_plcModule->getDi(it.value().fullPath, it.value().observer, out, forceRefresh);
        return ret == 0 ? IO_SUCCESS : IO_ERR_READ_FAILED;
    }
    else
    {
        auto it = m_cfg.acsDoMap.find(varId);
        if (it == m_cfg.acsDoMap.end())
        {
            return IO_ERR_NOT_FOUND;
        }
        int ret = m_acsModule->getDi(it.value().observer, out, forceRefresh);
        return ret == 0 ? IO_SUCCESS : IO_ERR_READ_FAILED;
    }
}

// ===================================================================
// AO / DO 写
// ===================================================================

int IoController::setAo(int varId, double value, bool verify, double expectedValue)
{
    if (isBusy(varId))
    {
        return IO_ERR_BUSY;
    }

    if (isPlc(varId))
    {
        auto it = m_cfg.plcAoMap.find(varId);
        if (it == m_cfg.plcAoMap.end())
        {
            return IO_ERR_NOT_FOUND;
        }

        int ret = m_plcModule->setAo(it.value().fullPath, it.value().name, value);
        if (ret != 0)
        {
            if (verify)
            {
                emit sig_SetCompleted(varId, false, 2);
            }
            return IO_ERR_WRITE_FAILED;
        }
        if (verify)
        {
            PendingVerify pv;
            pv.varId = varId;
            pv.segment = segment(varId);
            pv.startTime = QDateTime::currentMSecsSinceEpoch();
            pv.expectedDouble = expectedValue;
            m_verifyQueue.append(pv);
        }
        return IO_SUCCESS;
    }
    else
    {
        auto it = m_cfg.acsAoMap.find(varId);
        if (it == m_cfg.acsAoMap.end())
        {
            return IO_ERR_NOT_FOUND;
        }

        int ret = m_acsModule->setAo(it.value().name, value);
        if (ret != 0)
        {
            if (verify)
            {
                emit sig_SetCompleted(varId, false, 2);
            }
            return IO_ERR_WRITE_FAILED;
        }
        if (verify)
        {
            PendingVerify pv;
            pv.varId = varId;
            pv.segment = segment(varId);
            pv.startTime = QDateTime::currentMSecsSinceEpoch();
            pv.expectedDouble = expectedValue;
            m_verifyQueue.append(pv);
        }
        return IO_SUCCESS;
    }
}

int IoController::setDo(int varId, bool value, bool verify, bool expectedValue)
{
    if (isBusy(varId))
    {
        return IO_ERR_BUSY;
    }

    if (isPlc(varId))
    {
        auto it = m_cfg.plcDoMap.find(varId);
        if (it == m_cfg.plcDoMap.end())
        {
            return IO_ERR_NOT_FOUND;
        }

        int ret = m_plcModule->setDo(it.value().fullPath, it.value().name, value);
        if (ret != 0)
        {
            if (verify)
            {
                emit sig_SetCompleted(varId, false, 2);
            }
            return IO_ERR_WRITE_FAILED;
        }
        if (verify)
        {
            PendingVerify pv;
            pv.varId = varId;
            pv.segment = segment(varId);
            pv.startTime = QDateTime::currentMSecsSinceEpoch();
            pv.expectedBool = expectedValue;
            m_verifyQueue.append(pv);
        }
        return IO_SUCCESS;
    }
    else
    {
        auto it = m_cfg.acsDoMap.find(varId);
        if (it == m_cfg.acsDoMap.end())
        {
            return IO_ERR_NOT_FOUND;
        }

        int ret = m_acsModule->setDo(it.value().name, value);
        if (ret != 0)
        {
            if (verify)
            {
                emit sig_SetCompleted(varId, false, 2);
            }
            return IO_ERR_WRITE_FAILED;
        }
        if (verify)
        {
            PendingVerify pv;
            pv.varId = varId;
            pv.segment = segment(varId);
            pv.startTime = QDateTime::currentMSecsSinceEpoch();
            pv.expectedBool = expectedValue;
            m_verifyQueue.append(pv);
        }
        return IO_SUCCESS;
    }
}

// ===================================================================
// Buffer
// ===================================================================

int IoController::startBuffer(int varId)
{
    if (segment(varId) != 9)
    {
        return IO_ERR_NOT_FOUND;
    }
    auto it = m_cfg.acsBufMap.find(varId);
    if (it == m_cfg.acsBufMap.end())
    {
        return IO_ERR_NOT_FOUND;
    }
    int ret = m_acsModule->startBuffer(it.value().bufferId);
    return ret == 0 ? IO_SUCCESS : IO_ERR_WRITE_FAILED;
}

int IoController::stopBuffer(int varId)
{
    if (segment(varId) != 9)
    {
        return IO_ERR_NOT_FOUND;
    }
    auto it = m_cfg.acsBufMap.find(varId);
    if (it == m_cfg.acsBufMap.end())
    {
        return IO_ERR_NOT_FOUND;
    }
    int ret = m_acsModule->stopBuffer(it.value().bufferId);
    return ret == 0 ? IO_SUCCESS : IO_ERR_WRITE_FAILED;
}

int IoController::getBufferRunning(int varId, bool forceRefresh, bool &out)
{
    if (segment(varId) != 9)
    {
        return IO_ERR_NOT_FOUND;
    }
    auto it = m_cfg.acsBufMap.find(varId);
    if (it == m_cfg.acsBufMap.end())
    {
        return IO_ERR_NOT_FOUND;
    }
    int ret = m_acsModule->getBufferRunning(it.value().bufferId, out, forceRefresh);
    return ret == 0 ? IO_SUCCESS : IO_ERR_READ_FAILED;
}

// ===================================================================
// Cmd
// ===================================================================

int IoController::execCmd(int varId, const QList<double> &values)
{
    if (segment(varId) != 10)
    {
        return IO_ERR_NOT_FOUND;
    }
    auto it = m_cfg.plcCmdMap.find(varId);
    if (it == m_cfg.plcCmdMap.end())
    {
        return IO_ERR_NOT_FOUND;
    }
    int ret = m_plcModule->executeCmd(it.value().fullPath, it.value().name, values);
    return ret == 0 ? IO_SUCCESS : IO_ERR_WRITE_FAILED;
}

// ===================================================================
// Setting
// ===================================================================

int IoController::setSettingDouble(int varId, double value)
{
    if (segment(varId) != 8)
    {
        return IO_ERR_NOT_FOUND;
    }
    auto it = m_cfg.plcSettingDoubleMap.find(varId);
    if (it == m_cfg.plcSettingDoubleMap.end())
    {
        return IO_ERR_NOT_FOUND;
    }
    int ret = m_plcModule->setSettingDouble(it.value().fullPath, it.value().name, value);
    return ret == 0 ? IO_SUCCESS : IO_ERR_WRITE_FAILED;
}

int IoController::setSettingBool(int varId, bool value)
{
    if (segment(varId) != 8)
    {
        return IO_ERR_NOT_FOUND;
    }
    auto it = m_cfg.plcSettingBoolMap.find(varId);
    if (it == m_cfg.plcSettingBoolMap.end())
    {
        return IO_ERR_NOT_FOUND;
    }
    int ret = m_plcModule->setSettingBool(it.value().fullPath, it.value().name, value);
    return ret == 0 ? IO_SUCCESS : IO_ERR_WRITE_FAILED;
}

// ===================================================================
// IoServerInfo
// ===================================================================

IoServerInfo IoController::getServerInfo() const
{
    IoServerInfo info;

    for (auto it = m_cfg.acsAiMap.begin(); it != m_cfg.acsAiMap.end(); ++it)
    {
        info.io.AI.push_back({it.key(), it.value().name.toStdString()});
    }
    for (auto it = m_cfg.plcAiMap.begin(); it != m_cfg.plcAiMap.end(); ++it)
    {
        QString d = it.value().label.isEmpty() ? it.value().name : it.value().label;
        info.io.AI.push_back({it.key(), d.toStdString()});
    }
    for (auto it = m_cfg.acsDiMap.begin(); it != m_cfg.acsDiMap.end(); ++it)
    {
        info.io.DI.push_back({it.key(), it.value().name.toStdString()});
    }
    for (auto it = m_cfg.plcDiMap.begin(); it != m_cfg.plcDiMap.end(); ++it)
    {
        QString d = it.value().label.isEmpty() ? it.value().name : it.value().label;
        info.io.DI.push_back({it.key(), d.toStdString()});
    }
    for (auto it = m_cfg.acsAoMap.begin(); it != m_cfg.acsAoMap.end(); ++it)
    {
        info.io.AO.push_back({it.key(), it.value().name.toStdString(), it.value().observer.toStdString()});
    }
    for (auto it = m_cfg.plcAoMap.begin(); it != m_cfg.plcAoMap.end(); ++it)
    {
        QString d = it.value().label.isEmpty() ? it.value().name : it.value().label;
        info.io.AO.push_back({it.key(), d.toStdString(), it.value().observer.toStdString()});
    }
    for (auto it = m_cfg.acsDoMap.begin(); it != m_cfg.acsDoMap.end(); ++it)
    {
        info.io.DO.push_back({it.key(), it.value().name.toStdString(), it.value().observer.toStdString()});
    }
    for (auto it = m_cfg.plcDoMap.begin(); it != m_cfg.plcDoMap.end(); ++it)
    {
        QString d = it.value().label.isEmpty() ? it.value().name : it.value().label;
        info.io.DO.push_back({it.key(), d.toStdString(), it.value().observer.toStdString()});
    }
    for (auto it = m_cfg.plcSettingDoubleMap.begin(); it != m_cfg.plcSettingDoubleMap.end(); ++it)
    {
        QString d = it.value().label.isEmpty() ? it.value().name : it.value().label;
        info.io.SettingDouble.push_back({it.key(), d.toStdString()});
    }
    for (auto it = m_cfg.plcSettingBoolMap.begin(); it != m_cfg.plcSettingBoolMap.end(); ++it)
    {
        QString d = it.value().label.isEmpty() ? it.value().name : it.value().label;
        info.io.SettingBool.push_back({it.key(), d.toStdString()});
    }
    for (auto it = m_cfg.plcCmdMap.begin(); it != m_cfg.plcCmdMap.end(); ++it)
    {
        QString d = it.value().label.isEmpty() ? it.value().name : it.value().label;
        IoCmdVarInfo cmd;
        cmd.id = it.key();
        cmd.name = d.toStdString();
        for (const QString &a : it.value().args)
        {
            IoArgInfo arg;
            arg.name = a.toStdString();
            cmd.args.push_back(arg);
        }
        info.io.Cmd.push_back(cmd);
    }
    for (auto it = m_cfg.acsBufMap.begin(); it != m_cfg.acsBufMap.end(); ++it)
    {
        info.io.Buffer.push_back({it.key(), "buffer " + std::to_string(it.value().bufferId)});
    }

    return info;
}

// ===================================================================
// Custom
// ===================================================================

int IoController::execCustomCommand(const IoCustomCommand &cmd)
{
    std::cout << "[CTL] IoCustomCommand received, json count=" << cmd.json.size() << std::endl;
    for (size_t i = 0; i < cmd.json.size(); ++i)
    {
        std::cout << "[CTL]   [" << i << "] " << cmd.json[i] << std::endl;
    }
    return IO_SUCCESS;
}

void IoController::publishCustomNotify(const IoCustomNotifyData &data)
{
    emit sig_CustomNotifyReady(data);
}

// ===================================================================
// Observer 推送（聚合 ACS + PLC 缓存，周期触发）
// ===================================================================

void IoController::onObserverTimer()
{
    IoObserverData data;

    for (auto it = m_cfg.acsAiMap.begin(); it != m_cfg.acsAiMap.end(); ++it)
    {
        double v;
        if (m_acsModule->getAi(it.value().name, v, false) == 0)
        {
            data.acs.AI.push_back({it.key(), v});
        }
    }
    for (auto it = m_cfg.acsDiMap.begin(); it != m_cfg.acsDiMap.end(); ++it)
    {
        bool v;
        if (m_acsModule->getDi(it.value().name, v, false) == 0)
        {
            data.acs.DI.push_back({it.key(), v});
        }
    }
    for (auto it = m_cfg.acsAoMap.begin(); it != m_cfg.acsAoMap.end(); ++it)
    {
        double v;
        if (m_acsModule->getAi(it.value().observer, v, false) == 0)
        {
            data.acs.AO.push_back({it.key(), v});
        }
    }
    for (auto it = m_cfg.acsDoMap.begin(); it != m_cfg.acsDoMap.end(); ++it)
    {
        bool v;
        if (m_acsModule->getDi(it.value().observer, v, false) == 0)
        {
            data.acs.DO.push_back({it.key(), v});
        }
    }
    for (auto it = m_cfg.acsBufMap.begin(); it != m_cfg.acsBufMap.end(); ++it)
    {
        bool v;
        if (m_acsModule->getBufferRunning(it.value().bufferId, v, false) == 0)
        {
            data.acs.Buf.push_back({it.key(), v});
        }
    }

    for (auto it = m_cfg.plcAiMap.begin(); it != m_cfg.plcAiMap.end(); ++it)
    {
        double v;
        if (m_plcModule->getAi(it.value().fullPath, it.value().name, v, false) == 0)
        {
            data.plc.AI.push_back({it.key(), v});
        }
    }
    for (auto it = m_cfg.plcDiMap.begin(); it != m_cfg.plcDiMap.end(); ++it)
    {
        bool v;
        if (m_plcModule->getDi(it.value().fullPath, it.value().name, v, false) == 0)
        {
            data.plc.DI.push_back({it.key(), v});
        }
    }
    for (auto it = m_cfg.plcAoMap.begin(); it != m_cfg.plcAoMap.end(); ++it)
    {
        double v;
        if (m_plcModule->getAi(it.value().fullPath, it.value().observer, v, false) == 0)
        {
            data.plc.AO.push_back({it.key(), v});
        }
    }
    for (auto it = m_cfg.plcDoMap.begin(); it != m_cfg.plcDoMap.end(); ++it)
    {
        bool v;
        if (m_plcModule->getDi(it.value().fullPath, it.value().observer, v, false) == 0)
        {
            data.plc.DO.push_back({it.key(), v});
        }
    }

    emit sig_ObserverDataReady(data);
}

// ===================================================================
// 校验
// ===================================================================

void IoController::onVerifyCheck()
{
    if (m_verifyQueue.isEmpty())
    {
        return;
    }

    qint64 now = QDateTime::currentMSecsSinceEpoch();
    bool plcRefreshed = false;

    for (int i = m_verifyQueue.size() - 1; i >= 0; --i)
    {
        auto &pending = m_verifyQueue[i];

        if (now - pending.startTime > m_verifyTimeoutMs)
        {
            emit sig_SetCompleted(pending.varId, false, 1);
            m_verifyQueue.removeAt(i);
            continue;
        }

        // 加速刷新
        if ((pending.segment == 4 || pending.segment == 6) && !plcRefreshed)
        {
            m_plcModule->refreshAllObs();
            plcRefreshed = true;
        }

        bool ok = false;
        if (pending.segment == 4 || pending.segment == 5)
        {
            // AO
            double cur;
            bool forceRefresh = (pending.segment == 5); // ACS 直读硬件
            if (getAo(pending.varId, forceRefresh, cur) == IO_SUCCESS)
            {
                ok = qAbs(cur - pending.expectedDouble) <= VERIFY_EPSILON;
            }
        }
        else
        {
            // DO
            bool cur;
            bool forceRefresh = (pending.segment == 7); // ACS 直读硬件
            if (getDo(pending.varId, forceRefresh, cur) == IO_SUCCESS)
            {
                ok = (cur == pending.expectedBool);
            }
        }

        if (ok)
        {
            emit sig_SetCompleted(pending.varId, true, 0);
            m_verifyQueue.removeAt(i);
        }
    }
}

// ===================================================================
// 心跳
// ===================================================================

void IoController::onHeartbeat()
{
    emit sig_HeartbeatReady();
}
