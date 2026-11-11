#include "PlcClient.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QHash>
#include <cstdlib>
#include <QEventLoop>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QUrl>
#include <QDebug>
#include <iostream>

static const int HTTP_TIMEOUT_MS = 2000;
static const QString MCS_SERVICE_PATH =
    "/JsonServices/com.cordis.explorer.connectors.MachinePart.MachinePartConnector/";

PlcClient::PlcClient(const QString &ip, int port, QObject *parent)
    : QObject(parent)
{
    m_baseUrl = QString("http://%1:%2").arg(ip).arg(port) + MCS_SERVICE_PATH;
}

PlcClient::~PlcClient()
{
}

QJsonValue PlcClient::sendRequest(const QString &serviceName, const QJsonArray &inputArgs)
{
#ifdef IO_LOCAL_DEBUG
    QJsonObject fake;
    fake["type"] = "SUCCESS";
    if (serviceName == "getMachinePartProperties")
    {
        QJsonObject props;
        QJsonObject mp;
        mp["name"] = "local_debug";
        props["machinePart"] = mp;
        QJsonArray cmds;
        props["commands"] = QJsonArray();
        props["settings"] = QJsonArray();
        props["observers"] = QJsonArray();
        fake["value"] = props;
    }
    else if (serviceName == "getObserverValuesFromObserverPaths")
    {
        QJsonArray arr;
        QJsonArray inArgs = inputArgs;
        if (inArgs.size() >= 1)
        {
            QJsonObject wrapper = inArgs[0].toObject();
            QJsonArray paths = wrapper["observerPaths"].toArray();
            for (int i = 0; i < paths.size(); i++)
            {
                QJsonObject item;
                item["currentValue"] = 0.0;
                arr.append(item);
            }
        }
        fake["value"] = arr;
    }
    else
    {
        fake["value"] = QJsonArray();
    }
    return fake["value"];
#endif
    QUrl url(m_baseUrl + serviceName);

    QJsonArray payload;
    QJsonObject securityToken;
    securityToken["SecurityToken"] = QString("ADMIN");
    payload.append(securityToken);

    QJsonObject inputArgsWrapper;
    inputArgsWrapper["InputArgs"] = inputArgs;
    payload.append(inputArgsWrapper);

    QJsonDocument doc(payload);
    QByteArray data = doc.toJson(QJsonDocument::Compact);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkAccessManager nam;
    QNetworkReply *reply = nam.post(request, data);

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, reply, &QNetworkReply::abort);
    timer.start(HTTP_TIMEOUT_MS);
    loop.exec();

    if (!reply->isFinished())
    {
        reply->deleteLater();
        qWarning() << "PlcClient: HTTP timeout for service:" << serviceName;
        std::cout << "[PLC REQ] " << serviceName.toStdString() << std::endl;
        std::cout << "[PLC REQ] URL: " << url.toString().toStdString() << std::endl;
        std::cout << "[PLC REQ] Body: " << QString(data).toStdString() << std::endl;
        std::cout << "[PLC RSP] TIMEOUT" << std::endl;
        return QJsonValue();
    }

    QByteArray responseData = reply->readAll();
    reply->deleteLater();

    if (responseData.isEmpty())
    {
        qWarning() << "PlcClient: empty response for service:" << serviceName;
        std::cout << "[PLC REQ] " << serviceName.toStdString() << std::endl;
        std::cout << "[PLC REQ] URL: " << url.toString().toStdString() << std::endl;
        std::cout << "[PLC REQ] Body: " << QString(data).toStdString() << std::endl;
        std::cout << "[PLC RSP] EMPTY" << std::endl;
        return QJsonValue();
    }

    QJsonParseError parseError;
    QJsonDocument responseDoc = QJsonDocument::fromJson(responseData, &parseError);
    if (parseError.error != QJsonParseError::NoError)
    {
        qWarning() << "PlcClient: JSON parse error:" << parseError.errorString();
        std::cout << "[PLC REQ] " << serviceName.toStdString() << std::endl;
        std::cout << "[PLC REQ] URL: " << url.toString().toStdString() << std::endl;
        std::cout << "[PLC REQ] Body: " << QString(data).toStdString() << std::endl;
        std::cout << "[PLC RSP] PARSE ERROR: " << parseError.errorString().toStdString() << std::endl;
        std::cout << "[PLC RSP] Raw: " << QString(responseData).toStdString() << std::endl;
        return QJsonValue();
    }

    QJsonObject responseObj = responseDoc.object();

    QString type = responseObj["type"].toString();
    if (type == "FAULT")
    {
        qWarning() << "PlcClient: PLC returned FAULT for service:" << serviceName;
        std::cout << "[PLC REQ] " << serviceName.toStdString() << std::endl;
        std::cout << "[PLC REQ] URL: " << url.toString().toStdString() << std::endl;
        std::cout << "[PLC REQ] Body: " << QString(data).toStdString() << std::endl;
        std::cout << "[PLC RSP] FAULT: " << QString(responseData).toStdString() << std::endl;
        return QJsonValue();
    }

    return responseObj["value"];
}

