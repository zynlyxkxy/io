#include "AcsClient.h"

#include <QThread>
#include <QtConcurrent/QtConcurrent>
#include <QDebug>
#include <QTimer>
#include <QHash>
#include <cstdlib>

#define LOGRETURN \
    do \
    { \
        if (bRes) \
        { \
            return true; \
        } \
        else \
        { \
            int err_code = acsc_GetLastError(); \
            char err_note[128] = {}; \
            int recv = 0; \
            acsc_GetErrorString(m_handle, err_code, err_note, 128, &recv); \
            qDebug() << "ACS" << __func__ << "failed, errCode:" << err_code << "errMsg:" << err_note; \
            return false; \
        } \
    } while (0);

AcsClient::AcsClient(const QString &ip, int port, QObject *parent)
    : QObject(parent)
    , m_handle(NULL)
    , m_bRunning(false)
    , m_ip(ip)
    , m_port(port)
    , m_initDISPCH(0)
{
    m_socket = new QTcpSocket(this);
}

AcsClient::~AcsClient()
{
    Disconnect();
}

bool AcsClient::Connect()
{
#ifdef IO_LOCAL_DEBUG
    m_bRunning = true;
    return true;
#endif
    if (m_handle)
    {
        acsc_CloseHistoryBuffer(m_handle);
        acsc_CloseComm(m_handle);
    }

    m_handle = ACSC_INVALID;
    m_bRunning = true;

    tryConnect();

    if (m_handle == ACSC_INVALID)
    {
        return false;
    }
    else
    {
        acsc_OpenHistoryBuffer(m_handle, 100000);
        m_connectionCheck = QtConcurrent::run(this, &AcsClient::realTimeConCheck);
        return true;
    }
}

bool AcsClient::Disconnect()
{
#ifdef IO_LOCAL_DEBUG
    m_bRunning = false;
    return true;
#endif
    if (m_handle == ACSC_INVALID)
    {
        return true;
    }

    m_bRunning = false;
    bool bRes = acsc_CloseComm(m_handle);
    m_handle = ACSC_INVALID;
    m_socket->disconnectFromHost();
    m_socket->waitForDisconnected();
    if (bRes)
    {
        qDebug() << "Acs driver disconnected";
    }
    LOGRETURN;
}

// ========== variable read/write ==========

bool AcsClient::ReadInteger(QByteArray varName, int *pVal)
{
#ifdef IO_LOCAL_DEBUG
    if (pVal)
    {
        int h = qHash(varName);
        *pVal = (h % 2 == 0) ? 1 : 0;
    }
    return true;
#endif
    bool bRes = acsc_ReadInteger(m_handle, ACSC_NONE, (char *)LPCTSTR(varName.data()),
                                  ACSC_NONE, ACSC_NONE, ACSC_NONE, ACSC_NONE, pVal, NULL);
    if (!bRes)
    {
        qDebug() << "Var:" << varName;
    }
    LOGRETURN;
}

bool AcsClient::ReadInteger(QByteArray varArrayName, int nXFrom, int nXTo, int nYFrom, int nYTo, int *pVal)
{
#ifdef IO_LOCAL_DEBUG
    if (pVal)
    {
        int h = qHash(varArrayName);
        *pVal = (h % 2 == 0) ? 1 : 0;
    }
    return true;
#endif
    bool bRes = acsc_ReadInteger(m_handle, ACSC_NONE, (char *)LPCTSTR(varArrayName.data()),
                                  nXFrom, nXTo, nYFrom, nYTo, pVal, NULL);
    if (!bRes)
    {
        qDebug() << "Var:" << varArrayName;
    }
    LOGRETURN;
}

bool AcsClient::WriteInteger(QByteArray varName, int nVal)
{
#ifdef IO_LOCAL_DEBUG
    return true;
#endif
    bool bRes = acsc_WriteInteger(m_handle, ACSC_NONE, (char *)LPCTSTR(varName.data()),
                                   ACSC_NONE, ACSC_NONE, ACSC_NONE, ACSC_NONE, &nVal, NULL);
    if (!bRes)
    {
        qDebug() << "Var:" << varName;
    }
    LOGRETURN;
}

bool AcsClient::WriteInteger(QByteArray varArrayName, int nXFrom, int nXTo, int nYFrom, int nYTo, int *pVal)
{
#ifdef IO_LOCAL_DEBUG
    return true;
#endif
    bool bRes = acsc_WriteInteger(m_handle, ACSC_NONE, (char *)LPCTSTR(varArrayName.data()),
                                   nXFrom, nXTo, nYFrom, nYTo, pVal, NULL);
    if (!bRes)
    {
        qDebug() << "Var:" << varArrayName;
    }
    LOGRETURN;
}

