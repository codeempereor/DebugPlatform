#ifndef DEBUGPLATFORM_PACKETPARSERHIGHLIGHTER_H
#define DEBUGPLATFORM_PACKETPARSERHIGHLIGHTER_H

/**
 * @file packetparserhighlighter.h
 * @brief 日志报文解析高亮工具
 *
 * 纯工具类（非 QObject），提供：
 *  - 从日志消息中提取 hex 段
 *  - 自动探测协议（BinaryFrame / ModbusTcp / ModbusRtu / Unknown）
 *  - 按字段切分并生成彩色 HTML（字节区）+ 纯文本字段表
 *
 * 复用 Crc16::calculate 进行 Modbus RTU 校验探测。
 */

#include <QByteArray>
#include <QString>
#include <QVector>
#include <QStringList>

class PacketParserHighlighter
{
public:
    /// 协议类型
    enum PacketProto {
        Unknown = 0,
        ModbusRtu,
        ModbusTcp,
        BinaryFrame
    };

    /// 字段描述行
    struct FieldRow {
        QString fieldName;   ///< 字段名（如 "SlaveAddr"）
        int     offset;      ///< 在原始报文中的字节偏移
        int     length;      ///< 字节长度
        QString hex;         ///< 十六进制表示
        QString dec;         ///< 十进制/含义
        bool    valid;       ///< 校验是否通过
    };

    PacketParserHighlighter() = delete;  ///< 纯静态工具类

    // ---- 协议探测 ----

    /// 从带空格的 hex 字符串解析出原始字节；失败返回空 QByteArray
    static QByteArray hexStringToBytes(const QString &hexWithSpaces);

    /// 从原始字节探测协议类型
    static PacketProto detectProto(const QByteArray &raw);

    /// 从 hex 字符串一步到位：解析字节 + 探测协议
    /// 若 outRaw 非空，写入解析后的字节
    static PacketProto detectFromString(const QString &hexWithSpaces,
                                        QByteArray *outRaw = nullptr);

    // ---- 字段切分 ----

    /// 按协议切分字段表
    static QVector<FieldRow> parseFieldTable(const QByteArray &raw,
                                              PacketProto proto);

    // ---- 着色渲染 ----

    /// 字节区着色 HTML：每个字节用 <span style="background:#XXXXXX"> 包裹
    static QString formatBytesAsHtml(const QByteArray &raw, PacketProto proto);

    /// 字段表纯文本（对齐 3 列：字段名 | Hex@偏移 | 值/说明）
    static QString formatFieldTableAsText(const QVector<FieldRow> &rows);

    /// 协议名
    static QString protoName(PacketProto proto);

private:
    /// 单个字节段的背景色
    static QString colorForField(const QString &fieldName);

    /// Modbus RTU 字段切分
    static QVector<FieldRow> parseRtuFields(const QByteArray &raw);

    /// Modbus TCP 字段切分
    static QVector<FieldRow> parseTcpFields(const QByteArray &raw);

    /// BinaryFrame 字段切分
    static QVector<FieldRow> parseBinaryFrameFields(const QByteArray &raw);

    /// 格式化单字节为 "XX" 大写
    static QString byteHex(quint8 b);
};

#endif // DEBUGPLATFORM_PACKETPARSERHIGHLIGHTER_H
