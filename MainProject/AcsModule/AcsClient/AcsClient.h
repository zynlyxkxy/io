#ifndef ACSCLIENT_H
#define ACSCLIENT_H

#include "ACSC.h"

#include <QObject>
#include <QFuture>
#include <QTcpSocket>
#include <QString>

class AcsClient : public QObject
{
    Q_OBJECT
public:
    explicit AcsClient(const QString &ip, int port, QObject *parent = nullptr);
    ~AcsClient();

    bool Connect();
    bool Disconnect();
    bool IsConnected();

    // 变量读写
    bool ReadReal(QByteArray varName, double *pVal);
    bool ReadReal(QByteArray varArrayName, int nXFrom, int nXTo, int nYFrom, int nYTo, double *pVal);
    bool WriteReal(QByteArray varName, double dVal);
    bool WriteReal(QByteArray varArrayName, int nXFrom, int nXTo, int nYFrom, int nYTo, double *pVal);
    bool ReadInteger(QByteArray varName, int *pVal);
    bool ReadInteger(QByteArray varArrayName, int nXFrom, int nXTo, int nYFrom, int nYTo, int *pVal);
    bool WriteInteger(QByteArray varName, int nVal);
    bool WriteInteger(QByteArray varArrayName, int nXFrom, int nXTo, int nYFrom, int nYTo, int *pVal);

    // 轴操作
    bool Enable(int nAxis);
    bool Disable(int nAxis);
    bool SetVel(int nAxis, double dVel);
    bool GetVel(int nAxis, double *pVel);
    bool SetAcc(int nAxis, double dAcc);
    bool GetAcc(int nAxis, double *pAcc);
    bool SetDec(int nAxis, double dDec);
    bool GetDec(int nAxis, double *pDec);
    bool SetJerk(int nAxis, double dJerk);
    bool SetKillDec(int nAxis, double dVal);
    bool Halt(int nAxis);
    bool MoveAbs(int nAxis, double dPos);
    bool MoveRel(int nAxis, double dPos);
    bool Jog(int nAxis, double dVel);
    bool GetMotorState(int nAxis, int *pState);
    bool GetFPos(int nAxis, double *pPos);

    // Buffer 操作
    bool CompileBuffer(int nBuffer);
    bool StartBuffer(int nBuffer);
    bool StartBuffer(int nBuffer, QByteArray label);
    bool StopBuffer(int nBuffer);
    bool GetBufferState(int nBuffer, int *pState);
    bool IsBufferRunning(int nBuffer);

    // 原始指令
    bool SendRawCmd(QByteArray cmd, QByteArray &rcvData);

signals:
    void Disconnected(bool);

private:
    void realTimeConCheck();
    void tryConnect();
    void procTCPData();

    HANDLE m_handle;
    bool m_bRunning;
    QString m_ip;
    int m_port;
    QTcpSocket *m_socket;
    int m_initDISPCH;
    QFuture<void> m_connectionCheck;
};

inline bool AcsClient::IsConnected()
{
#ifdef IO_LOCAL_DEBUG
    return true;
#endif
    return m_handle != ACSC_INVALID;
}

#endif // ACSCLIENT_H
