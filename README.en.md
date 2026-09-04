<div align="center">

# Multi-Protocol Embedded Device Debug Platform

**An all-in-one debugging tool for custom embedded devices, replacing scattered tools like Serial Assistant, Modbus Poll, and NetAssist.**

[![Windows](https://img.shields.io/badge/Platform-Windows%20x64-blue?logo=windows&logoColor=white)](#)
[![C++](https://img.shields.io/badge/C++-11-orange?logo=c%2B%2B&logoColor=white)](#)
[![Qt](https://img.shields.io/badge/Qt-5.14.2%2B%20%2F%206.x-41CD52?logo=qt&logoColor=white)](#)
[![SQLite](https://img.shields.io/badge/SQLite-3.0-003B57?logo=sqlite&logoColor=white)](#)
[![Build](https://img.shields.io/badge/qmake-Release%20O2-green)](#)
[![Tests](https://img.shields.io/badge/Qt%20Test-258%20passed%20(100%25)-brightgreen)](#)
[![Coverage](https://img.shields.io/badge/Coverage-80.87%25%20Line-blue)](#)
[![License](https://img.shields.io/badge/License-MIT-yellow)](#)

[Download Release](#-download--quick-start) · [Features](#-feature-overview) · [Architecture](#-architecture) · [Build Guide](#-build-guide) · [Testing](#-testing--coverage) · [Highlights](#-design-highlights)

---

</div>

## Download & Quick Start

> **Zero-dependency offline package! Interviewer downloads zip -> extracts -> double-clicks DebugPlatform.exe to launch. No Qt/MinGW installation required.**

| Version | File | Size | Description |
|---------|------|------|-------------|
| v1.2.0 (latest) | DebugPlatform-v1.2.0-windows-x64.zip | ~15 MB (40 MB extracted) | Windows 7+ x64 / Includes all Qt runtime / Full MQTT 3.1.1 stack + device tree grouping + state sync |

**3-step quick start:**
1. Extract zip to any directory
2. Double-click DebugPlatform.exe
3. Menu "Device" -> "Add Device" -> select protocol -> start debugging

---

## Project Positioning

In industrial/embedded debugging, engineers often need **Serial Assistant + Modbus Poll + NetAssist + Excel logging** — at least 4 tools simultaneously, with data scattered and hard to analyze.

**This project = 1 desktop app = 4+ tools integrated**, with unified device management, data monitoring, history playback, and threshold alerting.

| Traditional Tool Combo | This Unified Platform |
|------------------------|----------------------|
| Serial Assistant (SSCOM) | Serial comm + hex send/parse |
| Modbus Poll | Modbus RTU/TCP stack + FC 0x03/0x06/0x10 |
| NetAssist | TCP/UDP comm + auto-reconnect + heartbeat |
| Excel/CSV logging | SQLite persistence + history query + playback |
| Manual monitoring + phone alerts | Threshold alert engine + beep/popup/alert log |

---

## Feature Overview

### Multi-Communication & Multi-Protocol Matrix (4x4)

| Comm \ Protocol | Modbus RTU | Modbus TCP | BinaryFrame | MQTT JSON |
|-----------------|:----------:|:----------:|:-----------:|:---------:|
| **Serial (RS-232/485)** | Yes | No | Yes | No |
| **TCP (Ethernet)** | No | Yes (MBAP) | Yes | No |
| **UDP (Datagram)** | No | No | Yes (custom port) | No |
| **MQTT (Broker)** | No | No | No | Yes (QoS0/1) |

### Core Features (12 Categories)

- **Multi-device management** — Serial/TCP/UDP devices in one tree, independent start/stop, auto-reconnect
- **Modbus RTU stack** — FC 0x03 read holding registers, 0x06 single write, 0x10 multi write, CRC16, exception response (0x80+code)
- **Modbus TCP stack** — MBAP header (TID/PID/Length/UnitID) + sticky-packet handling
- **Custom BinaryFrame parser** — 7-state byte-stream synchronous state machine, auto handles sticky/half/error frame resync
- **Real-time data monitor** — Register table grouped by device, real-time refresh, NaN/overflow highlighting
- **Command panel** — Hex input + auto protocol framing, send/response hex visualization
- **History query** — Multi-dimensional query by deviceId + address + time range, exportable TSV/CSV
- **Alert engine** — Register-level threshold rules (> / < / == / !=), alert logging + popup on trigger
- **Data playback engine** — Replay historical data at original or accelerated speed back to monitor panel
- **Unified logging** — Debug/Info/Warning/Error + module filtering (Protocol/Data/Device/UI), file save support
- **MQTT 3.1.1 stack** — Hand-written CONNECT/SUBSCRIBE/PUBLISH/PINGREQ/DISCONNECT codec, JSON payload parsing (single/array/raw value 3 formats)
- **Log packet parser view** — Auto-detect hex strings in logs, colorize by protocol (RTU/TCP/BinaryFrame) + field table (SlaveAddr/FC/CRC16), CRC check status

---

## Architecture

### 4-Layer Decoupled Structure

```
┌─────────────────────────────────────────────────────────────────────┐
│  UI Layer (Qt Widgets)    MainWindow / DeviceTree / DataMonitor     │
│          CommandPanelWidget · LogWidget · Config Dialogs            │
├─────────────────────────────────────────────────────────────────────┤
│  Data Layer                                                         │
│   ┌─────────────┐  ┌────────────┐  ┌──────────────┐                │
│   │ RingBuffer  │  │ DataCache  │  │ DataPersistent│               │
│   │ (thread-safe│  │ (real-time │  │ (SQLite WAL +│                │
│   │  ring buf)  │  │  hash map) │  │  composite idx)│              │
│   └─────────────┘  └────────────┘  └──────────────┘                │
│   ┌─────────────┐  ┌────────────┐                                   │
│   │ AlertManager│  │PlaybackEng │  (threshold alerts / playback)   │
│   └─────────────┘  └────────────┘                                   │
├─────────────────────────────────────────────────────────────────────┤
│  Device Layer    DeviceManager · DeviceConfig · ReconnectController │
│                                            exponential backoff 1s→32s│
├─────────────────────────────────────────────────────────────────────┤
│  Protocol Layer  CRC16 · ModbusRtuProtocol · ModbusTcpProtocol      │
│   · BinaryFrame 7-state parser · MqttProtocol · PacketParserHighlighter│
├─────────────────────────────────────────────────────────────────────┤
│  Core Layer      ICommunication (pure virtual interface)            │
│     CommunicationFactory (factory pattern) · Serial/TCP/UDP/MQTT    │
└─────────────────────────────────────────────────────────────────────┘
              ║         ║         ║         ║
          ┌───▼───┐ ┌───▼───┐ ┌───▼───┐ ┌───▼────┐
          │Serial │ │  TCP  │ │  UDP  │ │  MQTT  │
          └───────┘ └───────┘ └───────┘ └────────┘
```

### Design Principles

1. **Open-Closed Principle (OCP)**: New comm type = inherit ICommunication + register factory; new protocol = independent parser. No existing code modified.
2. **Centralized signal-slot routing**: All cross-module coupling in ServiceLocator singleton; UI has zero raw pointers to backend.
3. **Async event-driven**: QSerialPort/QTcpSocket/QUdpSocket all use native async APIs; UI never blocks on IO.
4. **RAII + parent object tree**: Qt objects use parent ownership; non-owning refs use QPointer to prevent dangling.
5. **SQLite performance trio**: WAL mode + (device_id, register_addr, timestamp) composite index + transaction batch writes.

---

## Build Guide

### Requirements

| Dependency | Version | Notes |
|------------|---------|-------|
| Qt | **5.14.2+** (Qt 6.x compatible) MinGW 64-bit | Code has Qt 5/6 conditional compilation; recommended 5.14.2 or 6.11.0 |
| MinGW | 7.3.0+ 64-bit (bundled with Qt) | mingw730_64 for Qt5, mingw1310_64 for Qt6 |
| qmake | Bundled with Qt | **Primary build system** (DebugPlatform.pro) |
| CMake | 3.16+ (optional) | Alternative build system, all sources synced |
| OS | Windows 7 x64 SP1+ | Recommended Win 10/11 x64 |

> **Qt 6 Compatibility**: Code uses `QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)` conditional compilation for: `QTextStream::setCodec` -> `setEncoding`, `QString::SkipEmptyParts` -> `Qt::SkipEmptyParts`, `QDateTime::setTimeSpec` -> constructor with timezone. Verified compiling on Qt 6.11.0 + MinGW 13.1.0.

### Build Steps (qmake)

```bat
REM 1. Open Qt command prompt (MinGW 64-bit) or set PATH
REM    Qt 5.14.2: set PATH=D:\Qt\5.14.2\mingw73_64\bin;D:\Qt\Tools\mingw730_64\bin;%PATH%
REM    Qt 6.11.0: set PATH=F:\Qt\6.11.0\mingw_64\bin;F:\Qt\Tools\mingw1310_64\bin;%PATH%

REM 2. Create build directory (out-of-source recommended)
mkdir build-DebugPlatform-Release
cd    build-DebugPlatform-Release

REM 3. qmake generates Makefile
qmake CONFIG+=release ..\DebugPlatform.pro

REM 4. Parallel compile (-j<cores>, typically -j8 or -j16)
mingw32-make -j8 release

REM 5. Output: build-DebugPlatform-Release\release\DebugPlatform.exe
```

### Build Steps (CMake, optional)

```bat
mkdir build-cmake
cd build-cmake
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="F:/Qt/6.11.0/mingw_64" ..
cmake --build . -j4
```

### Project Structure

```
DebugPlatform/
├── DebugPlatform.pro           # Main project (qmake)
├── CMakeLists.txt              # Alternative CMake build
├── README.md                   # Chinese README
├── README.en.md                # English README (this file)
├── LICENSE                     # MIT License
├── include/                    # Headers (by module)
│   ├── app/servicelocator.h        # Singleton dispatch center
│   ├── core/                       # Comm abstraction: interface + 4 impls + factory
│   ├── protocol/                   # Protocol: CRC16/ModbusRTU/ModbusTCP/BinaryFrame/MQTT
│   ├── data/                       # Data: ring buffer/cache/persistence/log/alert/playback
│   ├── device/                     # Device: config/manager/reconnect controller
│   └── ui/                         # UI: main window + 5 widgets + 4 dialogs
├── src/                        # Sources (mirrors include/)
├── resources/app.qrc           # Qt resources
├── tools/                      # Helper scripts (MQTT partner simulators)
├── tests/                      # Qt Test unit test subproject (258 cases)
└── docs/                       # Documentation
```

---

## Testing & Coverage

### Automated Unit Tests

```
Qt Test classes:  19
Test functions:   258
All passed:       258 (100%)
Failed:           0
```

| Test Module | Coverage |
|-------------|----------|
| TestCrc16 | CRC16 algorithm: empty/typical/large input |
| TestBinaryFrame | Build/parse/boundary/half-frame/CRC error/state machine |
| TestModbus | Modbus RTU request/response/exception; Modbus TCP MBAP |
| TestDataCache | Insert/query/overwrite/batch/clear |
| TestAlertManager | Rule CRUD + trigger signal + debounce |
| TestRingBuffer | Concurrent read/write + overflow strategy + boundaries |
| TestDeviceConfig | Serialize/deserialize/toString/integrity |
| TestDataPersistent | Init/device CRUD/insert-query/batch/alert/error handling |
| TestLogManager | Log append/filter/signal/save |
| TestPlaybackEngine | Load/play/pause/speed/stop |
| TestReconnectController | Exponential backoff/success reset/stop |
| TestCommunicationFactory | Factory creates Serial/TCP/UDP, invalid type returns null |
| TestICommunication | Interface contract/open-close idempotency/signals |
| TestTcpCommunication | TCP loopback connect/open-close/error signal/data TX/RX |
| TestUdpCommunication | UDP bind/open-close/datagram TX/RX |
| TestSerialPortCommunication | State management/config/invalid port/error path (no hardware needed) |
| TestMqttCommunication | MQTT packet codec (Remaining Length/strings/CONNECT/SUBSCRIBE/PUBLISH/PINGREQ) + parse |
| TestMqttProtocol | JSON payload parse (single/array/raw) + deviceId/address extraction |
| TestPacketParserHighlighter | Hex extraction/protocol detection/field split/coloring |

### gcov Code Coverage

| Category | Dimension | Executed | Total | Coverage |
|----------|-----------|----------|-------|----------|
| Overall | Lines | 1120 | 1385 | **80.87%** |

| Module | Files | Total Lines | Covered | Line Coverage |
|--------|-------|-------------|---------|---------------|
| device (Config+Reconnect) | 2 | 175 | 167 | **95.43%** |
| data (Cache+Alert+Log+RingBuf+Persistent+Playback) | 6 | 515 | 429 | **83.30%** |
| protocol (CRC16+BinaryFrame+Modbus RTU/TCP+MQTT) | 6 | 420 | 340 | **80.95%** |
| core (Factory+IComm+Serial+TCP+UDP+MQTT) | 7 | 275 | 195 | **70.91%** |

> Serial/TCP/UDP low-level implementation depends on hardware/real ports (~18% coverage). MQTT async paths covered via Mock tests.

---

## Design Highlights (Interview-Ready)

### 1. Layered Architecture & Factory Pattern (OCP in practice)
ICommunication pure virtual interface + CommunicationFactory registry factory. Adding MQTT/CAN/Bluetooth requires only:
```cpp
CommunicationFactory::registerCreator(CommunicationType::Can, &CanCommunication::create);
// No existing UI/data layer code modified
```

### 2. Hand-Written Protocol Stacks (4 types, no Qt Modbus/QMQTT dependency)
- **Modbus RTU**: Frame build/parse + CRC16 (Modbus polynomial 0xA001) + exception codes (bit7 set)
- **Modbus TCP**: Independent MBAP encapsulation (TransactionId/ProtocolId/Length/UnitId, 7 bytes), strict sticky-packet handling
- **BinaryFrame**: 7-state byte-stream synchronous state machine, byte-by-byte streaming parse
- **MQTT 3.1.1**: Hand-written CONNECT/SUBSCRIBE/PUBLISH/PINGREQ/DISCONNECT codec, Remaining Length variable encoding + JSON payload auto-adapt (3 formats)

### 3. Thread-Safe Ring Buffer (Classic Producer-Consumer)
Template RingBuffer<T> + std::mutex + dual-pointer ring array, default capacity 10000, overwrite-oldest on full. **Benchmarked 91.8M ops/sec single-producer-single-consumer (1M writes + 10K reads / 11ms)**.

### 4. Exponential Backoff Auto-Reconnect
delay = min(1000ms * 2^attempt, 32000ms), max 10 attempts, reset on success. QTimer single-shot async, **never blocks UI thread**.

### 5. SQLite Performance Trio (20.2x batch speedup: 416K vs 20.6K rows/sec)
- **WAL mode**: Set on handle open, allows concurrent read+write
- **Composite index** (device_id, register_addr, timestamp): 90% queries hit index tree
- **Transaction batch**: N single INSERTs merged into 1 transaction, disk IO plummets

### 6. Unified Signal-Slot Dispatch (ServiceLocator Singleton)
All cross-module signal-slot connections registered in one function:
```cpp
connect(manager,  &DeviceManager::deviceDataReceived, this, &ServiceLocator::onDeviceData);
connect(protocol, &ServiceLocator::dataPointParsed,    cache, &DataCache::upsert);
connect(protocol, &ServiceLocator::dataPointParsed,    db,    &DataPersistent::insertRegisterDataTask);
```
**Zero cross-module raw pointers, zero UI access to backend objects.**

### 7. Qt Async Signal-Slot + MVC Decoupling
All sockets use async event-driven APIs (readyRead/bytesWritten/connected/disconnected). **UI never blocks.** 100ms QTimer throttles display refresh to avoid high-frequency stutter.

### 8. Log Packet Parser View (Field-Level Visualization)
PacketParserHighlighter auto-detects hex strings in log messages, splits and colorizes by protocol (ModbusRtu/ModbusTcp/BinaryFrame). Each byte colored by field semantics (Header/SlaveAddr/FC/Data/CRC16/Tail), with aligned field table below (offset/hex/decimal meaning), CRC status marked OK/MISMATCH.

### 9. Complete Traceable Engineering Quality
- 19 classes x 258 unit tests (Qt Test, XML reports)
- gcov line coverage 80.87% (CSV + text reports)
- 8 typical bugs with "symptom -> root cause -> fix -> lesson" post-mortem docs
- Benchmark suite: RingBuffer 91.8M ops/sec, protocol parse 3.33M fps, SQLite batch 416K rows/sec
- Release offline package (v1.2.0, includes Qt runtime), interviewer double-click ready

---

## Roadmap

| Priority | Estimated Effort | Content |
|----------|-----------------|---------|
| P0 (2 days) | Done | Fix tech stack claims / Release package / README / 5 bug post-mortems / Benchmark suite |
| P1 (3-5 days) | Done | Multi-thread moveToThread / QPointer safety / robustness error-path tests / Log packet parser view |
| P2 (7+ days) | Done | MQTT 3.1.1 stack + device tree grouping + state sync + auto-reconnect + history query + CSV export |
| P3 (Future) | Done | ReconnectController test sync / CMake build verification / SerialPort unit tests / English README / Clang-Tidy |

---

## Resume Entry (Copy-Paste Ready)

```
[Multi-Protocol Embedded Device Debug Platform] | C++11 / Qt 5.14.2+ (qmake) / MinGW / SQLite 3
- All-in-one debug software for custom embedded devices, replacing Serial Assistant/Modbus Poll/NetAssist
- Designed 4-layer decoupled architecture (Comm Abstraction -> Protocol Parse -> Data Mgmt -> UI), factory pattern for pluggable comm types
- Independently wrote Modbus RTU/TCP + custom BinaryFrame + MQTT 3.1.1 protocol stacks:
  - Modbus RTU/TCP supports FC 0x03/0x06/0x10, exception code (bit7) parsing
  - BinaryFrame implements 7-state byte-stream state machine, auto handles sticky/half/error frame resync
  - MQTT 3.1.1 hand-written CONNECT/SUBSCRIBE/PUBLISH codec + JSON payload parsing
- Thread-safe ring buffer (std::mutex), benchmarked 91.8M ops/sec concurrent read/write
- Exponential backoff (1s->2s->4s->...->32s) auto-reconnect, UI completely non-blocking
- SQLite WAL + (device_id, addr, timestamp) composite index + transaction batch, benchmarked 416K rows/sec batch write (20.2x faster than single), range query avg 3.89ms
- Log packet parser view: auto-detect hex strings, protocol-based coloring + field table, CRC status marking
- Qt async signal-slot + MVC decoupling, CPU < 30% with 5 concurrent devices
- Engineering quality: 19 classes / 258 Qt Test cases 100% pass, gcov line coverage 80.87%
```

---

## License

MIT License — free to use, modify, distribute.

---

<div align="center">

**If this project helped your job hunt, a Star is the greatest encouragement!**

[![Made with Qt & C++](https://img.shields.io/badge/Made%20with-Qt%20%26%20C%2B%2B-red)](#)

</div>
