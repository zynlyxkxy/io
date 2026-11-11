# PLC IO Module 方案 v2

## 1. 核心原则

**PlcIoModule 是纯硬件抽象层**，不关心业务归类。

- 只负责：轮询 observer、执行命令、读写 Setting
- **不负责**：AO/DO 与 observer 的绑定关系（IoController/ConfigManager 的事）
- AO 的 observer 本质就是 AI（FLOAT observer），逻辑归类由上层处理

---

## 2. 实际文件位置

```
mainproject/plc_module/
├── PlcClient/
│   ├── PlcClient.h
│   └── PlcClient.cpp
├── PlcIoModule/
│   ├── PlcIoModule.h
│   └── PlcIoModule.cpp
```

---

## 3. 整体架构

```
IoController                    ← 业务逻辑：绑定关系、数据归类
  └─ PlcIoModule                ← 硬件抽象：轮询、命令、Setting
       └─ PlcClient             ← HTTP/JSON 通信（QNetworkAccessManager）
```

---

## 4. PathInfo 结构

```cpp
struct PathInfo
{
    QString mpFullPath;
    QJsonObject machinePart;

    QHash<QString, QJsonObject> aoTemplates;    // 1 个 FLOAT 参数
    QHash<QString, QJsonObject> doTemplates;    // 1 个 BOOLEAN 参数
    QHash<QString, QJsonObject> cmdTemplates;   // 0 参数

    QSet<QString> aiNames;
    QSet<QString> diNames;

    QSet<QString> settingDoubleNames;  // FLOAT_TYPED_VALUE / INT_TYPED_VALUE
    QSet<QString> settingBoolNames;    // BOOLEAN_TYPED_VALUE
};
```

---

## 5. PlcIoModule 接口

```cpp
class PlcIoModule : public QObject
{
    Q_OBJECT
public:
    enum Error {
        Success = 0, DriverNotConnected, HttpTimeout, PlcFault,
        CmdNotFound, ObserverNotFound, SettingNotFound,
        PathNotFound, TypeMismatch,
    };

    // 路径管理
    bool addPath(const QString &mpFullPath);

    // AO / DO / Cmd（非 LOCAL_DEBUG 时需要模板）
    int setAo(const QString &mpFullPath, const QString &cmdName, double value);
    int setDo(const QString &mpFullPath, const QString &cmdName, bool value);
    int executeCmd(const QString &mpFullPath, const QString &cmdName);

    // AI / DI 读
    int getAi(const QString &mpFullPath, const QString &obsName, double &out,
              bool forceRefresh = false);
    int getDi(const QString &mpFullPath, const QString &obsName, bool &out,
              bool forceRefresh = false);

    // Setting
    int setSettingDouble(const QString &mpFullPath, const QString &name, double value);
    int setSettingBool(const QString &mpFullPath, const QString &name, bool value);

    // Observer 管理
    void registerObserver(const QString &mpFullPath, const QString &obsName);
    void setPollPeriod(int ms);
    void refreshAllObs();

signals:
    void sig_ObsRefreshed();
};
```

---

## 6. LOCAL_DEBUG 模拟

全局宏 `IO_LOCAL_DEBUG`，**在 PlcIoModule 层处理**（不在 PlcClient），因为需要根据 `aiNames`/`diNames` 区分生成策略：

| 方法 | 模拟行为 |
|------|---------|
| `getAi()` | `rand() / RAND_MAX * 10.0 - 5.0` — 模拟随机模拟量，匹配 ACS ReadReal |
| `getDi()` | `qHash(key) % 2 == 0` — 稳定 bool 值，匹配 ACS ReadInteger |
| `onPollTimer()` | 按 `aiNames`/`diNames` 分别生成：AI → rand()，DI → hash bool |
| `setAo()` | 直接 return Success，跳过模板查找 |
| `setDo()` | 直接 return Success，跳过模板查找 |
| `executeCmd()` | 直接 return Success，跳过模板查找 |
| `setSettingDouble()` | 直接 return Success，跳过模板校验 |
| `setSettingBool()` | 直接 return Success，跳过模板校验 |

**设计原因**：PlcClient 的 `getMachinePartProperties` mock 返回空模板，所以 `aoTemplates`/`doTemplates`/`cmdTemplates`/`settingDoubleNames`/`settingBoolNames` 均为空。在 PlcIoModule 层直接跳过可以避免 `CmdNotFound` / `SettingNotFound` 错误。

PlcClient 的 `sendRequest()` mock 仍处理 `getMachinePartProperties` 和兜底的 observer 请求，但正常流程下 observer 值由 PlcIoModule 直接生成。

---

## 7. 关键流程

### addPath（自动归类）

```
addPath(path):
  1. 查重
  2. fetchMachinePartProperties(path)
  3. 遍历 commands[]:
     arguments 为空          → cmdTemplates
     arguments 1个 FLOAT     → aoTemplates
     arguments 1个 BOOLEAN   → doTemplates
  4. 遍历 observers[]:
     FLOAT_OBSERVER          → aiNames
     BOOLEAN_OBSERVER        → diNames
  5. 遍历 settings[]:
     FLOAT/INT_TYPED_VALUE   → settingDoubleNames
     BOOLEAN_TYPED_VALUE     → settingBoolNames
```

### Observer 轮询

```
onPollTimer():
  按路径分组 m_pollKeys
  for each path:
    getObserverValues(path, names)  → HTTP 批量获取
    更新 m_obsCache
  emit sig_ObsRefreshed()
```

### AO/DO/Cmd 执行（非 LOCAL_DEBUG）

```
setAo(path, cmdName, value):
  1. findPath → PathInfo
  2. aoTemplates.find(cmdName) → 获取命令模板
  3. executeCommand(machinePart, template, {value})
     → HTTP POST executeCommand
```
