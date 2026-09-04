#ifndef BINARYFRAMEPARSER_H
#define BINARYFRAMEPARSER_H

#include <QObject>
#include <QByteArray>
#include <QQueue>
#include <QString>
#include <QtGlobal>

class BinaryFrameParser : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 帧解析状态枚举
     *
     * 状态机逐字节解析二进制帧：
     * 帧格式：[0xAA][0x55][长度(2B)][负载数据][CRC16(2B)][0x0D][0x0A]
     */
    enum class State {
        WAIT_HEADER_1,   ///< 等待帧头第一字节 0xAA
        WAIT_HEADER_2,   ///< 等待帧头第二字节 0x55
        WAIT_LENGTH,     ///< 等待长度字段（2 字节）
        WAIT_DATA,       ///< 等待负载数据
        WAIT_CRC,        ///< 等待 CRC16 校验（2 字节）
        WAIT_TAIL,       ///< 等待帧尾 0x0D 0x0A
        COMPLETE         ///< 帧接收完成（在此状态验证 CRC 和帧尾）
    };

    explicit BinaryFrameParser(QObject *parent = nullptr);
    ~BinaryFrameParser() override;

    /**
     * @brief 喂入单个字节
     * @param byte 输入字节
     */
    void feedByte(unsigned char byte);

    /**
     * @brief 喂入字节数组（处理粘包和半包）
     * @param data 输入数据
     */
    void feedData(const QByteArray &data);

    /**
     * @brief 重置解析器状态
     */
    void reset();

    /**
     * @brief 获取当前状态
     * @return 当前解析状态
     */
    State currentState() const;

signals:
    /**
     * @brief 完整帧接收信号
     * @param payload 负载数据（不含帧头、长度、CRC、帧尾）
     */
    void frameReceived(const QByteArray &payload);

    /**
     * @brief 解析错误信号
     * @param reason 错误原因描述
     */
    void parseError(const QString &reason);

private:
    /**
     * @brief 处理状态转换
     */
    void transitionTo(State newState);

    /**
     * @brief 验证完整帧的 CRC 和帧尾
     * @return true 验证通过
     */
    bool verifyFrame();

    State m_state;              ///< 当前解析状态
    QByteArray m_buffer;        ///< 当前帧接收缓冲区
    quint16 m_expectedLength;   ///< 预期负载长度
    int m_lengthByteCount;      ///< 已接收的长度字节数（0-2）
    int m_crcByteCount;         ///< 已接收的 CRC 字节数（0-2）
    int m_dataByteCount;
    int m_tailByteCount;        ///< 已接收的数据字节数

    static constexpr unsigned char HEADER_1 = 0xAA;
    static constexpr unsigned char HEADER_2 = 0x55;
    static constexpr unsigned char TAIL_1   = 0x0D;
    static constexpr unsigned char TAIL_2   = 0x0A;
    static constexpr int MAX_FRAME_SIZE = 4096; ///< 最大帧大小限制
};

#endif // BINARYFRAMEPARSER_H
