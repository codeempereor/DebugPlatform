#include "protocol/binaryframebuilder.h"
#include "protocol/crc16.h"

QByteArray BinaryFrameBuilder::buildFrame(const QByteArray &payload)
{
    QByteArray frame;

    // 1. 添加帧头
    frame.append(static_cast<char>(0xAA));
    frame.append(static_cast<char>(0x55));

    // 2. 添加长度字段（小端序：低字节在前）
    quint16 length = calculateLength(payload.size());
    frame.append(static_cast<char>(length & 0xFF));        // 低字节
    frame.append(static_cast<char>((length >> 8) & 0xFF)); // 高字节

    // 3. 添加负载数据
    frame.append(payload);

    // 4. 计算 CRC16（覆盖长度字段和负载数据）
    // CRC 计算起始：帧头之后，从长度字段开始
    const unsigned char *crcDataStart =
        reinterpret_cast<const unsigned char *>(frame.constData() + 2);
    const int crcDataLength = 2 + payload.size(); // 长度字段 + 负载数据

    quint16 crc = Crc16::calculate(crcDataStart, crcDataLength);

    // 5. 添加 CRC（小端序：低字节在前）
    frame.append(static_cast<char>(crc & 0xFF));        // 低字节
    frame.append(static_cast<char>((crc >> 8) & 0xFF)); // 高字节

    // 6. 添加帧尾
    frame.append(static_cast<char>(0x0D));
    frame.append(static_cast<char>(0x0A));

    return frame;
}

quint16 BinaryFrameBuilder::calculateLength(int payloadSize)
{
    // 长度字段值即为负载数据大小
    if (payloadSize < 0 || payloadSize > 65535) {
        return 0;
    }
    return static_cast<quint16>(payloadSize);
}
