#include "protocol/binaryframeparser.h"
#include "protocol/crc16.h"

BinaryFrameParser::BinaryFrameParser(QObject *parent)
    : QObject(parent)
    , m_state(State::WAIT_HEADER_1)
    , m_expectedLength(0)
    , m_lengthByteCount(0)
    , m_crcByteCount(0)
    , m_dataByteCount(0)
{
}

BinaryFrameParser::~BinaryFrameParser()
{
}

void BinaryFrameParser::feedByte(unsigned char byte)
{
    switch (m_state) {
    case State::WAIT_HEADER_1:
        if (byte == HEADER_1) {
            m_buffer.clear();
            m_buffer.append(static_cast<char>(byte));
            transitionTo(State::WAIT_HEADER_2);
        }
        break;

    case State::WAIT_HEADER_2:
        if (byte == HEADER_2) {
            m_buffer.append(static_cast<char>(byte));
            transitionTo(State::WAIT_LENGTH);
            m_lengthByteCount = 0;
        } else if (byte == HEADER_1) {
            // 重新同步：可能是粘包中的下一帧起始
            m_buffer.clear();
            m_buffer.append(static_cast<char>(byte));
            // 保持在 WAIT_HEADER_2 状态，等待下一字节
        } else {
            emit parseError(QStringLiteral("Invalid header: expected 0x55, got 0x%1")
                                .arg(byte, 2, 16, QLatin1Char('0')));
            reset();
        }
        break;

    case State::WAIT_LENGTH:
        m_buffer.append(static_cast<char>(byte));
        m_lengthByteCount++;
        if (m_lengthByteCount >= 2) {
            // 解析长度字段（小端序：低字节在前）
            m_expectedLength = static_cast<quint16>(
                (static_cast<unsigned char>(m_buffer[3]) << 8) |
                 static_cast<unsigned char>(m_buffer[2]));

            if (m_expectedLength > MAX_FRAME_SIZE) {
                emit parseError(QStringLiteral("Frame length %1 exceeds maximum %2")
                                    .arg(m_expectedLength)
                                    .arg(MAX_FRAME_SIZE));
                reset();
                return;
            }

            m_dataByteCount = 0;
            if (m_expectedLength == 0) {
                // 零长度帧，跳过数据阶段
                transitionTo(State::WAIT_CRC);
                m_crcByteCount = 0;
            } else {
                transitionTo(State::WAIT_DATA);
            }
        }
        break;

    case State::WAIT_DATA:
        m_buffer.append(static_cast<char>(byte));
        m_dataByteCount++;
        if (m_dataByteCount >= m_expectedLength) {
            transitionTo(State::WAIT_CRC);
            m_crcByteCount = 0;
        }
        break;

    case State::WAIT_CRC:
        m_buffer.append(static_cast<char>(byte));
        m_crcByteCount++;
        if (m_crcByteCount >= 2) {
            // CRC 接收完成，等待帧尾
            transitionTo(State::WAIT_TAIL);
            // 检查第二个 CRC 字节是否与 TAIL_1 相同
            // 如果相同，则当前字节已经是 TAIL_1，只需再等 TAIL_2
            // 否则需要等待两个字节：TAIL_1 和 TAIL_2
            if (byte == TAIL_1) {
                // CRC_MSB == TAIL_1，已经收到 TAIL_1，等待 TAIL_2
                m_tailByteCount = 1;
            } else {
                // CRC_MSB != TAIL_1，等待 TAIL_1
                m_tailByteCount = 0;
            }
        }
        break;

    case State::WAIT_TAIL:
        m_buffer.append(static_cast<char>(byte));
        m_tailByteCount++;
        
        if (m_tailByteCount == 1) {
            // 期望接收 TAIL_1 (0x0D)
            if (byte != TAIL_1) {
                emit parseError(QStringLiteral("Invalid frame tail: expected 0x0D, got 0x%1")
                                    .arg(byte, 2, 16, QLatin1Char('0')));
                reset();
                // 检查是否是新帧起始
                if (byte == HEADER_1) {
                    m_buffer.clear();
                    m_buffer.append(static_cast<char>(byte));
                    transitionTo(State::WAIT_HEADER_2);
                }
                return;
            }
            // 继续等待 TAIL_2
        } else if (m_tailByteCount >= 2) {
            // 期望接收 TAIL_2 (0x0A)
            if (byte == TAIL_2) {
                // 完整帧接收完成，验证 CRC 和帧尾
                if (verifyFrame()) {
                    emit frameReceived(m_buffer.mid(4, m_expectedLength));
                } else {
                    emit parseError(QStringLiteral("CRC verification failed"));
                }
            } else {
                emit parseError(QStringLiteral("Invalid frame tail: expected 0x0A, got 0x%1")
                                    .arg(byte, 2, 16, QLatin1Char('0')));
            }
            reset();
        }
        break;

    case State::COMPLETE:
        // 不应到达此状态，直接忽略
        break;
    }
}

void BinaryFrameParser::feedData(const QByteArray &data)
{
    for (int i = 0; i < data.size(); ++i) {
        feedByte(static_cast<unsigned char>(data[i]));
    }
}

void BinaryFrameParser::reset()
{
    m_state = State::WAIT_HEADER_1;
    m_buffer.clear();
    m_expectedLength = 0;
    m_lengthByteCount = 0;
    m_crcByteCount = 0;
    m_dataByteCount = 0;
    m_tailByteCount = 0;
}

BinaryFrameParser::State BinaryFrameParser::currentState() const
{
    return m_state;
}

void BinaryFrameParser::transitionTo(State newState)
{
    m_state = newState;
}

bool BinaryFrameParser::verifyFrame()
{
    // 帧结构：[HEADER1][HEADER2][LEN_LSB][LEN_MSB][...DATA...][CRC_LSB][CRC_MSB][TAIL1][TAIL2]
    // 最小帧（零数据）：头(2) + 长度(2) + 数据(0) + CRC(2) + 尾(2) = 8 字节
    const int minFrameSize = 8;
    if (m_buffer.size() < minFrameSize) {
        return false;
    }

    // 验证帧头
    if (static_cast<unsigned char>(m_buffer[0]) != HEADER_1 ||
        static_cast<unsigned char>(m_buffer[1]) != HEADER_2) {
        return false;
    }

    // 验证帧尾
    int tailOffset = 4 + m_expectedLength + 2; // 头(4) + 数据 + CRC(2)
    if (m_buffer.size() >= tailOffset + 2) {
        if (static_cast<unsigned char>(m_buffer[tailOffset]) != TAIL_1 ||
            static_cast<unsigned char>(m_buffer[tailOffset + 1]) != TAIL_2) {
            return false;
        }
    }

    // 提取 CRC 校验字段（位于数据之后、帧尾之前）
    const int crcOffset = 4 + m_expectedLength; // 跳过头部4字节 + 数据
    if (m_buffer.size() < crcOffset + 2) {
        return false;
    }

    // 计算从长度字段开始到数据结束的 CRC
    // 注意：CRC 覆盖的是长度字段和数据，不含帧头
    const unsigned char *crcDataStart =
        reinterpret_cast<const unsigned char *>(m_buffer.constData() + 2);
    const int crcDataLength = 2 + m_expectedLength; // 长度字段 + 数据

    quint16 calculatedCrc = Crc16::calculate(crcDataStart, crcDataLength);

    // 提取接收到的 CRC
    quint16 receivedCrc = static_cast<quint16>(
        (static_cast<unsigned char>(m_buffer[crcOffset + 1]) << 8) |
         static_cast<unsigned char>(m_buffer[crcOffset]));

    return calculatedCrc == receivedCrc;
}