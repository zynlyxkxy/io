#include "PlcIoModule.h"
#include "PlcClient/PlcClient.h"

#include <QTimer>
#include <QDebug>
#include <iostream>
#include <cstdlib>

PlcIoModule::PlcIoModule(const QString &ip, int port, QObject *parent)
    : QObject(parent)
    , m_client(new PlcClient(ip, port, this))
    , m_pollTimer(new QTimer(this))
{
    connect(m_pollTimer, &QTimer::timeout, this, &PlcIoModule::onPollTimer);
}

PlcIoModule::~PlcIoModule()
{
    m_pollTimer->stop();
}

// ========== addPath ==========

bool PlcIoModule::addPath(const QString &mpFullPath)
{
    if (m_paths.contains(mpFullPath))
    {
        std::cout << "[PLC IO] addPath: already registered: "
                  << mpFullPath.toStdString() << std::endl;
        return true;
    }

    MachinePartProperties props;
    if (!m_client->fetchMachinePartProperties(mpFullPath, props))
    {
        qWarning() << "PlcIoModule: addPath failed for" << mpFullPath;
        return false;
    }

    PathInfo info;
    info.mpFullPath = mpFullPath;
    info.machinePart = props.machinePart;

    // 归类命令模板
    for (const QJsonObject &cmd : props.commands)
    {
        QString name = cmd["name"].toString();
        QJsonArray args = cmd["arguments"].toArray();

        if (args.isEmpty())
        {
            info.cmdTemplates.insert(name, cmd);
        }
        else if (args.size() == 1)
        {
            QString type = args[0].toObject()["type"].toString();
            if (type.contains("FLOAT"))
            {
                info.aoTemplates.insert(name, cmd);
            }
            else if (type.contains("BOOLEAN"))
            {
                info.doTemplates.insert(name, cmd);
            }
        }
        else
        {
            info.multiArgTemplates.insert(name, cmd);
        }
    }

    // 归类 observer
    for (const QJsonObject &obs : props.observers)
    {
        QString name = obs["name"].toString();
        QString type = obs["type"].toString();
        if (type.contains("FLOAT"))
        {
            info.aiNames.insert(name);
        }
        else if (type.contains("BOOLEAN"))
        {
            info.diNames.insert(name);
        }
    }

    // 归类 setting
    for (const QJsonObject &stg : props.settings)
    {
        QString type = stg["type"].toString();
        QString name = stg["name"].toString();
        if (type == "BOOLEAN_TYPED_VALUE")
            info.settingBoolNames.insert(name);
        else
            info.settingDoubleNames.insert(name);  // FLOAT / INT
    }

    m_paths.insert(mpFullPath, info);

    std::cout << "[PLC IO] addPath OK: " << mpFullPath.toStdString()
              << " ao=" << info.aoTemplates.size()
              << " do=" << info.doTemplates.size()
              << " cmd=" << info.cmdTemplates.size()
              << " ai=" << info.aiNames.size()
              << " di=" << info.diNames.size()
              << " settingDbl=" << info.settingDoubleNames.size()
              << " settingBool=" << info.settingBoolNames.size() << std::endl;

    return true;
}

// ========== 辅助：查找路径 ==========

static PlcIoModule::Error findPath(const QHash<QString, PathInfo> &paths,
                                    const QString &mpFullPath,
                                    PathInfo *&out)
{
    auto it = paths.find(mpFullPath);
    if (it == paths.end())
    {
        return PlcIoModule::PathNotFound;
    }
    out = const_cast<PathInfo *>(&it.value());
    return PlcIoModule::Success;
}

// ========== setAo ==========

int PlcIoModule::setAo(const QString &mpFullPath, const QString &cmdName, double value)
{
#ifdef IO_LOCAL_DEBUG
    Q_UNUSED(mpFullPath);
    Q_UNUSED(cmdName);
    Q_UNUSED(value);
    std::cout << "[PLC IO] setAo LOCAL_DEBUG skip: "
              << mpFullPath.toStdString() << " / " << cmdName.toStdString()
              << " = " << value << std::endl;
    return Success;
#endif
    PathInfo *info;
    Error ret = findPath(m_paths, mpFullPath, info);
    if (ret != Success)
    {
        return ret;
    }

    auto it = info->aoTemplates.find(cmdName);
    if (it == info->aoTemplates.end())
    {
        return CmdNotFound;
    }

    return m_client->executeCommand(info->machinePart, it.value(), {value});
}

// ========== setDo ==========

int PlcIoModule::setDo(const QString &mpFullPath, const QString &cmdName, bool value)
{
#ifdef IO_LOCAL_DEBUG
    Q_UNUSED(mpFullPath);
    Q_UNUSED(cmdName);
    Q_UNUSED(value);
    std::cout << "[PLC IO] setDo LOCAL_DEBUG skip: "
              << mpFullPath.toStdString() << " / " << cmdName.toStdString()
              << " = " << value << std::endl;
    return Success;
#endif
    PathInfo *info;
    Error ret = findPath(m_paths, mpFullPath, info);
    if (ret != Success)
    {
        return ret;
    }

    auto it = info->doTemplates.find(cmdName);
    if (it == info->doTemplates.end())
    {
        return CmdNotFound;
    }

    return m_client->executeCommand(info->machinePart, it.value(), {value ? 1.0 : 0.0});
}