bool PlcClient::fetchMachinePartProperties(const QString &mpFullPath,
                                            MachinePartProperties &out)
{
    QJsonObject value;
    value["machinePartFullPath"] = mpFullPath;

    QJsonArray inputArgs;
    inputArgs.append(value);

    QJsonValue reply = sendRequest("getMachinePartProperties", inputArgs);
    if (reply.isUndefined() || reply.isNull() || !reply.isObject())
    {
        qWarning() << "PlcClient: getMachinePartProperties failed for" << mpFullPath;
        return false;
    }

    QJsonObject obj = reply.toObject();

    out.machinePart = obj["machinePart"].toObject();
    if (out.machinePart.isEmpty())
    {
        qWarning() << "PlcClient: machinePart is empty for" << mpFullPath;
        return false;
    }

    QJsonArray cmds = obj["commands"].toArray();
    for (const QJsonValue &v : cmds)
    {
        out.commands.append(v.toObject());
    }

    QJsonArray stgs = obj["settings"].toArray();
    for (const QJsonValue &v : stgs)
    {
        out.settings.append(v.toObject());
    }

    QJsonArray obss = obj["observers"].toArray();
    for (const QJsonValue &v : obss)
    {
        out.observers.append(v.toObject());
    }

    std::cout << "[PLC INIT] getMachinePartProperties OK"
              << " commands=" << cmds.size()
              << " settings=" << stgs.size()
              << " observers=" << obss.size()
              << " machinePart=" << out.machinePart["name"].toString().toStdString() << std::endl;

    return true;
}

int PlcClient::executeCommand(const QJsonObject &machinePart,
                               const QJsonObject &cmdTemplate,
                               const QList<double> &values)
{
    QJsonArray args = cmdTemplate["arguments"].toArray();

    if (values.size() != args.size())
    {
        qWarning() << "PlcClient: value count" << values.size()
                    << "doesn't match argument count" << args.size()
                    << "for command" << cmdTemplate["name"].toString();
        return 1;
    }

    for (int i = 0; i < args.size(); i++)
    {
        QJsonObject arg = args[i].toObject();
        arg["currentValue"] = values[i];
        arg["savedValue"]   = values[i];
        args[i] = arg;
    }

    QJsonObject cmdObj;
    cmdObj["name"]        = cmdTemplate["name"];
    cmdObj["machinePart"] = machinePart;
    cmdObj["arguments"]   = args;

    QJsonObject input;
    input["command"] = cmdObj;

    QJsonArray inputArgs;
    inputArgs.append(input);

    std::cout << "[PLC CMD] " << cmdTemplate["name"].toString().toStdString()
              << " values=";
    for (double v : values)
    {
        std::cout << v << " ";
    }
    std::cout << std::endl;

#ifdef IO_LOCAL_DEBUG
    std::cout << "[PLC CMD] LOCAL_DEBUG skip" << std::endl;
    return 0;
#endif

    QJsonValue reply = sendRequest("executeCommand", inputArgs);
    if (reply.isUndefined() || reply.isNull())
    {
        std::cout << "[PLC CMD] executeCommand FAILED (timeout/empty/fault)" << std::endl;
        return 1;
    }
    std::cout << "[PLC CMD] executeCommand OK" << std::endl;
    return 0;
}

