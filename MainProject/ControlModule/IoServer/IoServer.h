#ifndef IOSERVER_H
#define IOSERVER_H

#include <QObject>
#include "io_server.h"
#include "ControlModule/Controller/IoController.h"

class IoServer : public QObject, public UIIoControllerServerImpl
{
    Q_OBJECT

public:
    explicit IoServer(IoController *controller,
                      const std::string &moduleName = "rpc");

    using UIIoControllerServerImpl::IOServerInit;

    // ===== IoServerInfo =====
    virtual ISTARResultStatus IOGetServerInfo(
        ResponseMessageStruct<IoServerInfo> &response) override;

    // ===== AI / DI 读 =====
    virtual ISTARResultStatus IOGetAi(
        IoReadInput input,
        ResponseMessageStruct<IoAiOutput> &response) override;

    virtual ISTARResultStatus IOGetDi(
        IoReadInput input,
        ResponseMessageStruct<IoDiOutput> &response) override;

    // ===== AO / DO 读 =====
    virtual ISTARResultStatus IOGetAo(
        IoReadInput input,
        ResponseMessageStruct<IoAiOutput> &response) override;

    virtual ISTARResultStatus IOGetDo(
        IoReadInput input,
        ResponseMessageStruct<IoDiOutput> &response) override;

    // ===== AO / DO 写 =====
    virtual ISTARResultStatus IOSetAo(
        IoAoInput input,
        ResponseNoDataMessageStruct &response) override;

    virtual ISTARResultStatus IOSetDo(
        IoDoInput input,
        ResponseNoDataMessageStruct &response) override;

    // ===== Buffer =====
    virtual ISTARResultStatus IOStartBuffer(
        IoVarInput input,
        ResponseNoDataMessageStruct &response) override;

    virtual ISTARResultStatus IOStopBuffer(
        IoVarInput input,
        ResponseNoDataMessageStruct &response) override;

    virtual ISTARResultStatus IOGetBufferRunning(
        IoReadInput input,
        ResponseMessageStruct<IoDiOutput> &response) override;

    // ===== Cmd =====
    virtual ISTARResultStatus IOExecCmd(
        IoExecCmdInput input,
        ResponseNoDataMessageStruct &response) override;

    // ===== Setting =====
    virtual ISTARResultStatus IOSetSettingDouble(
        IoSettingDoubleInput input,
        ResponseNoDataMessageStruct &response) override;

    virtual ISTARResultStatus IOSetSettingBool(
        IoSettingBoolInput input,
        ResponseNoDataMessageStruct &response) override;

    // ===== Custom（预留） =====
    virtual ISTARResultStatus IOCustomCommand(
        IoCustomCommand cmd,
        ResponseNoDataMessageStruct &response) override;

private:
    IoController *m_controller;
    std::string m_moduleName;
};

#endif // IOSERVER_H
