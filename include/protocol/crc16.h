#ifndef CRC16_H
#define CRC16_H

#include <QByteArray>
#include <QtGlobal>

namespace Crc16 {

/**
 * @brief 使用 Modbus 标准多项式 (0xA001) 计算 CRC16 校验值
 *
 * 使用预计算的 256 项查找表优化性能。
 * 默认种子值为 0xFFFF（Modbus 标准初始值）。
 */

/**
 * @brief 从 QByteArray 计算 CRC16
 * @param data 待计算的数据
 * @param seed 初始种子值，默认为 0xFFFF
 * @return 16 位 CRC 校验值
 */
quint16 calculate(const QByteArray &data, quint16 seed = 0xFFFF);

/**
 * @brief 从原始字节指针计算 CRC16
 * @param data 字节数据指针
 * @param length 数据长度（字节数）
 * @param seed 初始种子值，默认为 0xFFFF
 * @return 16 位 CRC 校验值
 */
quint16 calculate(const unsigned char *data, int length, quint16 seed = 0xFFFF);

} // namespace Crc16

#endif // CRC16_H
