#ifndef CONFIG_DATA_H
#define CONFIG_DATA_H

#include "istar_serializer.hpp"

#include <string>
#include <vector>

// ===== config.json =====

struct S_AcsConfig {
    std::string ip;
    int port = 0;
    int pollPeriodMs = 0;
    std::string configPath;
    YLT_REFL(S_AcsConfig, ip, port, pollPeriodMs, configPath);
};

struct S_PlcConfig {
    std::string ip;
    int port = 0;
    int pollPeriodMs = 0;
    std::string configPath;
    YLT_REFL(S_PlcConfig, ip, port, pollPeriodMs, configPath);
};

struct S_MainConfig {
    S_AcsConfig acs;
    S_PlcConfig plc;
    int observerPeriodMs = 0;
    int verifyTimeoutMs = 0;
    YLT_REFL(S_MainConfig, acs, plc, observerPeriodMs, verifyTimeoutMs);
};

// ===== acs/io_config.json =====

struct S_AcsVarEntry {
    int id = 0;
    std::string name;
    YLT_REFL(S_AcsVarEntry, id, name);
};

struct S_AcsBufEntry {
    int id = 0;
    int bufferId = 0;
    YLT_REFL(S_AcsBufEntry, id, bufferId);
};

struct S_AcsAoEntry {
    int id = 0;
    std::string name;
    std::string observer;
    YLT_REFL(S_AcsAoEntry, id, name, observer);
};

struct S_AcsDoEntry {
    int id = 0;
    std::string name;
    std::string observer;
    YLT_REFL(S_AcsDoEntry, id, name, observer);
};

struct S_AcsIoConfig {
    std::vector<S_AcsVarEntry> AI;
    std::vector<S_AcsVarEntry> DI;
    std::vector<S_AcsAoEntry> AO;
    std::vector<S_AcsDoEntry> DO;
    std::vector<S_AcsBufEntry> Buffer;
    YLT_REFL(S_AcsIoConfig, AI, DI, AO, DO, Buffer);
};

// ===== plc/io_config.json =====

struct S_PlcVarEntry {
    int id = 0;
    std::string name;
    std::string label;
    YLT_REFL(S_PlcVarEntry, id, name, label);
};
struct S_PlcCmdEntry {
    int id = 0;
    std::string name;
    std::string label;
    std::vector<std::string> args;   // 整理后的参数名（契约/展示用；执行按位置填 PLC 模板）
    YLT_REFL(S_PlcCmdEntry, id, name, label, args);
};
struct S_PlcAoEntry {
    int id = 0;
    std::string name;
    std::string label;
    std::string observer;
    YLT_REFL(S_PlcAoEntry, id, name, label, observer);
};
struct S_PlcDoEntry {
    int id = 0;
    std::string name;
    std::string label;
    std::string observer;
    YLT_REFL(S_PlcDoEntry, id, name, label, observer);
};
struct S_PlcMachinePart {
    std::string fullPath;
    std::vector<S_PlcVarEntry> AI;
    std::vector<S_PlcVarEntry> DI;
    std::vector<S_PlcAoEntry> AO;
    std::vector<S_PlcDoEntry> DO;
    std::vector<S_PlcCmdEntry> Cmd;
    std::vector<S_PlcVarEntry> SettingDouble;
    std::vector<S_PlcVarEntry> SettingBool;
    YLT_REFL(S_PlcMachinePart, fullPath, AI, DI, AO, DO, Cmd, SettingDouble, SettingBool);
};

struct S_PlcIoConfig {
    std::vector<S_PlcMachinePart> machineParts;
    YLT_REFL(S_PlcIoConfig, machineParts);
};

#endif // CONFIG_DATA_H