bool AcsClient::ReadReal(QByteArray varName, double *pVal)
{
#ifdef IO_LOCAL_DEBUG
    if (pVal)
    {
        *pVal = (double)rand() / RAND_MAX * 10.0 - 5.0;
    }
    return true;
#endif
    bool bRes = acsc_ReadReal(m_handle, ACSC_NONE, (char *)LPCTSTR(varName.data()),
                               ACSC_NONE, ACSC_NONE, ACSC_NONE, ACSC_NONE, pVal, NULL);
    if (!bRes)
    {
        qDebug() << "Var:" << varName;
    }
    LOGRETURN;
}

bool AcsClient::ReadReal(QByteArray varArrayName, int nXFrom, int nXTo, int nYFrom, int nYTo, double *pVal)
{
#ifdef IO_LOCAL_DEBUG
    if (pVal)
    {
        *pVal = (double)rand() / RAND_MAX * 10.0 - 5.0;
    }
    return true;
#endif
    bool bRes = acsc_ReadReal(m_handle, ACSC_NONE, (char *)LPCTSTR(varArrayName.data()),
                               nXFrom, nXTo, nYFrom, nYTo, pVal, NULL);
    if (!bRes)
    {
        qDebug() << "Var:" << varArrayName;
    }
    LOGRETURN;
}

bool AcsClient::WriteReal(QByteArray varName, double dVal)
{
#ifdef IO_LOCAL_DEBUG
    return true;
#endif
    bool bRes = acsc_WriteReal(m_handle, ACSC_NONE, (char *)LPCTSTR(varName.data()),
                                0, 0, ACSC_NONE, ACSC_NONE, &dVal, NULL);
    if (!bRes)
    {
        qDebug() << "Var:" << varName;
    }
    LOGRETURN;
}

bool AcsClient::WriteReal(QByteArray varArrayName, int nXFrom, int nXTo, int nYFrom, int nYTo, double *pVal)
{
#ifdef IO_LOCAL_DEBUG
    return true;
#endif
    bool bRes = acsc_WriteReal(m_handle, ACSC_NONE, (char *)LPCTSTR(varArrayName.data()),
                                nXFrom, nXTo, nYFrom, nYTo, pVal, NULL);
    if (!bRes)
    {
        qDebug() << "Var:" << varArrayName;
    }
    LOGRETURN;
}

// ========== axis operations ==========

bool AcsClient::Enable(int nAxis)
{
#ifdef IO_LOCAL_DEBUG
    return true;
#endif
    bool bRes = acsc_Enable(m_handle, nAxis, NULL);
    LOGRETURN;
}

bool AcsClient::Disable(int nAxis)
{
#ifdef IO_LOCAL_DEBUG
    return true;
#endif
    bool bRes = acsc_Disable(m_handle, nAxis, NULL);
    LOGRETURN;
}

bool AcsClient::SetVel(int nAxis, double dVel)
{
#ifdef IO_LOCAL_DEBUG
    return true;
#endif
    bool bRes = acsc_SetVelocity(m_handle, nAxis, dVel, NULL);
    LOGRETURN;
}

bool AcsClient::GetVel(int nAxis, double *pVel)
{
#ifdef IO_LOCAL_DEBUG
    if (pVel) *pVel = 0.0;
    return true;
#endif
    if (!pVel)
    {
        return false;
    }
    bool bRes = acsc_GetVelocity(m_handle, nAxis, pVel, NULL);
    LOGRETURN;
}

bool AcsClient::SetAcc(int nAxis, double dAcc)
{
#ifdef IO_LOCAL_DEBUG
    return true;
#endif
    bool bRes = acsc_SetAcceleration(m_handle, nAxis, dAcc, NULL);
    LOGRETURN;
}

bool AcsClient::GetAcc(int nAxis, double *pAcc)
{
#ifdef IO_LOCAL_DEBUG
    if (pAcc) *pAcc = 0.0;
    return true;
#endif
    if (!pAcc)
    {
        return false;
    }
    bool bRes = acsc_GetAcceleration(m_handle, nAxis, pAcc, NULL);
    LOGRETURN;
}

bool AcsClient::SetDec(int nAxis, double dDec)
{
#ifdef IO_LOCAL_DEBUG
    return true;
#endif
    bool bRes = acsc_SetDeceleration(m_handle, nAxis, dDec, NULL);
    LOGRETURN;
}