QVariantList PlcClient::getObserverValues(const QString &mpFullPath,
                                           const QStringList &obsNames)
{
    QJsonArray paths;
    for (const QString &name : obsNames)
    {
        QJsonObject p;
        p["path"] = mpFullPath + "/" + name;
        paths.append(p);
    }

    QJsonObject input;
    input["observerPaths"] = paths;

    QJsonArray inputArgs;
    inputArgs.append(input);

    QJsonValue reply = sendRequest("getObserverValuesFromObserverPaths", inputArgs);
    if (!reply.isArray())
    {
        std::cout << "[PLC OBS] getObserverValues FAILED"
                  << " isNull=" << reply.isNull()
                  << " isUndefined=" << reply.isUndefined()
                  << " isObject=" << reply.isObject()
                  << " isArray=" << reply.isArray() << std::endl;
        return QVariantList();
    }

    QVariantList result;
    QJsonArray arr = reply.toArray();
    for (const QJsonValue &v : arr)
    {
        result.append(v.toObject()["currentValue"].toVariant());
    }
    return result;
}

int PlcClient::setSettingValue(const QString &mpFullPath,
                                const QString &settingName,
                                const QJsonObject &typedValue)
{
    QJsonObject settingKey;
    settingKey["machinePartFullPath"] = mpFullPath;
    settingKey["commandName"] = QString("");
    settingKey["settingName"] = settingName;

    QJsonObject value;
    value["dummyKey"] = QString("");
    value["type"] = typedValue["type"];
    for (auto it = typedValue.begin(); it != typedValue.end(); ++it)
    {
        if (it.key() != "type")
            value[it.key()] = it.value();
    }

    QJsonObject input;
    input["settingKey"] = settingKey;
    input["value"] = value;

    QJsonArray inputArgs;
    inputArgs.append(input);

    std::cout << "[PLC SETTING] setSettingValue"
              << " path=" << mpFullPath.toStdString()
              << " name=" << settingName.toStdString()
              << " type=" << typedValue["type"].toString().toStdString() << std::endl;

#ifdef IO_LOCAL_DEBUG
    std::cout << "[PLC SETTING] LOCAL_DEBUG skip" << std::endl;
    return 0;
#endif

    QJsonValue reply = sendRequest("setCurrentValueForSetting", inputArgs);
    if (reply.isUndefined() || reply.isNull())
    {
        std::cout << "[PLC SETTING] setSettingValue FAILED" << std::endl;
        return 1;
    }
    std::cout << "[PLC SETTING] setSettingValue OK" << std::endl;
    return 0;
}

int PlcClient::saveSettingValue(const QString &mpFullPath,
                                 const QString &settingName)
{
    QJsonObject settingKey;
    settingKey["machinePartFullPath"] = mpFullPath;
    settingKey["settingName"] = settingName;

    QJsonObject input;
    input["settingKey"] = settingKey;

    QJsonArray inputArgs;
    inputArgs.append(input);

    std::cout << "[PLC SETTING] saveSettingValue"
              << " path=" << mpFullPath.toStdString()
              << " name=" << settingName.toStdString() << std::endl;

#ifdef IO_LOCAL_DEBUG
    std::cout << "[PLC SETTING] LOCAL_DEBUG skip" << std::endl;
    return 0;
#endif

    QJsonValue reply = sendRequest("saveCurrentValueForSetting", inputArgs);
    if (reply.isUndefined() || reply.isNull())
    {
        std::cout << "[PLC SETTING] saveSettingValue FAILED" << std::endl;
        return 1;
    }
    std::cout << "[PLC SETTING] saveSettingValue OK" << std::endl;
    return 0;
}
