#include "protocol/modbustcpprotocol.h"

ModbusTcpProtocol::ModbusTcpProtocol(quint8 unitId)
    : m_transactionId(0)
    , m_defaultUnitId(unitId)
{
}

quint16 ModbusTcpProtocol::currentTransactionId() const
{
    return m_transactionId;
}

quint16 ModbusTcpProtocol::nextTransactionId()
{
    quint16 id = m_transactionId;
    m_transactionId++;
    return id;
}

QByteArray ModbusTcpProtocol::encapsulateMbap(quint16 pduLen, quint8 unitId)
{
    QByteArray mbap;

    quint16 tid = nextTransactionId();

    // [Transaction ID(2B)]
    mbap.append(static_cast<char>((tid >> 8) & 0xFF));
    mbap.append(static_cast<char>(tid & 0xFF));

    // [Protocol ID(2B, 固定0)]
    mbap.append(static_cast<char>((PROTOCOL_ID >> 8) & 0xFF));
    mbap.append(static_cast<char>(PROTOCOL_ID & 0xFF));

    // [Length(2B) - PDU 长度 + Unit ID]
    quint16 length = pduLen + 1; // PDU + Unit ID
    mbap.append(static_cast<char>((length >> 8) & 0xFF));
    mbap.append(static_cast<char>(length & 0xFF));

    // [Unit ID(1B)]
    mbap.append(static_cast<char>(unitId));

    return mbap;
}

QByteArray ModbusTcpProtocol::buildReadHoldingRegisters(quint8 unitId,
                                                         quint16 startAddr,
                                                         quint16 count)
{
    QByteArray pdu;

    // PDU: [功能码 0x03][起始地址高][起始地址低][数量高][数量低]
    pdu.append(static_cast<char>(ModbusFunctionCode::ReadHoldingRegisters));
    pdu.append(static_cast<char>((startAddr >> 8) & 0xFF));
    pdu.append(static_cast<char>(startAddr & 0xFF));
    pdu.append(static_cast<char>((count >> 8) & 0xFF));
    pdu.append(static_cast<char>(count & 0xFF));

    return encapsulateMbap(static_cast<quint16>(pdu.size()), unitId) + pdu;
}

QByteArray ModbusTcpProtocol::buildWriteSingleRegister(quint8 unitId,
                                                        quint16 addr,
                                                        quint16 value)
{
    QByteArray pdu;

    // PDU: [功能码 0x06][地址高][地址低][值高][值低]
    pdu.append(static_cast<char>(ModbusFunctionCode::WriteSingleRegister));
    pdu.append(static_cast<char>((addr >> 8) & 0xFF));
    pdu.append(static_cast<char>(addr & 0xFF));
    pdu.append(static_cast<char>((value >> 8) & 0xFF));
    pdu.append(static_cast<char>(value & 0xFF));

    return encapsulateMbap(static_cast<quint16>(pdu.size()), unitId) + pdu;
}

QByteArray ModbusTcpProtocol::buildWriteMultipleRegisters(
    quint8 unitId, quint16 startAddr, const QVector<quint16> &values)
{
    QByteArray pdu;

    quint16 quantity = static_cast<quint16>(values.size());
    quint8 byteCount = static_cast<quint8>(quantity * 2);

    // PDU: [功能码 0x10][起始地址高][起始地址低][数量高][数量低][字节数]
    pdu.append(static_cast<char>(ModbusFunctionCode::WriteMultipleRegisters));
    pdu.append(static_cast<char>((startAddr >> 8) & 0xFF));
    pdu.append(static_cast<char>(startAddr & 0xFF));
    pdu.append(static_cast<char>((quantity >> 8) & 0xFF));
    pdu.append(static_cast<char>(quantity & 0xFF));
    pdu.append(static_cast<char>(byteCount));

    // [寄存器值1高][寄存器值1低]...
    for (quint16 value : values) {
        pdu.append(static_cast<char>((value >> 8) & 0xFF));
        pdu.append(static_cast<char>(value & 0xFF));
    }

    return encapsulateMbap(static_cast<quint16>(pdu.size()), unitId) + pdu;
}

QByteArray ModbusTcpProtocol::buildRequest(ModbusFunctionCode functionCode,
                                            quint8 unitId,
                                            const QByteArray &pdu)
{
    QByteArray fullPdu;
    fullPdu.append(static_cast<char>(functionCode));
    fullPdu.append(pdu);

    return encapsulateMbap(static_cast<quint16>(fullPdu.size()), unitId) + fullPdu;
}

