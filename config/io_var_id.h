#ifndef __IO_VAR_ID_H__
#define __IO_VAR_ID_H__

// ===================================================================
// IO 变量 ID 枚举 — 由 gen_io_var_id.py 自动生成，请勿手动修改
//
// 号段划分: id / 1000 确定类型
//   0: PLC AI
//   1: ACS AI
//   2: PLC DI
//   3: ACS DI
//   4: PLC AO
//   5: ACS AO
//   6: PLC DO
//   7: ACS DO
//   8: SettingDouble / SettingBool
//   9: Buffer
//   10: Cmd
//
// 此文件随协议一同提交给上层，上层可自行改名或 include。
// ===================================================================

enum AiVarId {
    // PLC AI  (0..999)
    AI_PLC_oUI_Xg = 0,
    AI_PLC_oUI_Yg = 1,
    AI_PLC_oUI_Rxg = 2,
    AI_PLC_oUI_Ryg = 3,
    AI_PLC_oUI_Rzg = 4,
    // ACS AI  (1000..1999)
    AI_ACS_CurrentCycles = 1000,
};

enum DiVarId {
    // PLC DI  (2000..2999)
    DI_PLC_oYN_XN_WithChip = 2000,
    DI_PLC_oYP_XN_WithChip = 2001,
    DI_PLC_oFA_WithChip = 2002,
    DI_PLC_oWT_WithWafer = 2003,
    DI_PLC_iPhoto_Finish_First = 2004,
    // ACS DI  (3000..3999)
    DI_ACS_WS_HorizontalMoveFinish = 3000,
    DI_ACS_DriveCycleFinish = 3001,
    DI_ACS_BondingProcess_YN_XP = 3002,
    DI_ACS_BondingProcess_YP_XN = 3003,
    DI_ACS_BondingProcess_YP_XP = 3004,
};

enum AoVarId {
    // PLC AO  (4000..4999)
    AO_PLC_X_Move = 4000,
    AO_PLC_Y_Move = 4001,
    AO_PLC_Rz_Move = 4002,
    // ACS AO  (5000..5999)
    AO_ACS_TargetX = 5000,
    AO_ACS_TargetY = 5001,
};

enum DoVarId {
    // PLC DO  (6000..6999)
    DO_PLC_BH_YN_XN_Close = 6000,
    DO_PLC_BH_YN_XN_OpenNegative = 6001,
    DO_PLC_BH_YN_XN_OpenPositive = 6002,
    DO_PLC_BH_YP_XN_Close = 6003,
    DO_PLC_BH_YP_XN_OpenNegative = 6004,
    DO_PLC_BH_YP_XN_OpenPositive = 6005,
    DO_PLC_FA_Close = 6006,
    DO_PLC_FA_OpenNegative = 6007,
    DO_PLC_FA_OpenPositive = 6008,
    // ACS DO  (7000..7999)
    DO_ACS_PreciousAlignFinish = 7000,
};

enum SettingDoubleVarId {
    // SettingDouble / SettingBool  (8000..8999)
};

enum SettingBoolVarId {
    // SettingDouble / SettingBool  (8000..8999)
};

enum BufferVarId {
    // Buffer  (9000..9999)
    BUF_14 = 9000,
    BUF_20 = 9001,
    BUF_21 = 9002,
};

enum CmdVarId {
    // Cmd  (10000..10999)
};

#endif // __IO_VAR_ID_H__

