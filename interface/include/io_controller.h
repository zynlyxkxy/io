/*********************************************************************************
* @Copyright(C), 2026 ISTAR. All rights reserved.
* IO — hand-written, fixed RPC interface (gen_rpc_v2.py input)
*
* Interfaces grouped by function (read/write/push), not by ACS/PLC.
* varId is globally unique; control layer auto-routes to ACS or PLC by
* range (varId / 1000).
* Different machines are distinguished by ZMQ IP:port.
*********************************************************************************/
#ifndef __IOV2_IO_CONTROLLER_H__
#define __IOV2_IO_CONTROLLER_H__

#include "istar_platform_basicdata.h"
#include "ioc_data.h"

#ifndef request
#define request
#define subscribe
#define publish
#endif

namespace IOModule
{

class UIIoController
{
public:
    // ==================== Server info ====================
    request virtual ISTARResultStatus IOGetServerInfo(OUT IoServerInfo &result) = 0;

    // ==================== AI / DI read ====================

    // forceRefresh=false read cache, true read directly from hardware
    request virtual ISTARResultStatus IOGetAi(IN IoReadInput input, OUT IoAiOutput &output) = 0;

    request virtual ISTARResultStatus IOGetDi(IN IoReadInput input, OUT IoDiOutput &output) = 0;

    // ==================== AO / DO read ====================

    request virtual ISTARResultStatus IOGetAo(IN IoReadInput input, OUT IoAiOutput &output) = 0;

    request virtual ISTARResultStatus IOGetDo(IN IoReadInput input, OUT IoDiOutput &output) = 0;

    // ==================== AO / DO write ====================
    // If verify=true, pushes IOSetCompleted after async verification;
    // returns ERR_BUSY while same varId is pending.

    request virtual ISTARResultStatus IOSetAo(IN IoAoInput input) = 0;

    request virtual ISTARResultStatus IOSetDo(IN IoDoInput input) = 0;

    // ==================== Buffer (ACS only) ====================

    request virtual ISTARResultStatus IOStartBuffer(IN IoVarInput input) = 0;

    request virtual ISTARResultStatus IOStopBuffer(IN IoVarInput input) = 0;

    request virtual ISTARResultStatus IOGetBufferRunning(IN IoReadInput input, OUT IoDiOutput &output) = 0;

    // ==================== Cmd (PLC only, may carry multiple values) ====================

    request virtual ISTARResultStatus IOExecCmd(IN IoExecCmdInput input) = 0;

    // ==================== Setting (PLC only) ====================

    request virtual ISTARResultStatus IOSetSettingDouble(IN IoSettingDoubleInput input) = 0;

    request virtual ISTARResultStatus IOSetSettingBool(IN IoSettingBoolInput input) = 0;

    // ==================== Set completion (server -> client push) ====================
    // Pushed after verification when verify=true;
    // errorCode: 0=pass, 1=timeout, 2=send failed.

    subscribe virtual ISTARResultStatus IOSetCompleted(OUT SetCompletedData &data) = 0;

    // ==================== Observer periodic push (server -> client) ====================
    // 一次推 ACS + PLC 全部缓存数据（聚合为一条消息）

    subscribe virtual ISTARResultStatus IOObserverData(OUT IoObserverData &data) = 0;

    // ==================== Heartbeat (1s) ====================

    subscribe virtual ISTARResultStatus IOHeartbeat(OUT IoHeartbeatData &data) = 0;

    // ==================== External custom (reserved) ====================
    // External -> server
    request virtual ISTARResultStatus IOCustomCommand(IN IoCustomCommand cmd) = 0;

    // Server -> external
    subscribe virtual ISTARResultStatus IOCustomNotify(OUT IoCustomNotifyData &data) = 0;
};

} // namespace IOModule

#endif // __IOV2_IO_CONTROLLER_H__
