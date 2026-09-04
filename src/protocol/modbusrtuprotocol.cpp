#include "protocol/modbusrtuprotocol.h"
#include "protocol/crc16.h"

QByteArray ModbusRtuProtocol::buildReadHoldingRegisters(quint8 slave,
                                                          quint16 startAddr,
                                                          quint16 count)
{
    QByteArray frame;

    // [从站地址][功能码 0x03][起始地址高字节][起始地址低字节][数量高字节][数量低字节]
    frame.append(static_cast<char>(slave));
    frame.append(static_cast<char>(ModbusFunctionCode::ReadHoldingRegisters));
    frame.append(static_cast<char>((startAddr >> 8) & 0xFF));
    frame.append(static_cast<char>(startAddr & 0xFF));
    frame.append(static_cast<char>((count >> 8) & 0xFF));
    frame.append(static_cast<char>(count & 0xFF));

    appendCrc(frame);
    return frame;
}

QByteArray ModbusRtuProtocol::buildWriteSingleRegister(quint8 slave,
                                                         quint16 addr,
                                                         quint16 value)
{
    QByteArray frame;

    // [从站地址][功能码 0x06][地址高字节][地址低字节][值高字节][值低字节]
    frame.append(static_cast<char>(slave));
    frame.append(static_cast<char>(ModbusFunctionCode::WriteSingleRegister));
    frame.append(static_cast<char>((addr >> 8) & 0xFF));
    frame.append(static_cast<char>(addr & 0xFF));
    frame.append(static_cast<char>((value >> 8) & 0xFF));
    frame.append(static_cast<char>(value & 0xFF));

    appendCrc(frame);
    return frame;
}

QByteArray ModbusRtuProtocol::buildWriteMultipleRegisters(
    quint8 slave, quint16 startAddr, const QVector<quint16> &values)
{
    QByteArray frame;

    quint16 quantity = static_cast<quint16>(values.size());
    quint8 byteCount = static_cast<quint8>(quantity * 2);

    // [从站地址][功能码 0x10][起始地址高][起始地址低][数量高][数量低][字节数]
    frame.append(static_cast<char>(slave));
    frame.append(static_cast<char>(ModbusFunctionCode::WriteMultipleRegisters));
    frame.append(static_cast<char>((startAddr >> 8) & 0xFF));
    frame.append(static_cast<char>(startAddr & 0xFF));
    frame.append(static_cast<char>((quantity >> 8) & 0xFF));
    frame.append(static_cast<char>(quantity & 0xFF));
    frame.append(static_cast<char>(byteCount));

    // [寄存器值1高][寄存器值1低]...
    for (quint16 value : values) {
        frame.append(static_cast<char>((value >> 8) & 0xFF));
        frame.append(static_cast<char>(value & 0xFF));
    }

    appendCrc(frame);
    return frame;
}

