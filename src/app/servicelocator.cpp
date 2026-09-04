#include "app/servicelocator.h"
#include "data/datacache.h"
#include "data/datapersistent.h"
#include "data/alertmanager.h"
#include "data/logmanager.h"
#include "device/devicemanager.h"
#include "data/playbackengine.h"
#include "protocol/binaryframeparser.h"
#include "protocol/modbusrtuprotocol.h"
#include "protocol/modbustcpprotocol.h"
#include "protocol/mqttprotocol.h"
#include "device/deviceconfig.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QSqlError>
#include <QSqlQuery>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <cstring>

ServiceLocator::~ServiceLocator()
{
    shutdown();
}

void ServiceLocator::initialize(QObject *parent)
{
    if (m_initialized) {
        qWarning() << "ServiceLocator::initialize() - already initialized";
        return;
    }

    qInfo() << "ServiceLocator::initialize() - creating services...";

    // 创建日志管理器（单例，最先初始化以便后续服务可以使用日志）
    m_logManager = &LogManager::instance();
    Q_UNUSED(parent) // LogManager 是单例，不需要 parent

    m_logManager->log(LogLevel::Info, LogCategory::App,
                      "Initializing services...");

    // 创建数据缓存
    m_dataCache = new DataCache(this);
    m_logManager->log(LogLevel::Info, LogCategory::App,
                      "DataCache created");

    // 创建数据持久化
    m_dataPersistent = new DataPersistent(this);
    m_logManager->log(LogLevel::Info, LogCategory::App,
                      "DataPersistent created");

    // 创建告警管理器
    m_alertManager = new AlertManager(this);
    m_logManager->log(LogLevel::Info, LogCategory::App,
                      "AlertManager created");

    // 创建设备管理器
    m_deviceManager = &DeviceManager::instance();
    m_logManager->log(LogLevel::Info, LogCategory::App,
                      "DeviceManager created");

    // 创建回放引擎
    m_playbackEngine = new PlaybackEngine(this);
    m_logManager->log(LogLevel::Info, LogCategory::App,
                      "PlaybackEngine created");

    // ========================================================================
    // 连接服务间的信号槽
    // ========================================================================

    // ------------------------------------------------------------------------
    // 0. 设备增加/删除时，维护 BinaryFrameParser 实例（每设备 1 个）
    // ------------------------------------------------------------------------
    // 解析器创建：对每一个已存在的设备（将来 addDevice 也要处理）初始化 parser
    // 注意：DeviceManager 没有 addDevice 信号，UI->DeviceManager::addDevice 直接走
    // 所以这里简单做法：在 deviceDataReceived 第一次到来时惰性创建 parser

    // 1. 设备数据 → 协议解析器（按设备协议类型分派）→ DataCache
    connect(m_deviceManager, &DeviceManager::deviceDataReceived,
            this, [this](const QString &deviceId, const QByteArray &data) {
            // 基本校验：设备/数据空就直接丢弃，避免后续分支崩溃
            if (deviceId.isEmpty() || data.isEmpty()) return;
            // Helper: resolve deviceId to display name for log messages
            auto dn = [&]() -> QString {
                auto c = DeviceManager::instance().getDeviceConfig(deviceId);
                return (!c.name.isEmpty() && c.name != c.id) ? c.name : deviceId;
            }();

            // [DEBUG] 添加调试日志
            if (m_logManager) {
                m_logManager->log(LogLevel::Debug, LogCategory::Protocol,
                    QString("[DEBUG] 收到设备数据: deviceId=%1, size=%2, data=%3")
                        .arg(dn)
                        .arg(data.size())
                        .arg(QString::fromLatin1(data.toHex(' ').toUpper())));
            }
            // ---- 响应时间测量（Task 13）----
            QString  txHex;
            int      responseTimeMs = -1;
            bool     hasPending = false;
            if (m_pendingSends.contains(deviceId)) {
                const PendingSend ps = m_pendingSends.take(deviceId);
                const qint64 nowTs = QDateTime::currentMSecsSinceEpoch();
                responseTimeMs = static_cast<int>(nowTs - ps.sendTsMs);
                if (responseTimeMs < 0) responseTimeMs = 0;
                txHex = QString::fromLatin1(ps.txFrame.toHex(' ').toUpper());
                hasPending = true;
            }

            // 总是广播原始数据给 UI（用于原始 hex/日志监控）
            emit deviceRawDataReceived(deviceId, data);

            if (!m_dataCache) return;

            const DeviceConfig cfg = m_deviceManager
                                        ? m_deviceManager->getDeviceConfig(deviceId)
                                        : DeviceConfig();
            // 找不到配置时不做协议解析（避免把"未知设备"的数据送进解析器乱崩）
            if (cfg.id.isEmpty()) return;

            if (cfg.protocolType == ProtocolType::ModbusRtu) {
                // ---- Modbus RTU 响应解析 ----
                ModbusResponse resp = ModbusRtuProtocol::parseResponse(data);
                if (resp.isException) {
                    if (m_logManager) {
                        m_logManager->log(LogLevel::Warning, LogCategory::Protocol,
                            QString("Modbus RTU 异常响应 [设备=%1, 功能码=0x%2, 异常码=0x%3]")
                                .arg(dn)
                                .arg(static_cast<int>(resp.functionCode), 2, 16, QLatin1Char('0'))
                                .arg(static_cast<int>(resp.exceptionCode), 2, 16, QLatin1Char('0')));
                    }
                } else {
                    // 根据功能码决定是否更新数据缓存
                    switch (resp.functionCode) {
                    case ModbusFunctionCode::ReadCoils:
                    case ModbusFunctionCode::ReadDiscreteInputs:
                    case ModbusFunctionCode::ReadHoldingRegisters:
                    case ModbusFunctionCode::ReadInputRegisters: {
                        // 读操作：响应包含实际数据，更新缓存
                        for (int i = 0; i < resp.data.size(); ++i) {
                            const quint16 addr = static_cast<quint16>(i);
                            const double value = static_cast<double>(resp.data[i]);
                            m_dataCache->updateValue(deviceId, addr, value);
                            emit deviceRegisterUpdated(deviceId, addr, value, QString(), QString());
                        }
                        break;
                    }
                    case ModbusFunctionCode::WriteSingleRegister: {
                        // 写单寄存器：响应回显 [地址, 值]，值是实际写入的值
                        if (resp.data.size() >= 2) {
                            const quint16 addr = resp.data[0];
                            const double value = static_cast<double>(resp.data[1]);
                            m_dataCache->updateValue(deviceId, addr, value);
                            emit deviceRegisterUpdated(deviceId, addr, value, QString(), QString());
                        }
                        break;
                    }
                    case ModbusFunctionCode::WriteMultipleRegisters:
                    case ModbusFunctionCode::WriteSingleCoil:
                    case ModbusFunctionCode::WriteMultipleCoils: {
                        // 写多寄存器/线圈：响应只回显 [起始地址, 数量]，不包含实际值
                        break;
                    }
                    default:
                        break;
                    }
                }

                // 回显响应到命令面板
                emit modbusResponseReceived(deviceId,
                    hasPending ? txHex : QString(),   // 配对到发送时用 TX 历史
                    QString::fromLatin1(data.toHex(' ').toUpper()),
                    responseTimeMs);

            } else if (cfg.protocolType == ProtocolType::BinaryFrame) {
                // ---- 自定义二进制帧：7 状态机（粘包/半包隔离）----
                if (!m_binaryParsers.contains(deviceId)) {
                    auto *parser = new BinaryFrameParser(this);
                    connect(parser, &BinaryFrameParser::frameReceived,
                            this, [this, deviceId](const QByteArray &payload) {
                                // 约定 payload 格式：[地址 2B][值 4B double 或 2B uint16]
                                // 简单约定：长度 >=6 时按 [addr:2][value:4 IEEE754 float]
                                // 否则每 4B 作为一个 (addr, uint16) 对
                        if (payload.size() >= 4) {
                            const quint16 addr =
                                (static_cast<quint8>(payload[0]) << 8) |
                                static_cast<quint8>(payload[1]);

                            if (payload.size() >= 6) {
                                // 6 字节或以上：float value (地址2B + 浮点值4B)
                                // 注意：浮点值按大端存储 (p[0]是MSB, p[3]是LSB)
                                float fval = 0.0f;
                                const uchar *p = reinterpret_cast<const uchar*>(payload.constData() + 2);
                                quint32 raw =
                                    (quint32(p[0]) << 24) |
                                    (quint32(p[1]) << 16) |
                                    (quint32(p[2]) <<  8) |
                                    (quint32(p[3])      );
                                memcpy(&fval, &raw, sizeof(fval));
                                m_dataCache->updateValue(deviceId, addr, static_cast<double>(fval));
                                emit deviceRegisterUpdated(deviceId, addr, fval, QString(), QString());
                            } else if (payload.size() == 4) {
                                // 4 字节：uint16 value (地址2B + 整数值2B)
                                const quint16 val =
                                    (static_cast<quint8>(payload[2]) << 8) |
                                    static_cast<quint8>(payload[3]);
                                m_dataCache->updateValue(deviceId, addr, static_cast<double>(val));
                                emit deviceRegisterUpdated(deviceId, addr, val, QString(), QString());
                            }
                        }
                            });
                    connect(parser, &BinaryFrameParser::parseError,
                            this, [this, deviceId, dn](const QString &reason) {
                                if (m_logManager) {
                                    m_logManager->log(LogLevel::Warning, LogCategory::Protocol,
                                        QString("二进制帧解析错误 [设备=%1]: %2").arg(dn, reason));
                                }
                            });
                    m_binaryParsers.insert(deviceId, parser);
                }
                m_binaryParsers[deviceId]->feedData(data);
                // 二进制帧协议：也要回显 RX + 响应耗时（用于透明调试）
                if (hasPending) {
                    emit modbusResponseReceived(deviceId, txHex,
                        QString::fromLatin1(data.toHex(' ').toUpper()),
                        responseTimeMs);
                }

            } else if (cfg.protocolType == ProtocolType::ModbusTcp) {
                // ---- Modbus TCP：使用缓冲区处理粘包/半包 ----
                m_tcpBuffers[deviceId].append(data);
                QByteArray &buf = m_tcpBuffers[deviceId];

                while (buf.size() >= 7) { // 至少需要 MBAP 头(7B)
                    // 解析 MBAP 头获取 length
                    const quint16 length =
                        (static_cast<quint8>(buf[4]) << 8) | static_cast<quint8>(buf[5]);

                    // 防御：MBAP length 字段最小 1（unitId），最大不超过 254
                    if (length < 1 || length > 254) {
                        if (m_logManager) {
                            m_logManager->log(LogLevel::Warning, LogCategory::Protocol,
                                QString("Modbus TCP 非法 length=%1 [设备=%2]，清空缓冲区")
                                    .arg(length).arg(dn));
                        }
                        buf.clear();
                        break;
                    }

                    const int frameSize = 6 + static_cast<int>(length); // MBAP 6B + length
                    if (buf.size() < frameSize) {
                        // 半包：等待更多数据
                        break;
                    }

                    // 提取完整帧
                    QByteArray frame = buf.left(frameSize);
                    buf.remove(0, frameSize);

                    // 解析响应
                    ModbusResponse resp = ModbusTcpProtocol::parseResponseRaw(frame);
                    if (!resp.isException) {
                        // 根据功能码决定是否更新数据缓存
                        switch (resp.functionCode) {
                        case ModbusFunctionCode::ReadCoils:
                        case ModbusFunctionCode::ReadDiscreteInputs:
                        case ModbusFunctionCode::ReadHoldingRegisters:
                        case ModbusFunctionCode::ReadInputRegisters: {
                            // 读操作：响应包含实际数据，更新缓存
                            for (int i = 0; i < resp.data.size(); ++i) {
                                const quint16 addr = static_cast<quint16>(i);
                                const double value = static_cast<double>(resp.data[i]);
                                m_dataCache->updateValue(deviceId, addr, value);
                                emit deviceRegisterUpdated(deviceId, addr, value, QString(), QString());
                            }
                            break;
                        }
                        case ModbusFunctionCode::WriteSingleRegister: {
                            // 写单寄存器：响应回显 [地址, 值]，值是实际写入的值
                            if (resp.data.size() >= 2) {
                                const quint16 addr = resp.data[0];
                                const double value = static_cast<double>(resp.data[1]);
                                m_dataCache->updateValue(deviceId, addr, value);
                                emit deviceRegisterUpdated(deviceId, addr, value, QString(), QString());
                            }
                            break;
                        }
                        case ModbusFunctionCode::WriteMultipleRegisters:
                        case ModbusFunctionCode::WriteSingleCoil:
                        case ModbusFunctionCode::WriteMultipleCoils: {
                            // 写多寄存器/线圈：响应只回显 [起始地址, 数量]，不包含实际值
                            // 不更新数据缓存，保持原有值
                            break;
                        }
                        default:
                            break;
                        }
                    }

                    // Modbus TCP 回显（仅回显第一帧，避免重复回显）
                    if (frameSize == 6 + static_cast<int>(length) &&
                        buf.isEmpty()) {
                        emit modbusResponseReceived(deviceId,
                            hasPending ? txHex : QString(),
                            QString::fromLatin1(frame.toHex(' ').toUpper()),
                            responseTimeMs);
                    }
                }

                // 如果缓冲区太大（异常情况），清空
                if (buf.size() > 1024) {
                    if (m_logManager) {
                        m_logManager->log(LogLevel::Warning, LogCategory::Protocol,
                            QString("Modbus TCP 缓冲区溢出 [设备=%1]，清空")
                                .arg(dn));
                    }
                    buf.clear();
                }

                // 只有在缓冲区处理完且没有更多数据时才回显原始数据
                if (buf.isEmpty()) {
                    emit modbusResponseReceived(deviceId,
                        hasPending ? txHex : QString(),
                        QString::fromLatin1(data.toHex(' ').toUpper()),
                        responseTimeMs);
                }
            }
        });

    // 2. 设备配置变更 → 持久化到 SQLite
    connect(m_deviceManager, &DeviceManager::deviceConfigChanged,
            this, [this](const QString &deviceId) {
            if (!m_dataPersistent || !m_deviceManager) return;
            DeviceConfig cfg = m_deviceManager->getDeviceConfig(deviceId);
            if (cfg.id.isEmpty()) return;

            // 确定设备类型
            QString typeStr;
            switch (cfg.commType) {
            case CommunicationType::Serial:   typeStr = "serial"; break;
            case CommunicationType::Tcp:      typeStr = "tcp"; break;
            case CommunicationType::Udp:      typeStr = "udp"; break;
            case CommunicationType::Mqtt:     typeStr = "mqtt"; break;
            default:                           typeStr = "unknown"; break;
            }

            // 序列化为 JSON
            QJsonObject cfgJson;
            QJsonObject commObj;
            cfgJson["id"]          = cfg.id;
            cfgJson["name"]        = cfg.name;
            cfgJson["commType"]    = static_cast<int>(cfg.commType);
            cfgJson["protocolType"]= static_cast<int>(cfg.protocolType);
            cfgJson["enabled"]     = cfg.enabled;
            cfgJson["pollInterval"]= cfg.pollInterval;
            cfgJson["timeout"]     = cfg.timeout;
            cfgJson["autoReconnect"] = cfg.autoReconnect;

            // 序列化通信配置
            const auto &cc = cfg.commConfig;
            switch (cc.type) {
            case CommunicationType::Serial:
                commObj["type"]       = "serial";
                commObj["portName"]   = cc.serial.portName;
                commObj["baudRate"]   = cc.serial.baudRate;
                commObj["dataBits"]   = cc.serial.dataBits;
                commObj["stopBits"]   = cc.serial.stopBits;
                commObj["parity"]     = static_cast<int>(cc.serial.parity);
                commObj["flowControl"]= static_cast<int>(cc.serial.flowControl);
                break;
            case CommunicationType::Tcp:
                commObj["type"]            = "tcp";
                commObj["hostAddress"]     = cc.tcp.hostAddress;
                commObj["port"]            = cc.tcp.port;
                commObj["connectionTimeout"]= cc.tcp.connectionTimeout;
                break;
            case CommunicationType::Udp:
                commObj["type"]        = "udp";
                commObj["hostAddress"] = cc.udp.hostAddress;
                commObj["port"]        = cc.udp.port;
                commObj["localPort"]   = cc.udp.localPort;
                break;
            case CommunicationType::Mqtt:
                commObj["type"]          = "mqtt";
                commObj["brokerHost"]    = cc.mqtt.brokerHost;
                commObj["brokerPort"]    = cc.mqtt.brokerPort;
                commObj["clientId"]      = cc.mqtt.clientId;
                commObj["username"]      = cc.mqtt.username;
                commObj["password"]      = cc.mqtt.password;
                commObj["subscribeTopic"]= cc.mqtt.subscribeTopic;
                commObj["publishTopic"]  = cc.mqtt.publishTopic;
                commObj["qos"]           = cc.mqtt.qos;
                commObj["keepAlive"]     = cc.mqtt.keepAlive;
                commObj["cleanSession"]  = cc.mqtt.cleanSession;
                break;
            default:
                commObj["type"] = "unknown";
                break;
            }
            cfgJson["commConfig"] = commObj;

            QJsonDocument doc(cfgJson);
            QString configJsonStr = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));

            // 写入数据库（addDevice 用 ON CONFLICT DO UPDATE 实现 upsert）
            m_dataPersistent->addDevice(cfg.id, cfg.name, typeStr, configJsonStr);
        });

    // 1b. MQTT 设备消息：topic + payload -> MqttProtocol JSON -> DataCache
    connect(m_deviceManager, &DeviceManager::deviceMqttMessage,
            this, [this](const QString &deviceId,
                        const QString &topic,
                        const QByteArray &payload) {
        if (deviceId.isEmpty() || payload.isEmpty()) return;

        // Helper: resolve deviceId to display name for log messages
        auto dnMqtt = [&]() -> QString {
            auto c = DeviceManager::instance().getDeviceConfig(deviceId);
            return (!c.name.isEmpty() && c.name != c.id) ? c.name : deviceId;
        }();

        // 广播原始 payload 给 UI（日志面板显示 + 着色解析视图）
        emit deviceRawDataReceived(deviceId, payload);

        if (!m_dataCache) return;

        const DeviceConfig cfg = m_deviceManager
                                    ? m_deviceManager->getDeviceConfig(deviceId)
                                    : DeviceConfig();
        if (cfg.id.isEmpty()) return;
        if (cfg.protocolType != ProtocolType::MqttJson) {
            // 非 MqttJson 协议：仅透传，不做 JSON 解析
            return;
        }

        QList<MqttDataPoint> points = MqttProtocol::parseMessage(topic, payload);
        if (points.isEmpty()) {
            if (m_logManager) {
                m_logManager->log(LogLevel::Warning, LogCategory::Protocol,
                    QString("MQTT JSON 解析失败 [设备=%1, topic=%2, payload=%3]")
                        .arg(dnMqtt, topic, QString::fromUtf8(payload.left(200))));
            }
            return;
        }
        // ----【关键】写入 deviceId 必须用 lambda 入参 deviceId（订阅所属的真实设备）
        //     JSON payload 里的 p.deviceId 仅作为诊断字段，绝不允许覆盖真实 id。
        //     否则会有两个严重 bug：
        //       1) emit deviceRegisterUpdated(错误ID) → UI 订阅的是真实设备ID，收不到
        //       2) DB 插入 register_data.device_id=错误ID → FOREIGN KEY 约束失败
        //     （我们的 partners/mqtt_partner_v2.py 故意在 JSON 里放 "deviceId": "devA"
        //      就是为了测这类字段不可信的场景）
        int warnMatches = 0;
        for (const MqttDataPoint &p : points) {
            const QString &realDeviceId = deviceId;   // 强制用订阅绑定的真实ID

            // 诊断：如果 JSON 里声明了 deviceId 但和真实设备名/id 不匹配，打 Warning
            if (!p.deviceId.isEmpty() && p.deviceId != cfg.name && p.deviceId != realDeviceId) {
                if (warnMatches < 3) {   // 最多打 3 条，避免高频连发刷屏
                    const QString msg =
                        QString("MQTT payload.deviceId=\"%1\" 和真实订阅设备不匹配 "
                                "(真实ID=%2 / 设备名=%3) —— 已忽略 payload.deviceId，"
                                "继续写入真实设备寄存器 addr=%4 val=%5")
                            .arg(p.deviceId, realDeviceId, cfg.name)
                            .arg(p.address).arg(p.value, 0, 'g', 6);
                    fprintf(stderr, "[MQTT SL WARN] %s\n", msg.toUtf8().constData());
                    fflush(stderr);
                    if (m_logManager) {
                        m_logManager->log(LogLevel::Warning, LogCategory::Protocol, msg);
                    }
                    ++warnMatches;
                }
            }

            // 双路诊断 Info：写入前打印（每批次的前 6 条详细打印，之后只报计数）
            int diagCount = 0;
            if (diagCount < 6) {
                const QString dbg = QString("MQTT WRITE → deviceId=%1, addr=%2, value=%3")
                                        .arg(realDeviceId).arg(p.address)
                                        .arg(p.value, 0, 'g', 8);
                fprintf(stderr, "[MQTT SL] %s\n", dbg.toUtf8().constData());
                fflush(stderr);
                if (m_logManager) {
                    m_logManager->log(LogLevel::Debug, QStringLiteral("data"), dbg);
                }
                ++diagCount;
            }

            // 真正写入：用 realDeviceId，不再信任 p.deviceId
            m_dataCache->updateValue(realDeviceId, p.address, p.value);
            emit deviceRegisterUpdated(realDeviceId, p.address, p.value,
                                       QString(), QString());
        }
        Q_UNUSED(warnMatches);
        if (m_logManager) {
            m_logManager->log(LogLevel::Debug, LogCategory::Protocol,
                QString("MQTT 解析: [设备=%1, topic=%2] %3 points")
                    .arg(dnMqtt, topic).arg(points.size()));
        }
    });

    // 4. 数据缓存 → 持久化（批量节流）
    //    使用定时器收集变化值，每 100ms 批量写入数据库
    m_batchTimer = new QTimer(this);
    m_batchTimer->setInterval(100);
    m_batchTimer->setSingleShot(false);

    connect(m_dataCache, &DataCache::valueChanged,
            this, [this](const QString &deviceId,
                         quint16 registerAddr,
                         double value) {
        PendingRegisterData item;
        item.deviceId = deviceId;
        item.registerAddr = registerAddr;
        item.value = value;
        m_pendingRegisterData.append(item);

        if (!m_batchTimer->isActive()) {
            m_batchTimer->start();
        }
    });

    connect(m_batchTimer, &QTimer::timeout,
            this, [this]() {
        if (m_pendingRegisterData.isEmpty()) {
            m_batchTimer->stop();
            return;
        }

        // 批量写入数据库（事务 + 参数化批处理，性能提升约 5-10 倍）
        if (m_dataPersistent) {
            QVector<QPair<QString, QPair<quint16, double>>> batch;
            batch.reserve(m_pendingRegisterData.size());
            for (const auto &item : m_pendingRegisterData) {
                batch.append(qMakePair(item.deviceId,
                                       qMakePair(item.registerAddr, item.value)));
            }
            m_dataPersistent->insertRegisterDataBatch(batch);
        }

        m_pendingRegisterData.clear();
        m_batchTimer->stop();
    });

    // ------------------------------------------------------------------------
    // 6. 告警链路：DataCache → AlertManager → DB + 日志 + UI 广播
    // ------------------------------------------------------------------------
    if (m_dataCache && m_alertManager) {
        connect(m_dataCache, &DataCache::valueChanged,
                m_alertManager, &AlertManager::checkValue);
    }

    if (m_alertManager) {
        connect(m_alertManager, &AlertManager::alertTriggered,
                this, [this](const Alert &a) {
            // (a) 落库 alerts 表
            if (m_dataPersistent) {
                m_dataPersistent->insertAlert(a.deviceId, a.registerAddr,
                                              a.value, a.level, a.message);
            }
            // (b) 写日志（Error 级 → LogWidget 红色）
            if (m_logManager) {
                m_logManager->log(LogLevel::Error, LogCategory::Alert,
                    QString("[%1] 寄存器 0x%2 值=%3，触发告警：%4")
                        .arg([&](){ auto c=DeviceManager::instance().getDeviceConfig(a.deviceId); return (!c.name.isEmpty()&&c.name!=c.id)?c.name:a.deviceId; }())
                        .arg(a.registerAddr, 4, 16, QLatin1Char('0'))
                        .arg(a.value, 0, 'f', 3)
                        .arg(a.message));
            }
            // (c) 广播 alertOccurred → MainWindow 状态栏更新计数 + 点击查看
            emit alertOccurred(a.deviceId, a.registerAddr, a.value,
                               a.level, a.message, a.timestamp);
        });
    }

    // 5. 日志管理器信号 → LogWidget（仅 UI 渲染，不再双写控制台）
    //    【注意】LogManager::log() 内部已经通过 fprintf(stderr) 输出到应用程序输出面板，
    //    这里不要再次 qInfo()/qDebug()，否则每条日志会在应用程序输出里出现两遍。
    Q_UNUSED(m_logManager);

    m_initialized = true;
    m_logManager->log(LogLevel::Info, LogCategory::App,
                      "All services initialized successfully");
}

