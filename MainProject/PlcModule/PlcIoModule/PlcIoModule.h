#ifndef PLCIOMODULE_H
#define PLCIOMODULE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include <QHash>
#include <QSet>
#include <QList>
#include <QVariant>
#include <QMutex>

class PlcClient;
class QTimer;

struct PathInfo
{
    QString mpFullPath;
    QJsonObject machinePart;
    QHash<QString, QJsonObject> aoTemplates;    // 1 个 FLOAT 参数
    QHash<QString, QJsonObject> doTemplates;    // 1 个 BOOLEAN 参数
    QHash<QString, QJsonObject> cmdTemplates;   // 0 参数
    QHash<QString, QJsonObject> multiArgTemplates;  // 2+ 参数命令
    QSet<QString> aiNames;
    QSet<QString> diNames;
    QSet<QString> settingDoubleNames;           // FLOAT_TYPED_VALUE / INT_TYPED_VALUE
    QSet<QString> settingBoolNames;             // BOOLEAN_TYPED_VALUE
};

class PlcIoModule : public QObject
{
    Q_OBJECT
public:
    enum Error {
        Success = 0,
        DriverNotConnected,
        HttpTimeout,
        PlcFault,
        CmdNotFound,
        ObserverNotFound,
        SettingNotFound,
        PathNotFound,
        TypeMismatch,
    };

    explicit PlcIoModule(const QString &ip, int port, QObject *parent = nullptr);
    ~PlcIoModule();

    bool addPath(const QString &mpFullPath);

    // --- AO：1 个 FLOAT 参数的命令 ---
    int setAo(const QString &mpFullPath, const QString &cmdName, double value);

    // --- DO：1 个 BOOLEAN 参数的命令 ---
    int setDo(const QString &mpFullPath, const QString &cmdName, bool value);

    // --- AI：FLOAT_OBSERVER ---
    int getAi(const QString &mpFullPath, const QString &obsName, double &out,
              bool forceRefresh = false);

    // --- DI：BOOLEAN_OBSERVER ---
    int getDi(const QString &mpFullPath, const QString &obsName, bool &out,
              bool forceRefresh = false);

    // --- Cmd：0 或多个参数命令（values 按参数顺序，0 参数命令传空） ---
    int executeCmd(const QString &mpFullPath, const QString &cmdName,
                   const QList<double> &values = {});

    // --- SettingDouble ---
    int setSettingDouble(const QString &mpFullPath, const QString &settingName,
                         double value);

    // --- SettingBool ---
    int setSettingBool(const QString &mpFullPath, const QString &settingName,
                       bool value);

    // --- 观察量管理 ---
    void registerObserver(const QString &mpFullPath, const QString &obsName);
    void registerObservers(const QString &mpFullPath, const QStringList &obsNames);
    void setPollPeriod(int ms);

    // --- 强制刷新所有已注册 observer（校验用） ---
    void refreshAllObs();

    // --- 查询 ---
    QStringList paths() const;
    QStringList aoNames(const QString &mpFullPath) const;
    QStringList doNames(const QString &mpFullPath) const;
    QStringList cmdNames(const QString &mpFullPath) const;
    QStringList aiNames(const QString &mpFullPath) const;
    QStringList diNames(const QString &mpFullPath) const;
    QStringList settingDoubleNames(const QString &mpFullPath) const;
    QStringList settingBoolNames(const QString &mpFullPath) const;

signals:
    void sig_ObsRefreshed();

private:
    void onPollTimer();

    PlcClient *m_client;
    QHash<QString, PathInfo> m_paths;
    QHash<QString, QVariant> m_obsCache;   // "mpFullPath/obsName" → 值
    QStringList m_pollKeys;                 // "mpFullPath/obsName" 列表
    QTimer *m_pollTimer;
};

#endif // PLCIOMODULE_H
