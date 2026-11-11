#include "IoServer.h"
#include "istar_log.h"

#include <iostream>

IoServer::IoServer(IoController *controller, const std::string &moduleName)
    : UIIoControllerServerImpl()
    , m_controller(controller)
    , m_moduleName(moduleName)
{
    // subscribe 推送（通过 lambda 适配 const & → 非 const &）
    connect(m_controller, &IoController::sig_ObserverDataReady,
            this, [this](const IoObserverData &d)
    {
        IoObserverData copy = d;
        IOObserverDataPublish(copy);
    });

    connect(m_controller, &IoController::sig_SetCompleted,
            this, [this](int varId, bool success, int errorCode)
    {
        SetCompletedData data;
        data.varId = varId;
        data.success = success;
        data.errorCode = errorCode;
        IOSetCompletedPublish(data);
    });

    connect(m_controller, &IoController::sig_HeartbeatReady,
            this, [this]()
    {
        // 平台无参心跳(StatusUpload)：1s 一发，无载荷，客户端收到即判活
        IOStatusUploadPublish();
    });

    connect(m_controller, &IoController::sig_CustomNotifyReady,
            this, [this](const IoCustomNotifyData &d)
    {
        IoCustomNotifyData copy = d;
        IOCustomNotifyPublish(copy);
    });
}

// ===== IoServerInfo =====

ISTARResultStatus IoServer::IOGetServerInfo(
    ResponseMessageStruct<IoServerInfo> &response)
{
    response.data = m_controller->getServerInfo();
    std::cout << "[RPC] GetServerInfo OK" << std::endl;
    return ISTAR_RESULT_SUCCESS;
}

// ===== AI / DI 读 =====

ISTARResultStatus IoServer::IOGetAi(
    IoReadInput input,
    ResponseMessageStruct<IoAiOutput> &response)
{
    IoAiOutput out;
    out.errorCode = m_controller->getAi(input.varId, input.forceRefresh, out.value);
    response.data = out;
    response.errorCode = (ISTARResultStatus)(out.errorCode);
    std::cout << "[RPC] GetAi varId=" << input.varId
              << " forceRefresh=" << input.forceRefresh
              << " val=" << out.value
              << " err=" << out.errorCode << std::endl;
    return ISTAR_RESULT_SUCCESS;
}

ISTARResultStatus IoServer::IOGetDi(
    IoReadInput input,
    ResponseMessageStruct<IoDiOutput> &response)
{
    IoDiOutput out;
    out.errorCode = m_controller->getDi(input.varId, input.forceRefresh, out.value);
    response.data = out;
    response.errorCode = (ISTARResultStatus)(out.errorCode);
    std::cout << "[RPC] GetDi varId=" << input.varId
              << " forceRefresh=" << input.forceRefresh
              << " val=" << out.value
              << " err=" << out.errorCode << std::endl;
    return ISTAR_RESULT_SUCCESS;
}

// ===== AO / DO 读 =====

ISTARResultStatus IoServer::IOGetAo(
    IoReadInput input,
    ResponseMessageStruct<IoAiOutput> &response)
{
    IoAiOutput out;
    out.errorCode = m_controller->getAo(input.varId, input.forceRefresh, out.value);
    response.data = out;
    response.errorCode = (ISTARResultStatus)(out.errorCode);
    std::cout << "[RPC] GetAo varId=" << input.varId
              << " forceRefresh=" << input.forceRefresh
              << " val=" << out.value
              << " err=" << out.errorCode << std::endl;
    return ISTAR_RESULT_SUCCESS;
}

ISTARResultStatus IoServer::IOGetDo(
    IoReadInput input,
    ResponseMessageStruct<IoDiOutput> &response)
{
    IoDiOutput out;
    out.errorCode = m_controller->getDo(input.varId, input.forceRefresh, out.value);
    response.data = out;
    response.errorCode = (ISTARResultStatus)(out.errorCode);
    std::cout << "[RPC] GetDo varId=" << input.varId
              << " forceRefresh=" << input.forceRefresh
              << " val=" << out.value
              << " err=" << out.errorCode << std::endl;
    return ISTAR_RESULT_SUCCESS;
}

// ===== AO / DO 写 =====

ISTARResultStatus IoServer::IOSetAo(
    IoAoInput input,
    ResponseNoDataMessageStruct &response)
{
    int ret = m_controller->setAo(input.varId, input.value, input.verify, input.expectedValue);
    std::cout << "[RPC] SetAo varId=" << input.varId << " val=" << input.value
              << " verify=" << input.verify << " expect=" << input.expectedValue
              << " ret=" << ret << std::endl;
    response.bSuccess = (ret == 0);
    response.errorCode = (ISTARResultStatus)(ret);
    return ISTAR_RESULT_SUCCESS;
}