void ServiceLocator::shutdown()
{
    if (!m_initialized) {
        return;
    }

    qInfo() << "ServiceLocator::shutdown() - shutting down services...";

    if (m_logManager) {
        m_logManager->log(LogLevel::Info, LogCategory::App,
                          "Shutting down services...");
    }

    // 停止批量写入定时器
    if (m_batchTimer) {
        m_batchTimer->stop();
        m_batchTimer->deleteLater();
        m_batchTimer = nullptr;
    }

    // 清空待写入数据
    m_pendingRegisterData.clear();

    // 删除子对象（QPointer 会自动置空）
    if (m_dataCache)         { delete m_dataCache; m_dataCache = nullptr; }
    if (m_dataPersistent)    { delete m_dataPersistent; m_dataPersistent = nullptr; }
    if (m_alertManager)      { delete m_alertManager; m_alertManager = nullptr; }
    if (m_playbackEngine)    { delete m_playbackEngine; m_playbackEngine = nullptr; }

    // 单例对象不需要删除
    m_deviceManager = nullptr;
    m_logManager = nullptr;

    m_initialized = false;

    LogManager::instance().log(LogLevel::Info, LogCategory::App,
                               "All services shut down");
}

DataCache *ServiceLocator::dataCache() const
{
    return m_dataCache;
}

