// IO — hand-written, fixed RPC data structures
// Project-specific enum definitions: see config/io_config/io_var_id.h

#ifndef __IOV2_IO_DATA_H__
#define __IOV2_IO_DATA_H__

#include <string>
#include <vector>

namespace IOModule
{

// ===== Read input (shared by GetAi / GetDi / GetBufferRunning) =====
struct IoReadInput
{
    int varId;               // globally unique variable id
    bool forceRefresh;       // false=read cache, true=read directly from hardware
};

// ===== AI / AO read output =====
struct IoAiOutput
{
    int errorCode;           // 0=success, other=error code
    double value;
};

// ===== DI / DO read output =====
struct IoDiOutput
{
    int errorCode;
    bool value;
};

// ===== AO write input =====
struct IoAoInput
{
    int varId;
    double value;            // target value to write
    bool verify;
    double expectedValue;    // expected observer value when verify=true
};

// ===== DO write input =====
struct IoDoInput
{
    int varId;
    bool value;
    bool verify;
    bool expectedValue;      // expected observer value when verify=true
};

// ===== SettingDouble write input =====
struct IoSettingDoubleInput
{
    int varId;
    double value;
};

// ===== SettingBool write input =====
struct IoSettingBoolInput
{
    int varId;
    bool value;
};

// ===== Parameterless operation (shared by StartBuffer / StopBuffer) =====
struct IoVarInput
{
    int varId;
};

// ===== ExecCmd input (PLC command, values ordered as the command arguments) =====
struct IoExecCmdInput
{
    int varId;
    std::vector<double> values;      // positional values; empty for 0-param commands
};

// ===== Set completion (server -> client push) =====
// errorCode: 0=pass, 1=timeout, 2=send failed
struct SetCompletedData
{
    int varId;
    bool success;
    int errorCode;
};

// ===== Observer entries =====
struct ObserverDoubleEntry
{
    int varId;
    double value;
};

struct ObserverBoolEntry
{
    int varId;
    bool value;
};

// ===== ACS Observer periodic push (500ms) =====
struct AcsObserverData
{
    std::vector<ObserverDoubleEntry> AI;
    std::vector<ObserverBoolEntry>   DI;
    std::vector<ObserverDoubleEntry> AO;
    std::vector<ObserverBoolEntry>   DO;
    std::vector<ObserverBoolEntry>   Buf;
};

// ===== PLC Observer periodic push (500ms) =====
struct PlcObserverData
{
    std::vector<ObserverDoubleEntry> AI;
    std::vector<ObserverBoolEntry>   DI;
    std::vector<ObserverDoubleEntry> AO;
    std::vector<ObserverBoolEntry>   DO;
};

// ===== Observer 周期推送（一次推 ACS + PLC 全部缓存） =====
struct IoObserverData
{
    AcsObserverData acs;
    PlcObserverData plc;
};

// ===== Heartbeat (1s) =====
struct IoHeartbeatData
{
    long long timestamp;             // ms since epoch
};

// ===== IoServerInfo (returned by RpcGetServerInfo) =====
struct IoVarInfo
{
    int id;
    std::string name;
};

struct IoArgInfo
{
    std::string name;               // argument name (from PLC command template)
};

struct IoCmdVarInfo
{
    int id;
    std::string name;
    std::vector<IoArgInfo> args;    // command arguments; empty for 0-param commands
};

struct IoAoVarInfo
{
    int id;
    std::string name;
    std::string observer;            // bound observer name
};

struct IoDoVarInfo
{
    int id;
    std::string name;
    std::string observer;
};

struct IoInfo
{
    std::vector<IoVarInfo> AI;
    std::vector<IoVarInfo> DI;
    std::vector<IoAoVarInfo> AO;
    std::vector<IoDoVarInfo> DO;
    std::vector<IoVarInfo> SettingDouble;
    std::vector<IoVarInfo> SettingBool;
    std::vector<IoCmdVarInfo> Cmd;
    std::vector<IoVarInfo> Buffer;
};

struct IoServerInfo
{
    IoInfo io;
};

// ===== External custom command (external -> server, reserved) =====
struct IoCustomCommand
{
    std::vector<std::string> json;   // can send multiple JSON commands at once
};

// ===== Custom notification (server -> external, reserved) =====
struct IoCustomNotifyData
{
    std::vector<std::string> json;   // can push multiple JSON notifications at once
};

} // namespace IOModule

#endif // __IOV2_IO_DATA_H__