ISTARResultStatus IoServer::IOSetDo(
    IoDoInput input,
    ResponseNoDataMessageStruct &response)
{
    int ret = m_controller->setDo(input.varId, input.value, input.verify, input.expectedValue);
    std::cout << "[RPC] SetDo varId=" << input.varId << " val=" << input.value
              << " verify=" << input.verify << " expect=" << input.expectedValue
              << " ret=" << ret << std::endl;
    response.bSuccess = (ret == 0);
    response.errorCode = (ISTARResultStatus)(ret);
    return ISTAR_RESULT_SUCCESS;
}

// ===== Buffer =====

ISTARResultStatus IoServer::IOStartBuffer(
    IoVarInput input,
    ResponseNoDataMessageStruct &response)
{
    std::cout << "[RPC] StartBuffer varId=" << input.varId << std::endl;
    int ret = m_controller->startBuffer(input.varId);
    response.bSuccess = (ret == 0);
    response.errorCode = (ISTARResultStatus)(ret);
    return ISTAR_RESULT_SUCCESS;
}

ISTARResultStatus IoServer::IOStopBuffer(
    IoVarInput input,
    ResponseNoDataMessageStruct &response)
{
    std::cout << "[RPC] StopBuffer varId=" << input.varId << std::endl;
    int ret = m_controller->stopBuffer(input.varId);
    response.bSuccess = (ret == 0);
    response.errorCode = (ISTARResultStatus)(ret);
    return ISTAR_RESULT_SUCCESS;
}

ISTARResultStatus IoServer::IOGetBufferRunning(
    IoReadInput input,
    ResponseMessageStruct<IoDiOutput> &response)
{
    IoDiOutput out;
    out.errorCode = m_controller->getBufferRunning(input.varId, input.forceRefresh, out.value);
    std::cout << "[RPC] GetBufferRunning varId=" << input.varId
              << " forceRefresh=" << input.forceRefresh
              << " val=" << out.value
              << " err=" << out.errorCode << std::endl;
    response.data = out;
    response.errorCode = (ISTARResultStatus)(out.errorCode);
    return ISTAR_RESULT_SUCCESS;
}

// ===== Cmd =====

ISTARResultStatus IoServer::IOExecCmd(
    IoExecCmdInput input,
    ResponseNoDataMessageStruct &response)
{
    std::cout << "[RPC] ExecCmd varId=" << input.varId
              << " values=" << input.values.size() << std::endl;

    QList<double> values;
    for (double v : input.values)
    {
        values.append(v);
    }

    int ret = m_controller->execCmd(input.varId, values);
    response.bSuccess = (ret == 0);
    response.errorCode = (ISTARResultStatus)(ret);
    return ISTAR_RESULT_SUCCESS;
}

// ===== Setting =====

ISTARResultStatus IoServer::IOSetSettingDouble(
    IoSettingDoubleInput input,
    ResponseNoDataMessageStruct &response)
{
    int ret = m_controller->setSettingDouble(input.varId, input.value);
    std::cout << "[RPC] SetSettingDouble varId=" << input.varId
              << " val=" << input.value << " ret=" << ret << std::endl;
    response.bSuccess = (ret == 0);
    response.errorCode = (ISTARResultStatus)(ret);
    return ISTAR_RESULT_SUCCESS;
}

ISTARResultStatus IoServer::IOSetSettingBool(
    IoSettingBoolInput input,
    ResponseNoDataMessageStruct &response)
{
    int ret = m_controller->setSettingBool(input.varId, input.value);
    std::cout << "[RPC] SetSettingBool varId=" << input.varId
              << " val=" << input.value << " ret=" << ret << std::endl;
    response.bSuccess = (ret == 0);
    response.errorCode = (ISTARResultStatus)(ret);
    return ISTAR_RESULT_SUCCESS;
}

// ===== Custom（预留） =====

ISTARResultStatus IoServer::IOCustomCommand(
    IoCustomCommand cmd,
    ResponseNoDataMessageStruct &response)
{
    std::cout << "[RPC] IoCustomCommand jsonCount=" << cmd.json.size() << std::endl;
    m_controller->execCustomCommand(cmd);
    response.bSuccess = true;
    return ISTAR_RESULT_SUCCESS;
}