DataPersistent *ServiceLocator::dataPersistent() const
{
    return m_dataPersistent;
}

AlertManager *ServiceLocator::alertManager() const
{
    return m_alertManager;
}

LogManager *ServiceLocator::logManager() const
{
    return m_logManager;
}

DeviceManager *ServiceLocator::deviceManager() const
{
    return m_deviceManager;
}

PlaybackEngine *ServiceLocator::playbackEngine() const
{
    return m_playbackEngine;
}

void ServiceLocator::markSendTimestamp(const QString &deviceId, const QByteArray &txFrame, int timeoutMs)
{
    if (deviceId.isEmpty()) return;

    // Helper: resolve deviceId to display name for log messages
    auto dnTs = [&]() -> QString {
        auto c = DeviceManager::instance().getDeviceConfig(deviceId);
        return (!c.name.isEmpty() && c.name != c.id) ? c.name : deviceId;
    }();
    PendingSend ps;
    ps.sendTsMs  = QDateTime::currentMSecsSinceEpoch();
    ps.txFrame   = txFrame;
    ps.timeoutMs = timeoutMs > 0 ? timeoutMs : 3000;
    m_pendingSends.insert(deviceId, ps);

    // 异步超时单 Shot：若到期仍存在则视为响应超时
    auto *timer = new QTimer(this);
    timer->setSingleShot(true);
    timer->start(ps.timeoutMs);
    connect(timer, &QTimer::timeout, this, [this, deviceId, timer, dnTs]() {
        timer->deleteLater();
        if (!m_pendingSends.contains(deviceId)) return;
        // 仅在完全没有后续匹配数据时视为超时（避免和真实响应竞速误判）
        const auto ps2 = m_pendingSends.value(deviceId);
        const qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - ps2.sendTsMs;
        if (elapsed >= ps2.timeoutMs) {
            const QString txHex =
                QString::fromLatin1(ps2.txFrame.toHex(' ').toUpper());
            m_pendingSends.remove(deviceId);
            emit modbusRequestTimedOut(deviceId, txHex);
            if (m_logManager) {
                m_logManager->log(LogLevel::Warning, LogCategory::Protocol,
                    QString("命令响应超时 [设备=%1, 耗时=%2ms, TX=%3]")
                        .arg(dnTs).arg(elapsed).arg(txHex));
            }
        }
    });
}

bool ServiceLocator::isInitialized() const
{
    return m_initialized;
}
