#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <QHash>
#include <QString>
#include <QStringList>

struct AcsVarInfo
{
    int id;
    QString name;
};

struct AcsBufInfo
{
    int id;
    int bufferId;
};

struct AcsAoDoInfo
{
    int id;
    QString name;
    QString observer;
};

struct PlcVarInfo
{
    int id;
    QString fullPath;
    QString name;
    QString label;
};

struct PlcCmdInfo
{
    int id;
    QString fullPath;
    QString name;
    QString label;
    QStringList args;               // 整理后的参数名（getServerInfo 下发用）
};

struct PlcAoDoInfo
{
    int id;
    QString fullPath;
    QString name;
    QString label;
    QString observer;
};

class ConfigManager
{
public:
    static ConfigManager &instance();

    bool load(const QString &configFilePath);

    // 原始配置
    QString acsIp() const;
    int acsPort() const;
    int acsPollMs() const;
    QString plcIp() const;
    int plcPort() const;
    int plcPollMs() const;
    int observerPeriodMs() const;
    int verifyTimeoutMs() const;

    // PLC machinePart 路径（初始化 addPath 用）
    QStringList plcMachinePartPaths() const;

    // ACS 变量
    QHash<int, AcsVarInfo> acsAiMap;
    QHash<int, AcsVarInfo> acsDiMap;
    QHash<int, AcsAoDoInfo> acsAoMap;
    QHash<int, AcsAoDoInfo> acsDoMap;
    QHash<int, AcsBufInfo> acsBufMap;

    // PLC 变量
    QHash<int, PlcVarInfo> plcAiMap;
    QHash<int, PlcVarInfo> plcDiMap;
    QHash<int, PlcAoDoInfo> plcAoMap;
    QHash<int, PlcAoDoInfo> plcDoMap;
    QHash<int, PlcCmdInfo> plcCmdMap;
    QHash<int, PlcVarInfo> plcSettingDoubleMap;
    QHash<int, PlcVarInfo> plcSettingBoolMap;

private:
    ConfigManager() = default;
    ConfigManager(const ConfigManager &) = delete;
    ConfigManager &operator=(const ConfigManager &) = delete;

    bool loadAcsConfig(const QString &configPath);
    bool loadPlcConfig(const QString &configPath);

    /// 把配置里写的相对子路径（如 "acs/io_config.json"）解析成绝对路径：
    /// 基准是本模块配置文件自身所在目录（<配置根>/io_config/），与进程 cwd 无关
    QString resolve(const QString &path) const;

    QString m_configDir;   ///< 本模块配置文件所在目录（load 时确定）
    QString m_acsIp;
    int m_acsPort = 0;
    int m_acsPollMs = 500;
    QString m_plcIp;
    int m_plcPort = 0;
    int m_plcPollMs = 500;
    int m_observerPeriodMs = 500;
    int m_verifyTimeoutMs = 20000;
    QStringList m_plcPaths;
};

#endif // CONFIG_MANAGER_H
