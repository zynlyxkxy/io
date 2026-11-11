#ifndef IOCONTROLLER_H
#define IOCONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QDateTime>

#include "ControlModule/ConfigManager/ConfigManager.h"
#include "AcsModule/AcsIoModule/AcsIoModule.h"
#include "PlcModule/PlcIoModule/PlcIoModule.h"
#include "io_server.h"

enum IoError
{
    IO_SUCCESS         = 0,
    IO_ERR_NOT_FOUND   = 1,
    IO_ERR_READ_FAILED  = 2,
    IO_ERR_WRITE_FAILED = 3,
    IO_ERR_BUSY         = 4,
};

struct PendingVerify
{
    int varId;
    int segment;            // 4=PLC_AO, 5=ACS_AO, 6=PLC_DO, 7=ACS_DO
    qint64 startTime;
    double expectedDouble;  // AO 用
    bool expectedBool;      // DO 用
};

class IoController : public QObject
{
    Q_OBJECT
public:
    explicit IoController(QObject *parent = nullptr);
    ~IoController();

    /// 初始化：configFilePath 为完整配置路径（<配置根>/io_config/config.json）；
    /// 其中的 acs/、plc/ 相对路径以该文件所在目录为基准解析
    bool init(const QString &configFilePath);

    // AI / DI 读
    int getAi(int varId, bool forceRefresh, double &out);
    int getDi(int varId, bool forceRefresh, bool &out);

    // AO / DO 读（读绑定的 observer）
    int getAo(int varId, bool forceRefresh, double &out);
    int getDo(int varId, bool forceRefresh, bool &out);

    // AO / DO 写
    int setAo(int varId, double value, bool verify, double expectedValue);
    int setDo(int varId, bool value, bool verify, bool expectedValue);

    // Buffer（仅 ACS）
    int startBuffer(int varId);
    int stopBuffer(int varId);
    int getBufferRunning(int varId, bool forceRefresh, bool &out);

    // Cmd（仅 PLC，values 按命令参数顺序，0 参数命令传空）
    int execCmd(int varId, const QList<double> &values);

    // Setting（仅 PLC）
    int setSettingDouble(int varId, double value);
    int setSettingBool(int varId, bool value);

    // IoServerInfo
    IoServerInfo getServerInfo() const;

    // Custom（预留）
    int execCustomCommand(const IoCustomCommand &cmd);
    void publishCustomNotify(const IoCustomNotifyData &data);

signals:
    void sig_ObserverDataReady(const IoObserverData &data);
    void sig_SetCompleted(int varId, bool success, int errorCode);
    void sig_HeartbeatReady();
    void sig_CustomNotifyReady(const IoCustomNotifyData &data);

private:
    void onObserverTimer();
    void onHeartbeat();
    void onVerifyCheck();

    static int segment(int varId)
    {
        return varId / 1000;
    }
    static bool isPlc(int varId)
    {
        return segment(varId) % 2 == 0;
    }
    bool isBusy(int varId) const;

    AcsIoModule *m_acsModule = nullptr;
    PlcIoModule *m_plcModule = nullptr;

    QTimer *m_heartbeatTimer = nullptr;
    QTimer *m_verifyTimer = nullptr;
    QTimer *m_observerTimer = nullptr;

    QList<PendingVerify> m_verifyQueue;

    ConfigManager &m_cfg;
    int m_observerPeriodMs = 500;
    int m_verifyTimeoutMs = 20000;
};

inline bool IoController::isBusy(int varId) const
{
    for (const auto &p : m_verifyQueue)
    {
        if (p.varId == varId)
        {
            return true;
        }
    }
    return false;
}

#endif // IOCONTROLLER_H
