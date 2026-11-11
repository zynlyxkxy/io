#include "ConfigManager.h"
#include "data/ConfigData.h"

#include "istar_serializer.hpp"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

using namespace ISTAR;

namespace
{
template <typename T>
bool parseJsonFile(const QString &path, T &out)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        qWarning() << "ConfigManager: cannot open" << path;
        return false;
    }
    const std::string content = file.readAll().toStdString();
    file.close();
    try
    {
        out = Serializer::from_rawJson<T>(content);
    }
    catch (const std::exception &e)
    {
        qWarning() << "ConfigManager: parse JSON failed" << path << e.what();
        return false;
    }
    return true;
}
} // namespace

ConfigManager &ConfigManager::instance()
{
    static ConfigManager inst;
    return inst;
}

bool ConfigManager::load(const QString &configFilePath)
{
    S_MainConfig mainCfg;
    if (!parseJsonFile(configFilePath, mainCfg))
    {
        return false;
    }

    // 子配置（acs/、plc/）以本文件所在目录为基准解析，见 resolve()
    m_configDir = QFileInfo(configFilePath).absolutePath();

    m_acsIp = QString::fromStdString(mainCfg.acs.ip);
    m_acsPort = mainCfg.acs.port;
    m_acsPollMs = mainCfg.acs.pollPeriodMs;
    m_plcIp = QString::fromStdString(mainCfg.plc.ip);
    m_plcPort = mainCfg.plc.port;
    m_plcPollMs = mainCfg.plc.pollPeriodMs;
    m_observerPeriodMs = mainCfg.observerPeriodMs;
    m_verifyTimeoutMs = mainCfg.verifyTimeoutMs;

    if (!loadAcsConfig(resolve(QString::fromStdString(mainCfg.acs.configPath))))
    {
        return false;
    }

    if (!loadPlcConfig(resolve(QString::fromStdString(mainCfg.plc.configPath))))
    {
        return false;
    }

    return true;
}

QString ConfigManager::resolve(const QString &path) const
{
    // 只有绝对路径要放过；config.json 里的子路径一律相对本模块配置文件所在目录
    return QDir::isAbsolutePath(path) ? path : QDir(m_configDir).filePath(path);
}

bool ConfigManager::loadAcsConfig(const QString &configPath)
{
    S_AcsIoConfig cfg;
    if (!parseJsonFile(configPath, cfg))
    {
        return false;
    }

    for (const auto &e : cfg.AI)
    {
        acsAiMap.insert(e.id, {e.id, QString::fromStdString(e.name)});
    }
    for (const auto &e : cfg.DI)
    {
        acsDiMap.insert(e.id, {e.id, QString::fromStdString(e.name)});
    }
    for (const auto &e : cfg.AO)
    {
        const QString observer = e.observer.empty() ? QString::fromStdString(e.name)
                                                    : QString::fromStdString(e.observer);
        acsAoMap.insert(e.id, {e.id, QString::fromStdString(e.name), observer});
    }
    for (const auto &e : cfg.DO)
    {
        const QString observer = e.observer.empty() ? QString::fromStdString(e.name)
                                                    : QString::fromStdString(e.observer);
        acsDoMap.insert(e.id, {e.id, QString::fromStdString(e.name), observer});
    }
    for (const auto &e : cfg.Buffer)
    {
        acsBufMap.insert(e.id, {e.id, e.bufferId});
    }

    return true;
}

bool ConfigManager::loadPlcConfig(const QString &configPath)
{
    S_PlcIoConfig cfg;
    if (!parseJsonFile(configPath, cfg))
    {
        return false;
    }

    for (const auto &mp : cfg.machineParts)
    {
        const QString fullPath = QString::fromStdString(mp.fullPath);
        m_plcPaths.append(fullPath);

        for (const auto &e : mp.AI)
        {
            plcAiMap.insert(e.id, {e.id, fullPath, QString::fromStdString(e.name),
                                   QString::fromStdString(e.label)});
        }
        for (const auto &e : mp.DI)
        {
            plcDiMap.insert(e.id, {e.id, fullPath, QString::fromStdString(e.name),
                                   QString::fromStdString(e.label)});
        }
        for (const auto &e : mp.AO)
        {
            plcAoMap.insert(e.id, {e.id, fullPath, QString::fromStdString(e.name),
                                   QString::fromStdString(e.label),
                                   QString::fromStdString(e.observer)});
        }
        for (const auto &e : mp.DO)
        {
            plcDoMap.insert(e.id, {e.id, fullPath, QString::fromStdString(e.name),
                                   QString::fromStdString(e.label),
                                   QString::fromStdString(e.observer)});
        }
        for (const auto &e : mp.Cmd)
        {
            PlcCmdInfo info;
            info.id = e.id;
            info.fullPath = fullPath;
            info.name = QString::fromStdString(e.name);
            info.label = QString::fromStdString(e.label);
            for (const std::string &a : e.args)
            {
                info.args.append(QString::fromStdString(a));
            }
            plcCmdMap.insert(e.id, info);
        }
        for (const auto &e : mp.SettingDouble)
        {
            plcSettingDoubleMap.insert(e.id, {e.id, fullPath, QString::fromStdString(e.name),
                                              QString::fromStdString(e.label)});
        }
        for (const auto &e : mp.SettingBool)
        {
            plcSettingBoolMap.insert(e.id, {e.id, fullPath, QString::fromStdString(e.name),
                                            QString::fromStdString(e.label)});
        }
    }

    return true;
}

QString ConfigManager::acsIp() const
{
    return m_acsIp;
}
int ConfigManager::acsPort() const
{
    return m_acsPort;
}
int ConfigManager::acsPollMs() const
{
    return m_acsPollMs;
}
QString ConfigManager::plcIp() const
{
    return m_plcIp;
}
int ConfigManager::plcPort() const
{
    return m_plcPort;
}
int ConfigManager::plcPollMs() const
{
    return m_plcPollMs;
}
int ConfigManager::observerPeriodMs() const
{
    return m_observerPeriodMs;
}
int ConfigManager::verifyTimeoutMs() const
{
    return m_verifyTimeoutMs;
}
QStringList ConfigManager::plcMachinePartPaths() const
{
    return m_plcPaths;
}