ModbusResponse ModbusTcpProtocol::parseResponse(const QByteArray &response,
                                                  quint16 transactionId)
{
    ModbusResponse result;

    // 至少需要 MBAP 头 (7 字节) + 功能码 (1 字节)
    if (response.size() < 8) {
        result.isException = true;
        result.exceptionCode = ModbusExceptionCode::SlaveDeviceFailure;
        return result;
    }

    // 解析 MBAP 头
    quint16 tid =
        (static_cast<quint16>(static_cast<unsigned char>(response[0])) << 8) |
        static_cast<quint16>(static_cast<unsigned char>(response[1]));

    quint16 protocolId =
        (static_cast<quint16>(static_cast<unsigned char>(response[2])) << 8) |
        static_cast<quint16>(static_cast<unsigned char>(response[3]));

    quint16 length =
        (static_cast<quint16>(static_cast<unsigned char>(response[4])) << 8) |
        static_cast<quint16>(static_cast<unsigned char>(response[5]));


    // 验证 MBAP length 字段：表示从 unitId 开始的剩余字节数，合法范围 [1, 254]
    if (length < 1 || length > 254) {
        result.isException = true;
        result.exceptionCode = ModbusExceptionCode::SlaveDeviceFailure;
        return result;
    }
    // 校验实际数据长度是否与 length 声明一致
    if (response.size() < 6 + static_cast<int>(length)) {
        result.isException = true;
        result.exceptionCode = ModbusExceptionCode::SlaveDeviceFailure;
        return result;
    }

    quint8 unitId = static_cast<quint8>(response[6]);

    // 验证协议 ID
    if (protocolId != PROTOCOL_ID) {
        result.isException = true;
        result.exceptionCode = ModbusExceptionCode::SlaveDeviceFailure;
        return result;
    }

    // 验证事务 ID（如果指定了的话）
    if (transactionId != 0 && tid != transactionId) {
        result.isException = true;
        result.exceptionCode = ModbusExceptionCode::SlaveDeviceFailure;
        return result;
    }

    result.slaveAddress = unitId;

    // PDU 从第 7 字节开始
    int pduOffset = 7;
    quint8 funcCode = static_cast<quint8>(response[pduOffset]);
    result.functionCode = static_cast<ModbusFunctionCode>(funcCode);

    // 检查是否为异常响应
    if (funcCode & 0x80) {
        result.isException = true;
        if (response.size() > pduOffset + 1) {
            result.exceptionCode = static_cast<ModbusExceptionCode>(
                static_cast<quint8>(response[pduOffset + 1]));
        } else {
            result.exceptionCode = ModbusExceptionCode::SlaveDeviceFailure;
        }
        return result;
    }

    result.isException = false;

    // 解析正常响应数据
    int dataOffset = pduOffset + 1;

    switch (result.functionCode) {
    case ModbusFunctionCode::ReadHoldingRegisters:
    case ModbusFunctionCode::ReadInputRegisters: {
        if (response.size() > dataOffset) {
            quint8 byteCount = static_cast<quint8>(response[dataOffset]);
            int regDataStart = dataOffset + 1;

            for (int i = regDataStart; i < regDataStart + byteCount &&
                 i + 1 < response.size(); i += 2) {
                quint16 regValue =
                    (static_cast<quint16>(static_cast<unsigned char>(response[i])) << 8) |
                    static_cast<quint16>(static_cast<unsigned char>(response[i + 1]));
                result.data.append(regValue);
            }
        }
        break;
    }

    case ModbusFunctionCode::ReadCoils:
    case ModbusFunctionCode::ReadDiscreteInputs: {
        if (response.size() > dataOffset) {
            quint8 byteCount = static_cast<quint8>(response[dataOffset]);
            int bitDataStart = dataOffset + 1;

            for (int i = bitDataStart; i < bitDataStart + byteCount &&
                 i < response.size(); ++i) {
                quint8 byte = static_cast<quint8>(response[i]);
                for (int bit = 0; bit < 8; ++bit) {
                    result.data.append((byte >> bit) & 0x01);
                }
            }
        }
        break;
    }

    case ModbusFunctionCode::WriteSingleRegister:
    case ModbusFunctionCode::WriteMultipleRegisters:
    case ModbusFunctionCode::WriteSingleCoil:
    case ModbusFunctionCode::WriteMultipleCoils: {
        if (response.size() > dataOffset + 3) {
            // 回显：[地址高][地址低][值/数量高][值/数量低]
            quint16 addr =
                (static_cast<quint16>(static_cast<unsigned char>(response[dataOffset])) << 8) |
                static_cast<quint16>(static_cast<unsigned char>(response[dataOffset + 1]));
            quint16 value =
                (static_cast<quint16>(static_cast<unsigned char>(response[dataOffset + 2])) << 8) |
                static_cast<quint16>(static_cast<unsigned char>(response[dataOffset + 3]));
            result.data.append(addr);
            result.data.append(value);
        }
        break;
    }

    default:
        result.isException = true;
        result.exceptionCode = ModbusExceptionCode::IllegalFunction;
        break;
    }

    return result;
}

ModbusResponse ModbusTcpProtocol::parseResponseRaw(const QByteArray &response)
{
    ModbusTcpProtocol parser;
    return parser.parseResponse(response);
}