bool AcsClient::GetDec(int nAxis, double *pDec)
{
#ifdef IO_LOCAL_DEBUG
    if (pDec) *pDec = 0.0;
    return true;
#endif
    if (!pDec)
    {
        return false;
    }
    bool bRes = acsc_GetDeceleration(m_handle, nAxis, pDec, NULL);
    LOGRETURN;
}

bool AcsClient::SetJerk(int nAxis, double dJerk)
{
#ifdef IO_LOCAL_DEBUG
    return true;
#endif
    bool bRes = acsc_SetJerk(m_handle, nAxis, dJerk, NULL);
    LOGRETURN;
}

bool AcsClient::SetKillDec(int nAxis, double dVal)
{
#ifdef IO_LOCAL_DEBUG
    return true;
#endif
    if (dVal == 0.0)
    {
        qDebug() << "Acs killDec is not allow to set 0.0";
        return false;
    }
    bool bRes = acsc_SetKillDeceleration(m_handle, nAxis, dVal, NULL);
    LOGRETURN;
}

bool AcsClient::Halt(int nAxis)
{
#ifdef IO_LOCAL_DEBUG
    return true;
#endif
    bool bRes = acsc_Halt(m_handle, nAxis, NULL);
    LOGRETURN;
}

bool AcsClient::MoveAbs(int nAxis, double dPos)
{
#ifdef IO_LOCAL_DEBUG
    return true;
#endif
    if (!Enable(nAxis))
    {
        qDebug() << "Acs PTP cannot move because axis not enabled";
        return false;
    }
    bool bRes = acsc_ToPoint(m_handle, 0, nAxis, dPos, NULL);
    LOGRETURN;
}

bool AcsClient::MoveRel(int nAxis, double dPos)
{
#ifdef IO_LOCAL_DEBUG
    return true;
#endif
    if (!Enable(nAxis))
    {
        qDebug() << "Acs PTPr cannot move because axis not enabled";
        return false;
    }
    bool bRes = acsc_ToPoint(m_handle, ACSC_AMF_RELATIVE, nAxis, dPos, NULL);
    LOGRETURN;
}

bool AcsClient::Jog(int nAxis, double dVel)
{
#ifdef IO_LOCAL_DEBUG
    return true;
#endif
    if (!Enable(nAxis))
    {
        qDebug() << "Acs Jog cannot move because axis not enabled";
        return false;
    }
    bool bRes = acsc_Jog(m_handle, ACSC_AMF_VELOCITY, nAxis, dVel, NULL);
    LOGRETURN;
}

bool AcsClient::GetMotorState(int nAxis, int *pState)
{
#ifdef IO_LOCAL_DEBUG
    if (pState) *pState = ACSC_MST_ENABLE | ACSC_MST_INPOS;
    return true;
#endif
    bool bRes = acsc_GetMotorState(m_handle, nAxis, pState, NULL);
    LOGRETURN;
}

bool AcsClient::GetFPos(int nAxis, double *pPos)
{
#ifdef IO_LOCAL_DEBUG
    if (pPos) *pPos = 0.0;
    return true;
#endif
    bool bRes = acsc_GetFPosition(m_handle, nAxis, pPos, NULL);
    LOGRETURN;
}

// ========== buffer operations ==========

bool AcsClient::CompileBuffer(int nBuffer)
{
#ifdef IO_LOCAL_DEBUG
    return true;
#endif
    bool bRes = acsc_CompileBuffer(m_handle, nBuffer, NULL);
    LOGRETURN;
}

bool AcsClient::StartBuffer(int nBuffer)
{
#ifdef IO_LOCAL_DEBUG
    return true;
#endif
    bool bRes = acsc_RunBuffer(m_handle, nBuffer, NULL, NULL);
    qDebug() << "Buffer state after call buffer run:" << IsBufferRunning(nBuffer);
    LOGRETURN;
}

bool AcsClient::StartBuffer(int nBuffer, QByteArray label)
{
#ifdef IO_LOCAL_DEBUG
    return true;
#endif
    bool bRes = acsc_RunBuffer(m_handle, nBuffer, label.data(), NULL);
    LOGRETURN;
}

bool AcsClient::StopBuffer(int nBuffer)
{
#ifdef IO_LOCAL_DEBUG
    return true;
#endif
    bool bRes = acsc_StopBuffer(m_handle, nBuffer, NULL);
    LOGRETURN;
}

bool AcsClient::GetBufferState(int nBuffer, int *pState)
{
#ifdef IO_LOCAL_DEBUG
    if (pState) *pState = 0;
    return true;
#endif
    bool bRes = acsc_GetProgramState(m_handle, nBuffer, pState, NULL);
    LOGRETURN;
}