// ========== executeCmd ==========

int PlcIoModule::executeCmd(const QString &mpFullPath, const QString &cmdName,
                            const QList<double> &values)
{
#ifdef IO_LOCAL_DEBUG
    Q_UNUSED(mpFullPath);
    Q_UNUSED(cmdName);
    Q_UNUSED(values);
    std::cout << "[PLC IO] executeCmd LOCAL_DEBUG skip: "
              << mpFullPath.toStdString() << " / " << cmdName.toStdString()
              << " values=" << values.size() << std::endl;
    return Success;
#endif
    PathInfo *info;
    Error ret = findPath(m_paths, mpFullPath, info);
    if (ret != Success)
    {
        return ret;
    }

    auto it = info->cmdTemplates.find(cmdName);
    if (it == info->cmdTemplates.end())
    {
        it = info->multiArgTemplates.find(cmdName);
        if (it == info->multiArgTemplates.end())
        {
            return CmdNotFound;
        }
    }

    return m_client->executeCommand(info->machinePart, it.value(), values);
}

// ========== getAi ==========

int PlcIoModule::getAi(const QString &mpFullPath, const QString &obsName,
                        double &out, bool forceRefresh)
{
    PathInfo *info;
    Error ret = findPath(m_paths, mpFullPath, info);
    if (ret != Success)
    {
        return ret;
    }

    QString cacheKey = mpFullPath + "/" + obsName;

#ifdef IO_LOCAL_DEBUG
    out = (double)rand() / RAND_MAX * 10.0 - 5.0;
    m_obsCache[cacheKey] = out;
    return Success;
#endif

    if (forceRefresh || !m_pollTimer->isActive())
    {
        QStringList names{obsName};
        QVariantList results = m_client->getObserverValues(mpFullPath, names);
        if (results.isEmpty())
        {
            return DriverNotConnected;
        }

        out = results[0].toDouble();
        m_obsCache[cacheKey] = out;
        return Success;
    }

    auto it = m_obsCache.find(cacheKey);
    if (it == m_obsCache.end())
    {
        return ObserverNotFound;
    }

    out = it.value().toDouble();
    return Success;
}

// ========== getDi ==========

int PlcIoModule::getDi(const QString &mpFullPath, const QString &obsName,
                        bool &out, bool forceRefresh)
{
    PathInfo *info;
    Error ret = findPath(m_paths, mpFullPath, info);
    if (ret != Success)
    {
        return ret;
    }

    QString cacheKey = mpFullPath + "/" + obsName;

#ifdef IO_LOCAL_DEBUG
    int h = qHash(cacheKey);
    out = (h % 2 == 0);
    m_obsCache[cacheKey] = out;
    return Success;
#endif

    if (forceRefresh || !m_pollTimer->isActive())
    {
        QStringList names{obsName};
        QVariantList results = m_client->getObserverValues(mpFullPath, names);
        if (results.isEmpty())
        {
            return DriverNotConnected;
        }

        out = results[0].toBool();
        m_obsCache[cacheKey] = out;
        return Success;
    }

    auto it = m_obsCache.find(cacheKey);
    if (it == m_obsCache.end())
    {
        return ObserverNotFound;
    }

    out = it.value().toBool();
    return Success;
}

// ========== setSettingDouble ==========

int PlcIoModule::setSettingDouble(const QString &mpFullPath, const QString &settingName,
                                   double value)
{
#ifdef IO_LOCAL_DEBUG
    std::cout << "[PLC IO] setSettingDouble LOCAL_DEBUG skip: "
              << mpFullPath.toStdString() << " / " << settingName.toStdString()
              << " = " << value << std::endl;
    return Success;
#endif
    PathInfo *info;
    Error ret = findPath(m_paths, mpFullPath, info);
    if (ret != Success)
        return ret;

    if (!info->settingDoubleNames.contains(settingName))
        return SettingNotFound;

    QJsonObject typedValue;
    typedValue["type"] = QString("FLOAT_TYPED_VALUE");
    typedValue["floatValue"] = value;

    int setRet = m_client->setSettingValue(mpFullPath, settingName, typedValue);
    if (setRet != 0)
        return setRet;

    return m_client->saveSettingValue(mpFullPath, settingName);
}

// ========== setSettingBool ==========

int PlcIoModule::setSettingBool(const QString &mpFullPath, const QString &settingName,
                                 bool value)
{
#ifdef IO_LOCAL_DEBUG
    std::cout << "[PLC IO] setSettingBool LOCAL_DEBUG skip: "
              << mpFullPath.toStdString() << " / " << settingName.toStdString()
              << " = " << value << std::endl;
    return Success;
#endif
    PathInfo *info;
    Error ret = findPath(m_paths, mpFullPath, info);
    if (ret != Success)
        return ret;

    if (!info->settingBoolNames.contains(settingName))
        return SettingNotFound;

    QJsonObject typedValue;
    typedValue["type"] = QString("BOOLEAN_TYPED_VALUE");
    typedValue["booleanValue"] = value;

    int setRet = m_client->setSettingValue(mpFullPath, settingName, typedValue);
    if (setRet != 0)
        return setRet;

    return m_client->saveSettingValue(mpFullPath, settingName);
}

