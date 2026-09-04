#ifndef BINARYFRAMEBUILDER_H
#define BINARYFRAMEBUILDER_H

#include <QByteArray>
#include <QtGlobal>

/**
 * @brief 二进制帧组包器
 *
 * 用于将负载数据组装为完整的二进制通信帧。
 * 帧格式：[0xAA][0x55][长度(2B)][负载数据][CRC16(2B)][0x0D][0x0A]
 */
class BinaryFrameBuilder
{
public:
    /**
     * @brief 根据负载数据构建完整帧
     * @param payload 负载数据
     * @return 完整的二进制帧
     */
    static QByteArray buildFrame(const QByteArray &payload);

    /**
     * @brief 计算帧长度字段值
     * @param payloadSize 负载数据大小
     * @return 长度字段值（负载大小）
     */
    static quint16 calculateLength(int payloadSize);

private:
    BinaryFrameBuilder() = delete;  // 静态工具类，禁止实例化
};

#endif // BINARYFRAMEBUILDER_H
