# ACS IO Module 方案

## 目标

从运动台 ACS Driver 中剥离 Read/Write/Buffer 功能。不处理运动台控制。

---

## 实际文件位置

```
mainproject/acs_module/
├── AcsClient/
│   ├── AcsClient.h               ← 从运动台模块复制
│   ├── AcsClient.cpp
│   └── acs/                      ← ACS SDK
│       ├── include/ACSC.h
│       └── lib/                  ← ACSCL_x64.dll, WibuCm64.dll, ACSCL_x64.lib
├── AcsIoModule/
│   ├── AcsIoModule.h
│   └── AcsIoModule.cpp
```

---

## 整体架构（两层）

```
上层：AcsIoModule
  - 构造函数直接连接（IP + port），内部持有 AcsClient
  - setAo / setDo → WriteReal / WriteInteger，不预注册
  - getAi / getDi → forceRefresh=true 直读硬件，false 走缓存
  - startBuffer / stopBuffer / getBufferRunning
  - registerAi / registerDi / registerBuffer → 注册到轮询
  - 无预配置，外部送什么就用什么

下层：AcsClient（从运动台模块复制）
  - 变量读写（ReadReal / WriteReal / ReadInteger / WriteInteger）
  - 轴操作（Enable / Disable / MoveAbs / Jog 等）— 保留但上层不调用
  - Buffer 操作（CompileBuffer / StartBuffer / StopBuffer / GetBufferState / IsBufferRunning）
  - 连接管理（Connect / Disconnect / IsConnected / Disconnected 信号）
  - 原始指令（SendRawCmd）
```

---

## LOCAL_DEBUG 模拟

全局宏 `IO_LOCAL_DEBUG`（CMakeLists.txt 定义），AcsClient 层处理：

| 方法 | 模拟行为 |
|------|---------|
| `Connect()` | 设 m_bRunning=true，return true |
| `Disconnect()` | 设 m_bRunning=false，return true |
| `IsConnected()` | 始终 return true |
| `ReadInteger()` | `qHash(varName) % 2 == 0 ? 1 : 0` |
| `ReadReal()` | `rand() / RAND_MAX * 10.0 - 5.0` |
| `WriteInteger()` / `WriteReal()` | return true（无操作） |
| `GetMotorState()` | 返回 `ACSC_MST_ENABLE \| ACSC_MST_INPOS` |
| `GetFPos()` | 返回 0.0 |
| `IsBufferRunning()` | `nBuffer % 2 == 0` |
| 所有轴操作 (Enable/Jog/MoveAbs...) | return true |
| 所有 Buffer 操作 (Compile/Start/Stop) | return true |
| `SendRawCmd()` | 返回空字符串，return true |

---

## 轮询

```
onPollTimer():
  for each var in m_registeredAi:
    m_client->ReadReal(var, &val)
    m_cache["ai:" + var] = val
  for each var in m_registeredDi:
    m_client->ReadInteger(var, &ival)
    m_cache["di:" + var] = (bool)ival
  for each id in m_registeredBuffers:
    m_cache["buffer:" + id] = m_client->IsBufferRunning(id)
  emit sig_ObsRefreshed()
```

---

## 关键设计决策

1. **不需要预配置 IO 列表**：AO/DO 直接发送，AI/DI 从缓存读或 forceRefresh 直读。注册的意义只是让轮询定时器持续刷新。
2. **registerAi / registerDi 分开**：ACS 读 AI 用 ReadReal，读 DI 用 ReadInteger，必须区分类型。
3. **Buffer 预配置**：Buffer 已在 ACS 控制器上预配置，模块只负责启动/停止/查状态。
4. **不需要 executeCmd**：ACS 没有 cmd 的概念。