// ========== 观察量管理 ==========

void PlcIoModule::registerObserver(const QString &mpFullPath, const QString &obsName)
{
    QString key = mpFullPath + "/" + obsName;
    if (!m_pollKeys.contains(key))
    {
        m_pollKeys.append(key);
        m_obsCache[key] = QVariant();  // 占位
    }
}

void PlcIoModule::registerObservers(const QString &mpFullPath,
                                     const QStringList &obsNames)
{
    for (const QString &name : obsNames)
    {
        registerObserver(mpFullPath, name);
    }
}

void PlcIoModule::setPollPeriod(int ms)
{
    if (ms <= 0)
    {
        m_pollTimer->stop();
        return;
    }
    m_pollTimer->start(ms);
}

// ========== 轮询 ==========

void PlcIoModule::onPollTimer()
{
    if (m_pollKeys.isEmpty())
    {
        return;
    }

    // 按路径分组
    QHash<QString, QStringList> grouped;
    for (const QString &key : m_pollKeys)
    {
        int idx = key.lastIndexOf('/');
        QString path = key.left(idx);
        QString obsName = key.mid(idx + 1);
        grouped[path].append(obsName);
    }

#ifdef IO_LOCAL_DEBUG
    for (auto it = grouped.begin(); it != grouped.end(); ++it)
    {
        const QString &path = it.key();
        const QStringList &names = it.value();

        if (!m_paths.contains(path))
            continue;

        PathInfo *info = const_cast<PathInfo *>(&m_paths[path]);
        for (const QString &name : names)
        {
            QString key = path + "/" + name;
            if (info->aiNames.contains(name))
                m_obsCache[key] = (double)rand() / RAND_MAX * 10.0 - 5.0;
            else
                m_obsCache[key] = (qHash(key) % 2 == 0) ? 1.0 : 0.0;
        }
    }
    emit sig_ObsRefreshed();
    return;
#endif

    for (auto it = grouped.begin(); it != grouped.end(); ++it)
    {
        const QString &path = it.key();
        const QStringList &names = it.value();

        if (!m_paths.contains(path))
        {
            continue;
        }

        QVariantList results = m_client->getObserverValues(path, names);
        if (results.isEmpty())
        {
            continue;
        }

        for (int i = 0; i < names.size() && i < results.size(); i++)
        {
            QString key = path + "/" + names[i];
            m_obsCache[key] = results[i];
        }
    }

    emit sig_ObsRefreshed();
}

void PlcIoModule::refreshAllObs()
{
    onPollTimer();
}

// ========== 查询 ==========

QStringList PlcIoModule::paths() const
{
    return m_paths.keys();
}

QStringList PlcIoModule::aoNames(const QString &mpFullPath) const
{
    auto it = m_paths.find(mpFullPath);
    if (it == m_paths.end())
    {
        return {};
    }
    QStringList names = it.value().aoTemplates.keys();
    names.sort();
    return names;
}

QStringList PlcIoModule::doNames(const QString &mpFullPath) const
{
    auto it = m_paths.find(mpFullPath);
    if (it == m_paths.end())
    {
        return {};
    }
    QStringList names = it.value().doTemplates.keys();
    names.sort();
    return names;
}

QStringList PlcIoModule::cmdNames(const QString &mpFullPath) const
{
    auto it = m_paths.find(mpFullPath);
    if (it == m_paths.end())
    {
        return {};
    }
    QStringList names = it.value().cmdTemplates.keys();
    names.sort();
    return names;
}

QStringList PlcIoModule::aiNames(const QString &mpFullPath) const
{
    auto it = m_paths.find(mpFullPath);
    if (it == m_paths.end())
    {
        return {};
    }
    QStringList names = it.value().aiNames.values();
    names.sort();
    return names;
}

QStringList PlcIoModule::diNames(const QString &mpFullPath) const
{
    auto it = m_paths.find(mpFullPath);
    if (it == m_paths.end())
    {
        return {};
    }
    QStringList names = it.value().diNames.values();
    names.sort();
    return names;
}

QStringList PlcIoModule::settingDoubleNames(const QString &mpFullPath) const
{
    auto it = m_paths.find(mpFullPath);
    if (it == m_paths.end())
        return {};
    QStringList names = it.value().settingDoubleNames.values();
    names.sort();
    return names;
}

QStringList PlcIoModule::settingBoolNames(const QString &mpFullPath) const
{
    auto it = m_paths.find(mpFullPath);
    if (it == m_paths.end())
        return {};
    QStringList names = it.value().settingBoolNames.values();
    names.sort();
    return names;
}
