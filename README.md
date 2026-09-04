<div align="center">

# 🔧 多协议嵌入式设备调试平台

**面向自研嵌入式设备的一体化调试软件，替代串口助手、Modbus Poll、NetAssist 等零散工具。**

[![Windows](https://img.shields.io/badge/Platform-Windows%20x64-blue?logo=windows&logoColor=white)](#)
[![C++](https://img.shields.io/badge/C++-11-orange?logo=c%2B%2B&logoColor=white)](#)
[![Qt](https://img.shields.io/badge/Qt-5.14.2%2B%20%2F%206.x-41CD52?logo=qt&logoColor=white)](#)
[![SQLite](https://img.shields.io/badge/SQLite-3.0-003B57?logo=sqlite&logoColor=white)](#)
[![Build](https://img.shields.io/badge/qmake-Release%20O2-green)](#)
[![Tests](https://img.shields.io/badge/Qt%20Test-258%20passed%20(100%25)-brightgreen)](#)
[![Coverage](https://img.shields.io/badge/Coverage-80.70%25%20Line-blue)](#)
[![License](https://img.shields.io/badge/License-MIT-yellow)](#)

[🚀 下载 Release](#-下载--快速体验) · [📸 功能速览](#-功能速览) · [🏗️ 技术架构](#-技术架构) · [📦 构建指南](#-构建指南) · [🧪 测试与覆盖率](#-测试与覆盖率) · [✨ 设计亮点](#-设计亮点)

---

</div>

## 🚀 下载 & 快速体验

> **零依赖离线包！面试官下载 zip → 解压 → 双击 DebugPlatform.exe 即可启动，无需安装 Qt/MinGW 环境。**

| 版本 | 文件 | 大小 | 说明 |
|------|------|------|------|
| v1.2.0 (最新) | [DebugPlatform-v1.2.0-windows-x64.zip](./DebugPlatform-v1.2.0-windows-x64.zip) | ~15 MB (解压 40 MB) | Windows 7+ x64 / 已含全部 Qt 运行时 / 完整 MQTT 3.1.1 协议栈 + 设备树分组 + 状态同步 |

👉 **Release 包快速 3 步上手：**
`
1. 解压 zip 到任意目录
2. 双击 DebugPlatform.exe 启动
3. 菜单「设备」→「添加设备」→ 选协议 → 开始调试 🎉
`

（详细操作流程见「🛠️ 使用示例」章节）

---

## 🎯 项目定位

工业/嵌入式调试场景下，工程师经常需要同时打开 **串口助手 + Modbus Poll + NetAssist + Excel 记录** 至少 4 个工具，数据来回切换、分散不易分析。

**本项目目标 = 1 个桌面应用 = 4+ 工具的能力整合**，统一的设备管理、数据监控、历史回放、阈值告警界面。

| 传统调试工具组合 | 本项目统一平台 |
|------------------|----------------|
| 串口助手 (SSCOM) | ✅ 串口通信 + 十六进制发送/解析 |
| Modbus Poll | ✅ Modbus RTU/TCP 协议栈 + 功能码 0x03/0x06/0x10 |
| NetAssist 网络助手 | ✅ TCP/UDP 通信 + 自动重连 + 心跳 |
| Excel/CSV 数据记录 | ✅ SQLite 持久化 + 历史查询 + 数据回放 |
| 人工盯数值 + 告警电话 | ✅ 阈值告警引擎 + 蜂鸣/弹窗/告警日志 |

---

## ✨ 功能速览

### 📡 多通信 & 多协议支持（4×4 矩阵）
| 通信方式 ↓ \ 协议 → | Modbus RTU | Modbus TCP | BinaryFrame | MQTT JSON |
|----------------------|:----------:|:----------:|:------------:|:---------:|
| **串口 (RS-232/485)** | ✅         | ❌         | ✅           | ❌        |
| **TCP (Ethernet)**    | ❌         | ✅ MBAP 封装 | ✅           | ❌        |
| **UDP (Datagram)**    | ❌         | ❌         | ✅ 可自定义端口 | ❌      |
| **MQTT (Broker)**     | ❌         | ❌         | ❌           | ✅ QoS0/1  |

### 🏭 核心功能（12 大类）
- [x] **多设备统一管理** - 串口/TCP/UDP 设备同一棵树管理，独立启停，断线重连
- [x] **Modbus RTU 协议栈** - 功能码 x03 读保持寄存器、x06 单写、x10 多写，CRC16 校验，异常响应（0x80+码）解析
- [x] **Modbus TCP 协议栈** - MBAP 报文头（TID/PID/Length/UnitID）封装 + 粘包处理
- [x] **自定义 BinaryFrame 解析** - 7 状态字节流同步状态机，自动处理粘包/半包/错误帧重同步
- [x] **实时数据监控** - 按设备分组的寄存器表格，实时刷新，支持 NaN/溢出标红
- [x] **命令面板** - Hex 输入 + 协议自动封装，发送/响应 Hex 可视化对比
- [x] **历史数据查询** - 按 deviceId + 地址 + 时间范围 多维查询，结果可导出 TSV/CSV
- [x] **告警引擎** - 寄存器级阈值规则（> / < / == / !=），触发后记录告警 + 弹窗
- [x] **数据回放引擎** - 历史上某个时间段的数据按原始速率/倍速重新"播放"，回到监控面板
- [x] **统一日志系统** - Debug/Info/Warning/Error + 模块过滤（Protocol/Data/Device/UI 等），支持保存到文件
- [x] **MQTT 3.1.1 协议栈** - 手写 CONNECT/SUBSCRIBE/PUBLISH/PINGREQ/DISCONNECT 报文编解码，JSON 载荷解析（单点/数组/纯数值 3 格式）
- [x] **日志报文解析视图** - 自动探测日志中的 hex 串，按协议（RTU/TCP/BinaryFrame）着色字节区 + 字段表（SlaveAddr/FC/CRC16 等），CRC 校验状态标记

### 🛠️ 使用示例

#### 例 1：Modbus TCP 读取保持寄存器（配合 Modbus Slave 工具）
`
① 启动 Modbus Slave → Connection → TCP/IP (Port 502) → 定义 Slave ID = 1，10 个寄存器 4x0001~4x0010
② 启动 DebugPlatform.exe → 菜单「设备」→「添加设备」
   → 通信方式: TCP；目标地址: 127.0.0.1:502；协议类型: ModbusTcp；名称: modbus_tcp_demo
③ 左侧设备树选中 modbus_tcp_demo → 点击「连接」
④ 右侧命令面板: 功能码 读保持寄存器(0x03)；起始地址: 0；数量: 10 → 点击发送
⑤ 观察中间数据监控面板: 地址 0~9 的 10 个寄存器值实时显示 ✅
`

#### 例 2：BinaryFrame 二进制帧（串口/UDP）
`
协议格式: [0xAA][0x55][Length LO][Length HI][Payload N B][CRC16 LO][CRC16 HI][0x0D][0x0A]
默认解析: Payload 前 2B = 寄存器地址 (uint16 大端)，后 4B = 浮点值 (IEEE754 大端)
例：发送 "00 02 42 F6 E9 79" → 系统自动封装 Header + Length + CRC + Tail
   → 解析后寄存器 2 = 123.456，监控面板立即更新
`

---

## 🏗️ 技术架构

### 分层结构图（4 层解耦）

`
┌─────────────────────────────────────────────────────────────────────┐
│  UI 层 (Qt Widgets)              MainWindow / DeviceTree / DataMonitor│
│          命令面板 CommandPanelWidget · 日志窗口 LogWidget           │
│          添加/历史/告警配置对话框                                    │
├─────────────────────────────────────────────────────────────────────┤
│  数据管理层 (Data Layer)                                            │
│   ┌─────────────┐  ┌────────────┐  ┌──────────────┐                │
│   │ RingBuffer  │  │ DataCache  │  │ DataPersistent │               │
│   │ (线程安全   │  │ (实时寄存器 │  │ (SQLite WAL + │                │
│   │  环形缓冲)  │  │  哈希表)    │  │  复合索引)    │                │
│   └─────────────┘  └────────────┘  └──────────────┘                │
│   ┌─────────────┐  ┌────────────┐                                   │
│   │ AlertManager│  │PlaybackEng │  (阈值告警引擎 / 历史数据回放)    │
│   └─────────────┘  └────────────┘                                   │
├─────────────────────────────────────────────────────────────────────┤
│  设备管理层 (Device Layer)  DeviceManager · DeviceConfig · Reconnect│
│                                            指数退避自动重连 (1s→32s) │
├─────────────────────────────────────────────────────────────────────┤
│  协议解析层 (Protocol Layer)  CRC16 · ModbusRtuProtocol (RTU 帧)    │
│   · ModbusTcpProtocol (MBAP+PDU) · BinaryFrame 7 状态同步状态机解析器│
│   · MqttProtocol (MQTT 3.1.1 JSON 载荷解析) · PacketParserHighlighter │
├─────────────────────────────────────────────────────────────────────┤
│  通信抽象层 (Core Layer)     ICommunication (纯虚接口)               │
│     CommunicationFactory (工厂模式)  SerialPort / TCP / UDP / MQTT 四实现 │
└─────────────────────────────────────────────────────────────────────┘
              ║         ║         ║         ║
          ┌───▼───┐ ┌───▼───┐ ┌───▼───┐ ┌───▼────┐
          │ 串口   │ │  TCP  │ │  UDP  │ │  MQTT  │
          └───────┘ └───────┘ └───────┘ └────────┘
`

### 设计原则
1. **开闭原则 (OCP)**：新通信方式只需继承 ICommunication + 注册工厂；新协议写独立解析器 → 不修改任何已有代码
2. **信号槽集中转发**：跨模块耦合统一在 ServiceLocator（单例）完成，UI 与业务零裸指针引用
3. **异步事件驱动**：QSerialPort/QTcpSocket/QUdpSocket 全部使用原生异步 API，UI 不会因 IO 阻塞卡顿
4. **RAII + 父对象树**：Qt 对象使用 parent 所有权；非所有权引用用 QPointer 防止悬空指针
5. **SQLite 三件套**：WAL 模式 + (device_id, register_addr, timestamp) 复合索引 + 事务批量写入，性能对标轻量级时序数据库

---

## 📦 构建指南

### 环境要求
| 依赖 | 版本要求 | 备注 |
|------|----------|------|
| Qt | **5.14.2+**（兼容 Qt 6.x）MinGW 64-bit | 代码已做 Qt 5/6 条件编译兼容；推荐 5.14.2 或 6.11.0 |
| MinGW | 7.3.0+ 64-bit (Qt 自带) | Qt 5 用 mingw730_64，Qt 6 用 mingw1310_64 |
| qmake | 随 Qt 提供 | **主构建系统**（DebugPlatform.pro） |
| CMake | 3.16+（可选） | 备用构建系统，已同步所有源文件 |
| 操作系统 | Windows 7 x64 SP1 及以上 | 推荐 Win 10/11 x64 |

> **Qt 6 兼容性说明**：代码中使用 `QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)` 条件编译处理了以下 API 变更：`QTextStream::setCodec` → `setEncoding`、`QString::SkipEmptyParts` → `Qt::SkipEmptyParts`、`QDateTime::setTimeSpec` → 构造函数指定时区。已在 Qt 6.11.0 + MinGW 13.1.0 下验证编译通过。

### 构建步骤
```bat
REM 1. 打开 Qt 命令行 (MinGW 64-bit) 或设置环境
REM    Qt 5.14.2: set PATH=D:\Qt\5.14.2\mingw73_64\bin;D:\Qt\Tools\mingw730_64\bin;%PATH%
REM    Qt 6.11.0: set PATH=F:\Qt\6.11.0\mingw_64\bin;F:\Qt\Tools\mingw1310_64\bin;%PATH%

REM 2. 新建 build 目录（推荐 out-of-source 构建）
mkdir build-DebugPlatform-Release
cd    build-DebugPlatform-Release

REM 3. qmake 生成 Makefile
qmake CONFIG+=release ..\DebugPlatform.pro

REM 4. 并行编译（建议 -j<核数>，通常 -j8 或 -j16）
mingw32-make -j8 release

REM 5. 产物位于 build-DebugPlatform-Release\release\DebugPlatform.exe
```

### 代码结构
`
DebugPlatform/
├── DebugPlatform.pro           # 主工程 (qmake)
├── CMakeLists.txt              # 备用 CMake 构建
├── README.md                   # 本文件
├── LICENSE                     # MIT License
├── include/                    # 头文件（按模块分）
│   ├── app/servicelocator.h        # 单例调度中心
│   ├── core/                       # 通信抽象层：接口+4实现类(Serial/TCP/UDP/MQTT)+工厂
│   ├── protocol/                   # 协议解析层：CRC16/ModbusRTU/ModbusTCP/BinaryFrame/MQTT
│   ├── data/                       # 数据管理层：环形缓冲/实时缓存/持久化/日志/告警/回放
│   ├── device/                     # 设备管理层：设备配置/管理器/重连控制器
│   └── ui/                         # UI 层：主窗口 + 5 大组件 + 4 对话框
├── src/                        # 源文件（对应 include 结构）
├── resources/app.qrc           # Qt 资源文件
├── tools/                      # 辅助工具
│   ├── mqtt_partner.py             # MQTT 对端模拟脚本（发数据用）
│   ├── mqtt_partner_v2.py          # 数据格式 v2
│   ├── mqtt_partner_v3.py          # 高频压力测试版
│   └── mqtt_stress.py              # 1000 条消息压力测试
├── tests/                      # Qt Test 单元测试子项目
│   ├── DebugPlatformTests.pro      # 测试子项目构建
│   ├── test_main.cpp               # 测试入口（19 类全部 qExec）
│   ├── include/                    # 测试头文件（test_xxx.h）
│   └── src/                        # 测试实现（test_xxx.cpp，共 258 个用例）
└── docs/                       # 文档
    ├── README.md                   # 项目主页（本文件镜像）
    ├── resume_highlights.md        # 简历亮点 & 面试高频问答
    ├── 求职项目优化建议.md         # P0/P1/P2 优化路线图 & 5 分钟面试话术
    └── 工程复盘与踩坑记录.md       # 典型 Bug 复盘
`

---

## 🧪 测试与覆盖率

### 自动化单元测试
`
Qt Test 测试类: 19 个
测试函数数:   258 项
全部通过:     258 项 ✅ (100%)
失败数:       0
`

| 测试模块 | 覆盖内容 | 用例数 |
|---------|---------|--------|
| TestCrc16 | CRC16 算法空输入/典型输入/大输入 | ✅ |
| TestBinaryFrame | 构建/解析/边界帧/半帧/错误帧 CRC/状态机 | ✅ |
| TestModbus | Modbus RTU 请求/响应/异常；Modbus TCP MBAP 报文头 | ✅ |
| TestDataCache | 插入/查询/覆盖/批量/清空 | ✅ |
| TestAlertManager | 规则增删改查 + 触发信号 + 连续触发防抖 | ✅ |
| TestRingBuffer | 并发读写 + 满覆盖策略 + 边界 case | ✅ |
| TestDeviceConfig | 序列化/反序列化/toString/完整性 | ✅ |
| TestDataPersistent | 初始化/设备 CRUD/插入查询/批量/告警/错误处理 | ✅ |
| TestLogManager | 日志追加/过滤/信号/保存 | ✅ |
| TestPlaybackEngine | 加载数据包/播放/暂停/倍速/停止 | ✅ |
| TestReconnectController | 指数退避算法/成功重置/停止 | ✅ |
| TestCommunicationFactory | 工厂创建 Serial/TCP/UDP，无效类型返回空 | ✅ |
| TestICommunication | 接口契约/打开关闭幂等/信号发射 | ✅ |
| TestTcpCommunication | TCP 回环连接/打开关闭/错误信号/数据收发 | ✅ |
| TestUdpCommunication | UDP 绑定/打开关闭/数据报收发 | ✅ |
| TestSerialPortCommunication | 串口状态管理/配置验证/无效端口/错误路径（无需硬件） | ✅ |
| TestMqttCommunication | MQTT 报文编解码（Remaining Length/字符串/CONNECT/SUBSCRIBE/PUBLISH/PINGREQ）+ 解析 | ✅ |
| TestMqttProtocol | JSON 载荷解析（单点/数组/纯数值）+ deviceId/address 提取 | ✅ |
| TestPacketParserHighlighter | hex 串提取/协议探测/字段切分/着色渲染 | ✅ |

### gcov 代码覆盖率（Qt Test 跑完统计）
| 类别 | 覆盖维度 | 已执行 | 总数 | 覆盖率 |
|------|---------|--------|------|--------|
| 总体 | 代码行 (Lines) | 1120 | 1385 | **80.87 %** ✅ |


| 模块 | 文件数 | 总行数 | 命中行数 | 行覆盖率 |
|------|--------|--------|----------|---------|
| device (Config+Reconnect) | 2 | 175 | 167 | **95.43 %** |
| data (Cache+Alert+Log+RingBuf+Persistent+Playback) | 6 | 515 | 429 | **83.30 %** |
| protocol (CRC16+BinaryFrame+Modbus RTU/TCP+MQTT) | 6 | 420 | 340 | **80.95 %** |
| core (Factory+IComm+Serial+TCP+UDP+MQTT) | 6 | 275 | 195 | **70.91 %** |

> ⚠️ 串口/TCP/UDP 底层实现因依赖硬件/真实端口，覆盖率较低（约 18%）。MQTT 通信因依赖 Broker 连接，部分异步路径通过 Mock 测试覆盖。

👉 **详细覆盖率 CSV**：tests/coverage_report/coverage_data.csv  
👉 **综合文本报告**：tests/coverage_report/coverage_report.txt

---

## 🌟 设计亮点（面试可直接讲）

### 1. 分层架构 & 工厂模式（开闭原则落地）
通信抽象 ICommunication 纯虚接口 + CommunicationFactory 注册式工厂。新增 MQTT / CAN / 蓝牙 只需要：
`cpp
// 新类继承 ICommunication → 实现接口 → 工厂一行注册
CommunicationFactory::registerCreator(CommunicationType::Can, &CanCommunication::create);
// ✅ 不修改任何已有 UI / 数据层代码
`

### 2. 手写完整协议栈（4 种，不依赖 Qt Modbus / QMQTT）
- **Modbus RTU**：帧构建/解析 + CRC16（Modbus 多项式 0xA001）+ 异常码（bit7 置 1）
- **Modbus TCP**：独立封装 MBAP（TransactionId/ProtocolId/Length/UnitId，7 字节），粘包处理严格按 size == 6 + MBAP.Length 出帧
- **BinaryFrame**：7 状态字节流同步状态机 [WAIT_H1 → WAIT_H2 → LEN_LO → LEN_HI → PAYLOAD → CRC_LO → CRC_HI → TAIL_1 → TAIL_2]（8 状态），实现"字节一个一个喂"的流式解析
- **MQTT 3.1.1**：手写 CONNECT/SUBSCRIBE/PUBLISH/PINGREQ/DISCONNECT 报文编解码，Remaining Length 变长编码 + JSON 载荷解析（单点 `{addr,value}` / 数组 `{data:[...]}` / 纯数值 3 格式自动适配）

### 3. 线程安全环形缓冲区（生产者-消费者经典题）
模板类 RingBuffer<T> + std::mutex + 双指针环形数组，默认容量 10000 点，满时采用"覆盖最老数据"策略。实测 **单生产-单消费者吞吐量 91.8M ops/sec（1M 写 + 10K 读 / 11ms）**。

### 4. 指数退避自动重连
delay = min(1000ms × 2^attempt, 32000ms)，默认最多尝试 10 次后停止，连接成功立即重置 attempt=0 delay=1000ms。配合 QTimer 单射异步，**完全不阻塞 UI 线程**。

### 5. SQLite 性能三件套（批量写入提速 20.2×，实测 416K rows/sec vs 单条 20.6K rows/sec）
- **WAL 模式**（SQLite 句柄一打开就设置）：允许并发读 + 写
- **复合索引** (device_id, register_addr, timestamp)：90% 查询直接命中索引树
- **事务批量**：DataPersistent::insertRegisterDataBatch 把 N 次单条 INSERT 合并成 1 次事务，磁盘 IO 骤降

### 6. 统一信号槽调度（ServiceLocator 单例）
所有跨模块信号槽在 ServiceLocator::connectSignals() 一个函数里完成集中注册：
`cpp
connect(manager,   &DeviceManager::deviceDataReceived,       this, &ServiceLocator::onDeviceData);   // 设备原始字节 → 协议解析
connect(protocol,  &ServiceLocator::dataPointParsed,         cache, &DataCache::upsert);              // 解析后 → 实时缓存
connect(protocol,  &ServiceLocator::dataPointParsed,         db,    &DataPersistent::insertRegisterDataTask);  // 同时写 SQLite
connect(alertMgr,  &AlertManager::alertTriggered,            this, [=]{ LogManager::instance()->warning(...);});
`
**零跨模块裸指针，零 UI 层访问后端对象**。

### 7. Qt 异步信号槽 + MVC 解耦
QSerialPort / QTcpSocket / QUdpSocket 全部使用异步事件驱动 API（readyRead / bytesWritten / connected / disconnected 信号），**UI 永不阻塞**。刷新节流用 100ms QTimer 合并显示避免高频卡顿。

### 8. 日志报文解析视图（字段级可视化）
PacketParserHighlighter 工具类自动探测日志消息中的 hex 串，按协议（ModbusRtu/ModbusTcp/BinaryFrame）切分字段并着色：每个字节按字段语义（Header/SlaveAddr/FC/Data/CRC16/Tail）染不同背景色，下方附对齐的字段表（偏移/Hex/十进制含义），CRC 校验状态用 OK/MISMATCH 标记。日志面板实时追加解析块，无需切工具。

### 9. 完整可追溯的工程质量
- 19 类 × 258 项单元测试（Qt Test，XML 报告）
- gcov 行覆盖率 80.87%（CSV + 文本报告）
- 5 个典型 Bug 的"现象→根因→修复→教训"复盘文档（[工程复盘与踩坑记录.md](./docs/工程复盘与踩坑记录.md)）
- Benchmark 套件实测：RingBuffer 91.8M ops/sec、协议解析 3.33M fps、SQLite 批量 416K rows/sec（[结果](./docs/benchmark_results.txt)）
- Release 离线包（v1.2.0，含 Qt 运行时），面试官双击可用

---

## 🛤️ 后续优化路线图

本项目仍在迭代，详细任务请见 [docs/求职项目优化建议.md](./docs/求职项目优化建议.md)，共三级清单：

| 优先级 | 预估工时 | 主要内容 |
|--------|---------|---------|
| 🔴 P0 必做（2 天） | 已完成 ✅ | 修技术栈谎言 / 打 Release 包 / 写 README / 5 坑复盘文档 / Benchmark 套件（[实测结果](./docs/benchmark_results.txt)） |
| 🟠 P1 推荐（3-5 天） | 全部完成 ✅ | 多线程 moveToThread / QPointer 指针安全 / 健壮性错误路径单测 / 日志报文解析视图（PacketParserHighlighter，字段着色+字段表）|
| 🟢 P2 加分（7 天 +） | 全部完成 ✅ | MQTT 3.1.1 协议栈 + 设备树分组 + 状态同步 + 断线重连 + 历史查询 + CSV 导出 |
| 🔵 P3 展望 | 已完成 ✅ | ReconnectController 测试同步 / CMake 构建验证 / SerialPort 单元测试 / 英文 README / Clang-Tidy 静态分析 |

---

## 📋 简历项目写法（直接复制粘贴）

`
【多协议嵌入式设备调试平台】 | C++11 / Qt 5.14.2（qmake）/ MinGW 7.3 / SQLite 3
- 面向自研嵌入式设备的一体化调试软件，替代串口助手/Modbus Poll/NetAssist 等零散工具
- 设计 4 层解耦架构（通信抽象→协议解析→数据管理→UI），工厂模式实现通信类型可插拔扩展
- 独立手写 Modbus RTU/TCP + 自定义 BinaryFrame + MQTT 3.1.1 协议栈：
  · Modbus RTU/TCP 支持功能码 0x03/0x06/0x10、异常码（bit7 置位）解析
  · BinaryFrame 实现 7 状态字节流状态机，自动处理粘包/半包/错误帧重同步
  · MQTT 3.1.1 手写 CONNECT/SUBSCRIBE/PUBLISH 报文编解码 + JSON 载荷解析
- 线程安全环形缓冲区（std::mutex），实测 91.8M ops/sec 并发读写（1M 写 + 10K 读 / 11ms）
- 指数退避算法（1s→2s→4s→8s→…→32s）实现断线自动重连，UI 完全无感
- SQLite WAL 模式 + (device_id, addr, timestamp) 复合索引 + 事务批量，实测批量写入 416K rows/sec（比单条快 20.2×），范围查询 avg 3.89ms
- 日志报文解析视图：自动探测 hex 串按协议着色+字段表，CRC 校验状态标记
- Qt 异步信号槽 + MVC 解耦，5 设备并行时 CPU 占用 < 30%
- 工程质量：19 类 / 258 项 Qt Test 单元测试 100% 通过，gcov 行覆盖率 80.87%
`

---

## 📝 License

MIT License — 自由使用、修改、分发。  
⚠️ **在秋招简历中参考使用时，请根据自己实际修改内容做适配，不要 1:1 原封不动抄（容易查重）**。

---

<div align="center">

**如果这个项目帮到你的秋招，点一个 ⭐ Star 就是对作者最大的鼓励！**

[![Made with ❤️ by Qt & C++](https://img.shields.io/badge/Made%20with-%E2%9D%A4%20Qt%20%26%20C%2B%2B-red)](#)

</div>