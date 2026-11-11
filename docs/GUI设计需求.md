# GUI 设计需求

## 1. 实际文件结构

```
gui/
├── CMakeLists.txt
├── main.cpp
├── Common/
│   └── Serializer/
│       ├── qserializer.h
│       └── qserializer.cpp
├── MainWindow/
│   ├── mainwindow.h
│   └── mainwindow.cpp
├── IoGroup/
│   ├── iogroup.h                    ← 主页面，包含 QTabWidget（6 个 tab）
│   └── iogroup.cpp
├── IoWidget/
│   ├── aiwidget.h / .cpp / .ui
│   ├── aowidget.h / .cpp / .ui
│   ├── diwidget.h / .cpp / .ui
│   ├── dowidget.h / .cpp / .ui
│   ├── settingwidget.h / .cpp / .ui
│   └── bufferwidget.h / .cpp / .ui
└── rpc/
    └── api/
        ├── rpc_controller.h         ← 从 mainproject 同步
        └── rpc_data.h               ← 从 mainproject 同步
```

**MainWindow**：通过 RPC 连接服务端。收到第一个心跳后触发 `discoverIo()` → `RpcGetServerInfo` 获取 IO 目录 → `buildTabs` 创建界面 → `setRpcClient` 连接信号。心跳 watchdog 每 1s 检查，超时 5s 自动重连。

**IoGroup**：6 个 tab。收到 RPC observer 推送后更新 widget。用户写入时通过 RPC 下发。

---

## 2. 连接方式

GUI 通过 ZMQ RPC 连接服务端（mainproject），从 `mw_config.yaml` 读取中间件配置。服务端启动后 GUI 调用 `RpcGetServerInfo` 获取完整 IO 目录（ServerInfo），据此动态创建界面。

不再使用 `config_gui.json` 预配置 server 列表。

---

## 3. IoGroup 页面布局

使用 `QTabWidget`，6 个 tab，顺序：**AI → DI → AO → DO → Setting → Buffer**

### Tab 1-4: AI / DI / AO / DO

每个 tab 内两列布局：**PLC（左列）| ACS（右列）**，列头蓝色分割线。widget 按 `varId % 1000` 确定行号。

```
┌──────────────────────────────────────────────┐
│  QScrollArea                                 │
│  ┌───PLC────┐  ┌───ACS────┐                 │
│  │ Widget   │  │ Widget   │                 │
│  │ [id][name][value] │ [id][name][value] │   │
│  │ Widget   │  │ Widget   │                 │
│  └──────────┘  └──────────┘                 │
│  QGridLayout, 2 列                           │
└──────────────────────────────────────────────┘
```

### Tab 5: Setting

两列布局：**Bool（左列）| Double（右列）**

### Tab 6: Buffer

单列布局。

---

## 4. Widget 设计

### AiWidget / AoWidget — `[id:50px] [name] [value:DoubleSpinBox]`
- `lbl_Index`：4 位补零 varId
- `lbl_Name`：变量名
- `spb_Value`：只读，observer 推送更新，6 位小数

### DiWidget — `[id:50px] [name] [btn_Status:25x25]`
- `lbl_Index` + `lbl_Name` + `btn_Status`
- 只读绿色/灰色圆点

### DoWidget — `[id:50px] [name] [btn_Status:25x25]`
- `lbl_Index` + `lbl_Name` + `btn_Status`
- 可点击切换，emit `setDo(varId, bool)` → RPC `RpcSetDo`

### SettingWidget — `[id:50px] [name] [spinBox/btn_Toggle] [lbl_BoolText] [btn_Set]`
- Double 模式（isDouble=true）：显示 SpinBox（范围 ±999999，6 位小数），隐藏 Toggle 和 BoolText
- Bool 模式（isDouble=false）：显示 Toggle 按钮 + "On"/"Off" 标签 + Set 按钮，隐藏 SpinBox
- BoolText 默认值与 Toggle 未选中状态一致（"Off"）
- 点击 Set → emit `setSettingDouble(varId, value)` 或 `setSettingBool(varId, value)`

### BufferWidget — `[id:50px] [name] [btn_Start] [btn_Stop] [status_label]`
- 启动/停止按钮 + 运行状态指示灯

---

## 5. 数据流

```
[RPC Server] → ZMQ → [RPC Client (GUI)]
                           │
          subscribe 回调 ──────────────────────────┐
          RpcAcsObserverData                        │
          RpcPlcObserverData                        │
                  │                                 │
                  ▼                                 │
          emit sig_AcsObserverReceived              │
          emit sig_PlcObserverReceived              │
                  │ (Qt::QueuedConnection)          │
                  ▼                                 │
          IoGroup::onAcsObserverData()              │
          IoGroup::onPlcObserverData()              │
             │                                      │
             ├─ AI → m_aiWidgets[id]->display(value)│
             ├─ DI → m_diWidgets[id]->display(value)│
             ├─ AO → m_aoWidgets[id]->display(value)│
             ├─ DO → m_doWidgets[id]->display(value)│
             └─ Buf → m_bufferWidgets[id]->display  │

用户操作:
  DoWidget → emit setDo → connectSignals lambda
    → m_rpc->RpcSetDo({varId, value, verify=false})

  AoWidget Set → emit setAo → connectSignals lambda
    → m_rpc->RpcSetAo({varId, value, verify=false})

  SettingWidget Set → emit setSettingDouble/setSettingBool
    → m_rpc->RpcSetSettingDouble / RpcSetSettingBool

  BufferWidget → emit startBuffer/stopBuffer
    → m_rpc->RpcStartBuffer / RpcStopBuffer
```

### 初始化顺序（关键）

```
MainWindow::connectServer()
  → RPCClientInit
  → RegisterCallback(RPC_HEARTBEAT_ID)

第一次心跳到达:
  → discoverIo()
    → RpcGetServerInfo → ServerInfo
    → m_ioGroup->buildTabs(serverInfo)   // 创建所有 widget
    → registerCallbacks()                // 注册 observer 推送回调
    → m_ioGroup->setRpcClient(m_rpc)     // 存指针 + connectSignals()
```

**注意**：`setRpcClient` 在 `buildTabs` 之后调用，内部会调 `connectSignals()` 连接 widget 信号到 RPC。顺序反了按钮不会触发 RPC。

---

## 6. 已解决的待讨论项

1. **变量展示粒度**：AI/DI/AO/DO/Setting/Buffer **全部展示**（6 个 tab）。Cmd 暂不展示。
2. **变量名显示**：使用 config JSON 中的 name 字段。
3. **多机台支持**：当前为单 server 模式。
4. **Buffer/Cmd/Setting**：Buffer 和 Setting 均有独立 tab 和操作按钮。Cmd 暂不展示。
