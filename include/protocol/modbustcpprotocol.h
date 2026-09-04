#ifndef MODBUSTCPPROTOCOL_H
#define MODBUSTCPPROTOCOL_H

#include <QByteArray>
#include <QVector>
#include <QtGlobal>
#include "protocol/modbusrtuprotocol.h"

/**
 * @brief Modbus TCP 协议实现
 *
 * 提供 Modbus TCP 协议的帧构建和解析功能。
 * Modbus TCP 使用 MBAP (Modbus Application Protocol) 报文头封装：
 *   [Transaction ID(2B)][Protocol ID(2B, 固定0)][Length(2B)][Unit ID(1B)][PDU(N B)]
 *
 * Transaction ID 用于匹配请求与响应，由本类自动管理（0-65535 循环）。
 */
class ModbusTcpProtocol
{
public:
    /**
     * @brief 构造函数，自动初始化事务 ID
     * @param unitId 单元 ID（默认 1，对应 RTU 从站地址）
     */
    explicit ModbusTcpProtocol(quint8 unitId = 1);

    /**
     * @brief 获取当前事务 ID
     * @return 当前事务 ID
     */
    quint16 currentTransactionId() const;

    /**
     * @brief 构建读保持寄存器请求
     * @param unitId 单元 ID
     * @param startAddr 起始寄存器地址
     * @param count 读取数量
     * @return Modbus TCP 帧数据（含 MBAP 头）
     */
    QByteArray buildReadHoldingRegisters(quint8 unitId, quint16 startAddr,
                                          quint16 count);

    /**
     * @brief 构建写单个寄存器请求
     * @param unitId 单元 ID
     * @param addr 寄存器地址
     * @param value 写入值
     * @return Modbus TCP 帧数据（含 MBAP 头）
     */
    QByteArray buildWriteSingleRegister(quint8 unitId, quint16 addr,
                                         quint16 value);

    /**
     * @brief 构建写多个寄存器请求
     * @param unitId 单元 ID
     * @param startAddr 起始寄存器地址
     * @param values 写入值列表
     * @return Modbus TCP 帧数据（含 MBAP 头）
     */
    QByteArray buildWriteMultipleRegisters(quint8 unitId, quint16 startAddr,
                                             const QVector<quint16> &values);

    /**
     * @brief 构建通用 Modbus TCP 请求
     * @param functionCode 功能码
     * @param unitId 单元 ID
     * @param pdu 协议数据单元（PDU）
     * @return Modbus TCP 帧数据（含 MBAP 头）
     */
    QByteArray buildRequest(ModbusFunctionCode functionCode, quint8 unitId,
                             const QByteArray &pdu);

    /**
     * @brief 解析 Modbus TCP 响应
     * @param response 接收到的 TCP 响应数据
     * @param transactionId 对应的事务 ID
     * @return 解析后的 ModbusResponse 结构
     */
    ModbusResponse parseResponse(const QByteArray &response,
                                  quint16 transactionId = 0);

    /**
     * @brief 从原始 Modbus TCP 数据解析响应（静态方法）
     * @param response 接收到的 TCP 响应数据（含 MBAP 头）
     * @return 解析后的 ModbusResponse 结构
     */
    static ModbusResponse parseResponseRaw(const QByteArray &response);

private:
    /**
     * @brief 生成下一个事务 ID
     * @return 新的事务 ID
     */
    quint16 nextTransactionId();

    /**
     * @brief 封装 MBAP 报文头
     * @param pduLen PDU 长度
     * @param unitId 单元 ID
     * @return MBAP 头 + PDU 的完整数据
     */
    QByteArray encapsulateMbap(quint16 pduLen, quint8 unitId);

    quint16 m_transactionId;  ///< 当前事务 ID
    quint8 m_defaultUnitId;    ///< 默认单元 ID

    static constexpr quint16 PROTOCOL_ID = 0;  ///< 协议 ID（固定为 0）
};

#endif // MODBUSTCPPROTOCOL_H
