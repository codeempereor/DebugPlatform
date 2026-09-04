#ifndef DEBUGPLATFORM_CONFIG_H
#define DEBUGPLATFORM_CONFIG_H

/**
 * @file config.h
 * @brief 全局配置头文件
 *
 * 定义项目版本号、日志分类、通用宏、常量、枚举类型。
 * 所有模块共用此文件中的定义。
 */

// ============================================================================
// 版本信息
// ============================================================================
#define DEBUGPLATFORM_VERSION_MAJOR 1
#define DEBUGPLATFORM_VERSION_MINOR 2
#define DEBUGPLATFORM_VERSION_PATCH 0
#define DEBUGPLATFORM_VERSION       "1.2.0"
#define DEBUGPLATFORM_NAME          "Multi-Protocol Debug Platform"
#define DEBUGPLATFORM_ORGANIZATION  "DebugPlatform"

// ============================================================================
// 日志分类（配合 qInstallMessageHandler 使用）
// ============================================================================
namespace LogCategory {
    constexpr const char* Core     = "core";      // 通信抽象层
    constexpr const char* Protocol = "protocol";  // 协议解析层
    constexpr const char* Data     = "data";      // 数据管理层
    constexpr const char* Device   = "device";    // 设备管理层
    constexpr const char* UI       = "ui";        // UI 层
    constexpr const char* App      = "app";       // 应用层
    constexpr const char* Alert    = "alert";     // 告警
    constexpr const char* Playback = "playback";  // 回放
}

// ============================================================================
// 通用宏
// ============================================================================
// 禁用拷贝构造和赋值
#define DISABLE_COPY(Class) \
    Class(const Class&) = delete; \
    Class& operator=(const Class&) = delete;

// 禁用移动构造和赋值
#define DISABLE_MOVE(Class) \
    Class(Class&&) = delete; \
    Class& operator=(Class&&) = delete;

// 单例模式宏
#define SINGLETON(Class) \
public: \
    static Class& instance() { \
        static Class instance; \
        return instance; \
    } \
    DISABLE_COPY(Class) \
private: \
    Class() = default;

// ============================================================================
// 常量定义
// ============================================================================
namespace Constants {
    // 二进制帧格式
    constexpr unsigned char FRAME_HEADER_1 = 0xAA;
    constexpr unsigned char FRAME_HEADER_2 = 0x55;
    constexpr unsigned char FRAME_TAIL_1   = 0x0D;
    constexpr unsigned char FRAME_TAIL_2   = 0x0A;
    constexpr int FRAME_MIN_SIZE            = 7;  // 最小帧: 头(2) + 长度(2) + 负载(0) + CRC(2) + 尾(2) - 冗余=1

    // Modbus 常量
    constexpr int MODBUS_RTU_MIN_FRAME  = 4;   // Modbus RTU 最小帧: 地址 + 功能码 + CRC
    constexpr int MODBUS_TCP_HEADER_SIZE = 7;  // MBAP 报文头长度

    // 环形缓冲区
    constexpr int DEFAULT_RING_BUFFER_SIZE = 10000;
    constexpr int DEFAULT_POLL_INTERVAL    = 100;  // ms
    constexpr int DEFAULT_TIMEOUT          = 1000; // ms

    // 重连控制器
    constexpr int RECONNECT_BASE_DELAY    = 1000;  // ms, 初始退避延迟
    constexpr int RECONNECT_MAX_DELAY     = 32000; // ms, 最大退避延迟
    constexpr int RECONNECT_MAX_ATTEMPTS  = 10;    // 最大重连次数

    // MQTT 常量
    constexpr int MQTT_DEFAULT_PORT          = 1883;  // MQTT broker 默认端口
    constexpr int MQTT_DEFAULT_KEEP_ALIVE    = 60;     // keep alive (秒)
    constexpr int MQTT_PROTOCOL_LEVEL       = 4;      // MQTT 3.1.1 协议级别
    constexpr int MQTT_CONNECT_TIMEOUT       = 5000;   // 连接超时 (ms)

    // 数据库
    constexpr const char* DEFAULT_DB_PATH = "./debug_platform.db";
    constexpr int DEFAULT_DATA_RETENTION_DAYS = 30;
}

// ============================================================================
// 枚举类型
// ============================================================================
enum class CommunicationType {
    Serial,
    Tcp,
    Udp,
    Mqtt
};

enum class ProtocolType {
    BinaryFrame,
    ModbusRtu,
    ModbusTcp,
    MqttJson
};

enum class DeviceStatus {
    Offline,
    Connecting,
    Online,
    Error
};

enum class AlertLevel {
    None,
    Warning,
    Critical
};

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

// ============================================================================
// Qt 元类型声明（使枚举可用于 QVariant）
// ============================================================================
Q_DECLARE_METATYPE(CommunicationType)
Q_DECLARE_METATYPE(ProtocolType)
Q_DECLARE_METATYPE(DeviceStatus)
Q_DECLARE_METATYPE(AlertLevel)
Q_DECLARE_METATYPE(LogLevel)

// ============================================================================
// qHash 函数（Qt5 需要，使枚举可作为 QHash/QSet 的 key）
// ============================================================================
inline uint qHash(CommunicationType key, uint seed = 0) noexcept {
    return qHash(static_cast<int>(key), seed);
}

inline uint qHash(ProtocolType key, uint seed = 0) noexcept {
    return qHash(static_cast<int>(key), seed);
}

inline uint qHash(DeviceStatus key, uint seed = 0) noexcept {
    return qHash(static_cast<int>(key), seed);
}

inline uint qHash(AlertLevel key, uint seed = 0) noexcept {
    return qHash(static_cast<int>(key), seed);
}

inline uint qHash(LogLevel key, uint seed = 0) noexcept {
    return qHash(static_cast<int>(key), seed);
}

#endif // DEBUGPLATFORM_CONFIG_H
