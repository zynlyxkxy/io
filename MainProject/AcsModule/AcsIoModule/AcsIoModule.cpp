#include "AcsIoModule.h"
#include "AcsClient/AcsClient.h"

#include <QTimer>

// ── 构造 / 析构 ──────────────────────────────────────────────

AcsIoModule::AcsIoModule(const QString &ip, int port, QObject *parent)
    : QObject(parent)
    , m_client(new AcsClient(ip, port, this))
    , m_pollTimer(new QTimer(this))
{
    m_client->Connect();
    connect(m_pollTimer, &QTimer::timeout, this, &AcsIoModule::onPollTimer);
}

AcsIoModule::~AcsIoModule()
{
    m_pollTimer->stop();
    m_client->Disconnect();
}

// ── AO ───────────────────────────────────────────────────────

int AcsIoModule::setAo(const QString &varName, double value)
{
    if (!m_client->IsConnected())
        return DriverNotConnected;
    m_client->WriteReal(varName.toLatin1(), value);
    return Success;
}

// ── DO ───────────────────────────────────────────────────────

int AcsIoModule::setDo(const QString &varName, bool value)
{
    if (!m_client->IsConnected())
        return DriverNotConnected;
    m_client->WriteInteger(varName.toLatin1(), value ? 1 : 0);
    return Success;
}

// ── AI ───────────────────────────────────────────────────────

int AcsIoModule::getAi(const QString &varName, double &out, bool forceRefresh)
{
    if (!m_client->IsConnected())
        return DriverNotConnected;

    const QString key = "ai:" + varName;

    if (!forceRefresh && m_cache.contains(key)) {
        out = m_cache[key].toDouble();
        return Success;
    }

    double val = 0;
    if (!m_client->ReadReal(varName.toLatin1(), &val))
        return DriverNotConnected;

    m_cache[key] = val;
    out = val;
    return Success;
}

// ── DI ───────────────────────────────────────────────────────

int AcsIoModule::getDi(const QString &varName, bool &out, bool forceRefresh)
{
    if (!m_client->IsConnected())
        return DriverNotConnected;

    const QString key = "di:" + varName;

    if (!forceRefresh && m_cache.contains(key)) {
        out = m_cache[key].toBool();
        return Success;
    }

    int val = 0;
    if (!m_client->ReadInteger(varName.toLatin1(), &val))
        return DriverNotConnected;

    const bool bVal = (val != 0);
    m_cache[key] = bVal;
    out = bVal;
    return Success;
}

// ── Buffer ───────────────────────────────────────────────────

int AcsIoModule::startBuffer(int bufferId)
{
    if (!m_client->IsConnected())
        return DriverNotConnected;
    m_client->StartBuffer(bufferId);
    m_cache["buffer:" + QString::number(bufferId)] = true;
    return Success;
}

int AcsIoModule::stopBuffer(int bufferId)
{
    if (!m_client->IsConnected())
        return DriverNotConnected;
    m_client->StopBuffer(bufferId);
    m_cache["buffer:" + QString::number(bufferId)] = false;
    return Success;
}

int AcsIoModule::getBufferRunning(int bufferId, bool &out, bool forceRefresh)
{
    if (!m_client->IsConnected())
        return DriverNotConnected;

    const QString key = "buffer:" + QString::number(bufferId);

    if (!forceRefresh && m_cache.contains(key)) {
        out = m_cache[key].toBool();
        return Success;
    }

    const bool running = m_client->IsBufferRunning(bufferId);
    m_cache[key] = running;
    out = running;
    return Success;
}

// ── 注册 ─────────────────────────────────────────────────────

void AcsIoModule::registerAi(const QString &varName)
{
    if (!m_registeredAi.contains(varName))
        m_registeredAi.append(varName);
}

void AcsIoModule::registerDi(const QString &varName)
{
    if (!m_registeredDi.contains(varName))
        m_registeredDi.append(varName);
}

void AcsIoModule::registerBuffer(int bufferId)
{
    if (!m_registeredBuffers.contains(bufferId))
        m_registeredBuffers.append(bufferId);
}

// ── 轮询配置 ─────────────────────────────────────────────────

void AcsIoModule::setPollPeriod(int ms)
{
    if (ms <= 0)
    {
        m_pollTimer->stop();
        return;
    }
    m_pollTimer->start(ms);
}

// ── 轮询 ─────────────────────────────────────────────────────

void AcsIoModule::onPollTimer()
{
    if (!m_client->IsConnected())
        return;

    for (const QString &var : m_registeredAi) {
        double val = 0;
        if (m_client->ReadReal(var.toLatin1(), &val))
            m_cache["ai:" + var] = val;
    }

    for (const QString &var : m_registeredDi) {
        int val = 0;
        if (m_client->ReadInteger(var.toLatin1(), &val))
            m_cache["di:" + var] = (val != 0);
    }

    for (int id : m_registeredBuffers) {
        m_cache["buffer:" + QString::number(id)] = m_client->IsBufferRunning(id);
    }

    emit sig_ObsRefreshed();
}