ModbusResponse ModbusRtuProtocol::parseResponse(const QByteArray &response,
                                                   const ModbusRequest &request)
{
    ModbusResponse result;
    result.slaveAddress = request.slaveAddress;
    result.functionCode = request.functionCode;

    if (response.size() < 5) {
        // 最小响应：地址(1) + 功能码(1) + CRC(2) = 4 字节，加数据至少5字节
        result.isException = true;
        result.exceptionCode = ModbusExceptionCode::SlaveDeviceFailure;
        return result;
    }

    if (!verifyCrc(response)) {
        result.isException = true;
        result.exceptionCode = ModbusExceptionCode::SlaveDeviceFailure;
        return result;
    }

    quint8 slaveAddr = static_cast<quint8>(response[0]);
    quint8 funcCode = static_cast<quint8>(response[1]);

    result.slaveAddress = slaveAddr;
    result.functionCode = static_cast<ModbusFunctionCode>(funcCode);

    // 检查是否为异常响应（功能码最高位为1）
    if (funcCode & 0x80) {
        result.isException = true;
        quint8 exCode = static_cast<quint8>(response[2]);
        result.exceptionCode = static_cast<ModbusExceptionCode>(exCode);
        return result;
    }

    result.isException = false;

    // 根据功能码解析正常响应
    switch (result.functionCode) {
    case ModbusFunctionCode::ReadHoldingRegisters:
    case ModbusFunctionCode::ReadInputRegisters:
    case ModbusFunctionCode::ReadCoils:
    case ModbusFunctionCode::ReadDiscreteInputs: {
        // [地址][功能码][字节数][数据...][CRC低][CRC高]
        if (response.size() >= 4) {
            quint8 byteCount = static_cast<quint8>(response[2]);
            int dataStart = 3;
            int dataEnd = dataStart + byteCount;

            // 确保数据长度合理且不含 CRC
            int payloadLen = response.size() - 2; // 减去 CRC
            if (dataEnd <= payloadLen) {
                if (result.functionCode == ModbusFunctionCode::ReadHoldingRegisters ||
                    result.functionCode == ModbusFunctionCode::ReadInputRegisters) {
                    // 寄存器读取：每2字节一个寄存器值
                    for (int i = dataStart; i < dataEnd; i += 2) {
                        quint16 regValue =
                            (static_cast<quint16>(static_cast<unsigned char>(response[i])) << 8) |
                            static_cast<quint16>(static_cast<unsigned char>(response[i + 1]));
                        result.data.append(regValue);
                    }
                } else {
                    // 线圈/离散输入读取：每字节8个状态
                    for (int i = dataStart; i < dataEnd; ++i) {
                        quint8 byte = static_cast<quint8>(response[i]);
                        for (int bit = 0; bit < 8 && result.data.size() < request.quantity; ++bit) {
                            result.data.append((byte >> bit) & 0x01);
                        }
                    }
                }
            }
        }
        break;
    }

    case ModbusFunctionCode::WriteSingleRegister:
    case ModbusFunctionCode::WriteMultipleRegisters: {
        // 回显：[地址][功能码][地址高][地址低][值/数量高][值/数量低][CRC低][CRC高]
        if (response.size() >= 6) {
            quint16 addr =
                (static_cast<quint16>(static_cast<unsigned char>(response[2])) << 8) |
                static_cast<quint16>(static_cast<unsigned char>(response[3]));
            quint16 value =
                (static_cast<quint16>(static_cast<unsigned char>(response[4])) << 8) |
                static_cast<quint16>(static_cast<unsigned char>(response[5]));
            result.data.append(addr);
            result.data.append(value);
        }
        break;
    }

    case ModbusFunctionCode::WriteSingleCoil:
    case ModbusFunctionCode::WriteMultipleCoils: {
        if (response.size() >= 6) {
            quint16 addr =
                (static_cast<quint16>(static_cast<unsigned char>(response[2])) << 8) |
                static_cast<quint16>(static_cast<unsigned char>(response[3]));
            quint16 value =
                (static_cast<quint16>(static_cast<unsigned char>(response[4])) << 8) |
                static_cast<quint16>(static_cast<unsigned char>(response[5]));
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

ModbusResponse ModbusRtuProtocol::parseResponse(const QByteArray &response)
{
    ModbusRequest dummy;
    // 设置默认请求信息
    if (response.size() >= 2) {
        dummy.slaveAddress = static_cast<quint8>(response[0]);
        quint8 funcCode = static_cast<quint8>(response[1]) & 0x7F; // 去掉异常标志
        dummy.functionCode = static_cast<ModbusFunctionCode>(funcCode);
    }
    return parseResponse(response, dummy);
}

QString ModbusRtuProtocol::exceptionCodeToString(ModbusExceptionCode code)
{
    switch (code) {
    case ModbusExceptionCode::IllegalFunction:
        return QStringLiteral("非法功能码 (Illegal Function)");
    case ModbusExceptionCode::IllegalDataAddress:
        return QStringLiteral("非法数据地址 (Illegal Data Address)");
    case ModbusExceptionCode::IllegalDataValue:
        return QStringLiteral("非法数据值 (Illegal Data Value)");
    case ModbusExceptionCode::SlaveDeviceFailure:
        return QStringLiteral("从站设备故障 (Slave Device Failure)");
    case ModbusExceptionCode::Acknowledge:
        return QStringLiteral("确认 (Acknowledge)");
    case ModbusExceptionCode::SlaveDeviceBusy:
        return QStringLiteral("从站忙 (Slave Device Busy)");
    case ModbusExceptionCode::MemoryParityError:
        return QStringLiteral("内存奇偶校验错误 (Memory Parity Error)");
    case ModbusExceptionCode::GatewayPathUnavailable:
        return QStringLiteral("网关路径不可用 (Gateway Path Unavailable)");
    case ModbusExceptionCode::GatewayTargetFailed:
        return QStringLiteral("网关目标设备响应失败 (Gateway Target Failed)");
    default:
        return QStringLiteral("未知异常码 (Unknown Exception Code)");
    }
}

void ModbusRtuProtocol::appendCrc(QByteArray &frame)
{
    quint16 crc = Crc16::calculate(
        reinterpret_cast<const unsigned char *>(frame.constData()),
        frame.size());

    // CRC 低字节在前
    frame.append(static_cast<char>(crc & 0xFF));
    frame.append(static_cast<char>((crc >> 8) & 0xFF));
}

bool ModbusRtuProtocol::verifyCrc(const QByteArray &frame)
{
    if (frame.size() < 4) {
        return false;
    }

    // 除去最后 2 字节 CRC，计算前面数据的 CRC
    int dataLen = frame.size() - 2;
    quint16 calculatedCrc = Crc16::calculate(
        reinterpret_cast<const unsigned char *>(frame.constData()),
        dataLen);

    // 提取接收到的 CRC（低字节在前）
    quint16 receivedCrc =
        (static_cast<quint16>(static_cast<unsigned char>(frame[dataLen + 1])) << 8) |
        static_cast<quint16>(static_cast<unsigned char>(frame[dataLen]));

    return calculatedCrc == receivedCrc;
}
