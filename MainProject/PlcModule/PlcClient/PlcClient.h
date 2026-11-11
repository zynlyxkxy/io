#ifndef PLCCLIENT_H
#define PLCCLIENT_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariant>
#include <QVariantList>
#include <QList>

struct MachinePartProperties
{
    QJsonObject machinePart;
    QList<QJsonObject> commands;
    QList<QJsonObject> settings;
    QList<QJsonObject> observers;
};

class PlcClient : public QObject
{
    Q_OBJECT
public:
    explicit PlcClient(const QString &ip, int port, QObject *parent = nullptr);
    ~PlcClient();

    bool fetchMachinePartProperties(const QString &mpFullPath,
                                    MachinePartProperties &out);

    int executeCommand(const QJsonObject &machinePart,
                       const QJsonObject &cmdTemplate,
                       const QList<double> &values);

    QVariantList getObserverValues(const QString &mpFullPath,
                                   const QStringList &obsNames);

    int setSettingValue(const QString &mpFullPath,
                        const QString &settingName,
                        const QJsonObject &typedValue);

    int saveSettingValue(const QString &mpFullPath,
                         const QString &settingName);

private:
    QJsonValue sendRequest(const QString &serviceName, const QJsonArray &inputArgs);

    QString m_baseUrl;
};

#endif // PLCCLIENT_H
