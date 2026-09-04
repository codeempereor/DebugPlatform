#ifndef MODBUSRTUPROTOCOL_H
#define MODBUSRTUPROTOCOL_H

#include <QByteArray>
#include <QVector>
#include <QString>
#include <QtGlobal>

/**
 * @brief Modbus 功能码枚举
 */
enum class ModbusFunctionCode : quint8 {
    ReadCoils               = 0x01,  ///< 读线圈
    ReadDiscreteInputs      = 0x02,  ///< 读离散输入
    ReadHoldingRegisters    = 0x03,  ///< 读保持寄存器
    ReadInputRegisters      = 0x04,  ///< 读输入寄存器
    WriteSingleCoil         = 0x05,  ///< 写单个线圈
    WriteSingleRegister     = 0x06,  ///< 写单个寄存器
    WriteMultipleCoils      = 0x0F,  ///< 写多个线圈
    WriteMultipleRegisters  = 0x10   ///< 写多个寄存器
};

/**
 * @brief Modbus 异常码枚举
 */
enum class ModbusExceptionCode : quint8 {
    IllegalFunction        = 0x01,  ///< 非法功能码
    IllegalDataAddress     = 0x02,  ///< 非法数据地址
    IllegalDataValue       = 0x03,  ///< 非法数据值
    SlaveDeviceFailure     = 0x04,  ///< 从站设备故障
    Acknowledge            = 0x05,  ///< 确认
    SlaveDeviceBusy        = 0x06,  ///< 从站忙
    MemoryParityError      = 0x08,  ///< 内存奇偶校验错误
    GatewayPathUnavailable = 0x0A,  ///< 网关路径不可用
    GatewayTargetFailed    = 0x0B   ///< 网关目标设备响应失败
};

/**
 * @brief Modbus 请求结构
 */
struct ModbusRequest {
    quint8 slaveAddress;        ///< 从站地址 (1-247)
    ModbusFunctionCode functionCode;  ///< 功能码
    quint16 startAddress;        ///< 起始寄存器地址
    quint16 quantity;            ///< 读/写数量或寄存器值
    QVector<quint16> data;      ///< 写入数据（写多个寄存器时使用）

    ModbusRequest()
        : slaveAddress(0)
        , functionCode(ModbusFunctionCode::ReadHoldingRegisters)
        , startAddress(0)
        , quantity(0)
    {
    }
};

/**
 * @brief Modbus 响应结构
 */
struct ModbusResponse {
    quint8 slaveAddress;        ///< 从站地址
    ModbusFunctionCode functionCode;  ///< 功能码
    QVector<quint16> data;      ///< 寄存器数据
    bool isException;           ///< 是否为异常响应
    ModbusExceptionCode exceptionCode;  ///< 异常码（仅当 isException=true 时有效）

    ModbusResponse()
        : slaveAddress(0)
        , functionCode(ModbusFunctionCode::ReadHoldingRegisters)
        , isException(false)
        , exceptionCode(ModbusExceptionCode::IllegalFunction)
    {
    }
};

/**
 * @brief Modbus RTU 协议实现
 *
 * 提供 Modbus RTU 协议的帧构建和解析功能。
 * Modbus RTU 帧格式：
 *   [从站地址(1B)][功能码(1B)][数据域(N B)][CRC16(2B, 低字节在前)]
 *
 * 帧间静默时间至少为 3.5 个字符时间。
 */
class ModbusRtuProtocol
{
public:
    ModbusRtuProtocol() = delete;  // 静态工具类

    /**
     * @brief 构建读保持寄存器请求
     * @param slave 从站地址 (1-247)
     * @param startAddr 起始寄存器地址
     * @param count 读取数量 (1-125)
     * @return Modbus RTU 帧数据
     */
    static QByteArray buildReadHoldingRegisters(quint8 slave, quint16 startAddr,
                                                 quint16 count);

    /**
     * @brief 构建写单个寄存器请求
     * @param slave 从站地址 (1-247)
     * @param addr 寄存器地址
     * @param value 写入值
     * @return Modbus RTU 帧数据
     */
    static QByteArray buildWriteSingleRegister(quint8 slave, quint16 addr,
                                                quint16 value);

    /**
     * @brief 构建写多个寄存器请求
     * @param slave 从站地址 (1-247)
     * @param startAddr 起始寄存器地址
     * @param values 写入值列表
     * @return Modbus RTU 帧数据
     */
    static QByteArray buildWriteMultipleRegisters(quint8 slave, quint16 startAddr,
                                                   const QVector<quint16> &values);

    /**
     * @brief 解析 Modbus RTU 响应
     * @param response 接收到的 RTU 响应数据
     * @param request 原始请求（用于验证响应匹配）
     * @return 解析后的 ModbusResponse 结构
     */
    static ModbusResponse parseResponse(const QByteArray &response,
                                         const ModbusRequest &request);

    /**
     * @brief 解析 Modbus RTU 响应（从响应数据直接解析）
     * @param response 接收到的 RTU 响应数据
     * @return 解析后的 ModbusResponse 结构
     */
    static ModbusResponse parseResponse(const QByteArray &response);

    /**
     * @brief 获取异常码描述
     * @param code 异常码
     * @return 描述字符串
     */
    static QString exceptionCodeToString(ModbusExceptionCode code);

private:
    /**
     * @brief 为帧添加 CRC16 校验
     * @param frame 待添加 CRC 的帧（前序数据）
     */
    static void appendCrc(QByteArray &frame);

    /**
     * @brief 验证帧的 CRC16 校验
     * @param frame 完整帧数据（含 CRC）
     * @return true 校验通过
     */
    static bool verifyCrc(const QByteArray &frame);
};

#endif // MODBUSRTUPROTOCOL_H