bool AcsClient::IsBufferRunning(int nBuffer)
{
#ifdef IO_LOCAL_DEBUG
    return (nBuffer % 2 == 0);
#endif
    if (nBuffer < 0)
    {
        return false;
    }

    int nState;
    if (GetBufferState(nBuffer, &nState))
    {
        return ACSC_PST_RUN == (nState & ACSC_PST_RUN);
    }
    return false;
}

// ========== raw command ==========

bool AcsClient::SendRawCmd(QByteArray cmd, QByteArray &rcvData)
{
#ifdef IO_LOCAL_DEBUG
    rcvData = "";
    return true;
#endif
    char rcvBuff[100] = {};
    int numRecv;
    QByteArray cmdMd = cmd + "\r";

    bool bRes = acsc_Transaction(m_handle, cmdMd.data(), cmdMd.length(), rcvBuff, 99, &numRecv, NULL);

    if (!bRes)
    {
        qDebug() << "Cmd:" << cmdMd;
    }

    rcvData = rcvBuff;
    LOGRETURN;
}

// ========== connection management ==========

void AcsClient::realTimeConCheck()
{
    while (m_bRunning)
    {
        if (acsc_GetTimeout(m_handle) == 0)
        {
            emit Disconnected(1);
            tryConnect();
        }
        else
        {
            emit Disconnected(0);

            QByteArray rcvData;
            SendRawCmd((QString("DISPCH = ") + QString::number(m_initDISPCH)).toLatin1(), rcvData);
        }
        QThread::msleep(500);
    }
}

void AcsClient::tryConnect()
{
    int nMaxtry = 3;
    while (nMaxtry && ACSC_INVALID == m_handle)
    {
        m_handle = acsc_OpenCommEthernetTCP(m_ip.toLatin1().data(), ACSC_SOCKET_STREAM_PORT);
        qDebug() << "acs: try to connect using TCP-IP";
        qDebug() << m_ip.toLatin1().data();

        m_socket->connectToHost(m_ip, ACSC_SOCKET_STREAM_PORT);
        m_socket->waitForConnected();
        if (m_socket->state() == QAbstractSocket::ConnectedState)
        {
            qDebug() << "m_socket connected";
            m_socket->write("\rDISPCH=38\r");
            QEventLoop loop;
            QTimer::singleShot(500, &loop, &QEventLoop::quit);
            loop.exec();
            m_socket->readAll();

            m_socket->write("\r?COMMCH\r");

            QEventLoop loop2;
            QTimer::singleShot(1000, &loop2, &QEventLoop::quit);
            loop2.exec();

            QByteArray tcpBytes = m_socket->readAll();

            int len = tcpBytes.length();
            QByteArray tcpBytesTrunc = tcpBytes.mid(4, len - 4 - 2);
            int commch = tcpBytesTrunc.toInt();
            qDebug() << tcpBytesTrunc.data();
            qDebug() << "?COMMCH:" << commch;
            m_initDISPCH = commch;

            QString bytesWrite = "\rDISPCH=" + QString::number(commch) + "\r";
            m_socket->write(bytesWrite.toLatin1());
        }

        connect(m_socket, &QTcpSocket::readyRead, this, &AcsClient::procTCPData);

        QEventLoop loop;
        QTimer::singleShot(500, &loop, &QEventLoop::quit);
        loop.exec();

        --nMaxtry;
    }

    if (ACSC_INVALID == m_handle)
    {
        qDebug() << "Acs connected failed. ErrCode:" << acsc_GetLastError();
        m_bRunning = false;
    }
    else
    {
        qDebug() << "Acs connected";
        m_bRunning = true;
    }
}

void AcsClient::procTCPData()
{
    if (!m_socket)
    {
        return;
    }

    QByteArray tcpBytes = m_socket->readAll();
    QByteArray logbytes;
    logbytes.reserve(tcpBytes.length());

    QByteArray bytesHex = tcpBytes.toHex();

    for (int i = 0; i < bytesHex.length() / 2; i++)
    {
        QByteArray bytes = bytesHex.mid(2 * i, 2);
        if (bytes == "e5")
        {
            i += 3;
            continue;
        }
        if (bytes == "0d")
        {
            continue;
        }
        if (bytes == "e6")
        {
            QByteArray bytesfromHex = QByteArray::fromHex(logbytes);
            qDebug() << "ACS FIRMWARE LOG:" << bytesfromHex.data();
            logbytes = "";
            continue;
        }

        logbytes.append(bytesHex[2 * i]);
        logbytes.append(bytesHex[2 * i + 1]);
    }
}
