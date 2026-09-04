#include "ui/packetparserhighlighter.h"
#include "protocol/crc16.h"
#include <QStringList>
#include <QRegularExpression>

// ============================================================================
// 工具
// ============================================================================

QString PacketParserHighlighter::byteHex(quint8 b)
{
    return QString::number(b, 16).toUpper().rightJustified(2, '0');
}

QString PacketParserHighlighter::protoName(PacketProto proto)
{
    switch (proto) {
    case BinaryFrame: return QStringLiteral("BinaryFrame");
    case ModbusTcp:   return QStringLiteral("ModbusTcp");
    case ModbusRtu:   return QStringLiteral("ModbusRtu");
    case Unknown:     return QStringLiteral("Unknown");
    }
    return QStringLiteral("Unknown");
}

QString PacketParserHighlighter::colorForField(const QString &fieldName)
{
    // 8 色浅背景
    if (fieldName == "Header" || fieldName == "MBAP")            return "#FFF3B0"; // 浅黄
    if (fieldName == "Length")                                    return "#E8DAEF"; // 浅紫
    if (fieldName == "SlaveAddr" || fieldName == "UnitId" ||
        fieldName == "StartAddr" || fieldName == "Addr")          return "#DAE8FC"; // 浅蓝
    if (fieldName == "FunctionCode")                             return "#D5E8D4"; // 浅绿
    if (fieldName == "Data" || fieldName == "Payload" ||
        fieldName == "RegValues" || fieldName == "ByteCount" ||
        fieldName == "Value" || fieldName == "Quantity")          return "#D1F2EB"; // 浅青
    if (fieldName == "CRC16")                                    return "#FADBD8"; // 浅粉
    if (fieldName == "Tail")                                      return "#FDEBD0"; // 浅橙
    if (fieldName == "ExceptionCode")                             return "#F5B7B1"; // 浅红
    return "#F2F2F2"; // 默认浅灰
}

QByteArray PacketParserHighlighter::hexStringToBytes(const QString &hexWithSpaces)
{
    // 抽取连续的 2 字符 hex 段
    // 正则：(^|\D)([0-9A-Fa-f]{2}(\s+[0-9A-Fa-f]{2})+)(\D|$)
    static QRegularExpression re(QStringLiteral("(?:\\s|^)((?:[0-9A-Fa-f]{2}\\s*){2,})(?:\\s|$)"));
    QRegularExpressionMatch match = re.match(hexWithSpaces);
    if (!match.hasMatch()) return QByteArray();

    QString hexSeg = match.captured(1).trimmed();
    QStringList tokens = hexSeg.split(QRegularExpression(QStringLiteral("\\s+")),
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                                       Qt::SkipEmptyParts);
#else
                                       QString::SkipEmptyParts);
#endif
    QByteArray result;
    result.reserve(tokens.size());
    for (const QString &t : tokens) {
        bool ok = false;
        quint8 b = static_cast<quint8>(t.toUInt(&ok, 16));
        if (!ok) return QByteArray();
        result.append(static_cast<char>(b));
    }
    return result;
}

// ============================================================================
// 协议探测
// ============================================================================

PacketParserHighlighter::PacketProto PacketParserHighlighter::detectProto(const QByteArray &raw)
{
    const int n = raw.size();

    // a) BinaryFrame: size >= 9, 首字节 0xAA 0x55, 末字节 0x0D 0x0A
    if (n >= 9) {
        if ((quint8)raw[0] == 0xAA && (quint8)raw[1] == 0x55 &&
            (quint8)raw[n-2] == 0x0D && (quint8)raw[n-1] == 0x0A) {
            return BinaryFrame;
        }
    }

    // b) ModbusRtu: size >= 5, CRC16 校验通过（先于 TCP 检测，
    //    因为 8 字节 RTU 读请求帧可能误满足 TCP 判据 PID=0）
    if (n >= 5) {
        quint16 crcCalc = Crc16::calculate(raw.left(n - 2));
        quint16 crcIn = (quint8)raw[n-2] | ((quint8)raw[n-1] << 8);
        if (crcCalc == crcIn) {
            return ModbusRtu;
        }
    }

    // c) ModbusTcp: size >= 8, PID=0x0000, Length 合理, 6+Length == size
    if (n >= 8) {
        bool pidZero = ((quint8)raw[2] == 0x00 && (quint8)raw[3] == 0x00);
        int lenField = ((quint8)raw[4] << 8) | (quint8)raw[5];
        if (pidZero && lenField >= 1 && lenField <= 254 && (6 + lenField) == n) {
            return ModbusTcp;
        }
    }

    return Unknown;
}

PacketParserHighlighter::PacketProto PacketParserHighlighter::detectFromString(
    const QString &hexWithSpaces, QByteArray *outRaw)
{
    QByteArray raw = hexStringToBytes(hexWithSpaces);
    if (outRaw) *outRaw = raw;
    if (raw.isEmpty()) return Unknown;
    return detectProto(raw);
}

// ============================================================================
// 字段切分
// ============================================================================

QVector<PacketParserHighlighter::FieldRow>
PacketParserHighlighter::parseFieldTable(const QByteArray &raw, PacketProto proto)
{
    switch (proto) {
    case ModbusRtu:     return parseRtuFields(raw);
    case ModbusTcp:     return parseTcpFields(raw);
    case BinaryFrame:   return parseBinaryFrameFields(raw);
    case Unknown:       return {};
    }
    return {};
}

QVector<PacketParserHighlighter::FieldRow>
PacketParserHighlighter::parseRtuFields(const QByteArray &raw)
{
    QVector<FieldRow> rows;
    const int n = raw.size();
    if (n < 5) return rows;

    int idx = 0;
    // SlaveAddr(1)
    {
        FieldRow r;
        r.fieldName = "SlaveAddr";
        r.offset = idx; r.length = 1;
        r.hex = byteHex((quint8)raw[idx]);
        r.dec = QString::number((quint8)raw[idx]);
        r.valid = true;
        rows.append(r);
        idx += 1;
    }

    // FunctionCode(1)
    quint8 fc = (quint8)raw[idx];
    bool isException = (fc & 0x80);
    {
        FieldRow r;
        r.fieldName = "FunctionCode";
        r.offset = idx; r.length = 1;
        r.hex = byteHex(fc);
        if (isException) {
            r.dec = QString("Exception 0x%1").arg(fc & 0x7F, 2, 16, QChar('0')).toUpper();
        } else {
            r.dec = QString("0x%1").arg(fc, 2, 16, QChar('0')).toUpper();
        }
        r.valid = true;
        rows.append(r);
        idx += 1;
    }

    // 按 FC 分支（PDU 部分，不含末尾 CRC）
    int pduEnd = n - 2; // CRC 占末 2 字节
    if (isException) {
        // 异常响应: ExceptionCode(1) + CRC
        FieldRow r;
        r.fieldName = "ExceptionCode";
        r.offset = idx; r.length = 1;
        r.hex = byteHex((quint8)raw[idx]);
        r.dec = QString("0x%1").arg((quint8)raw[idx], 2, 16, QChar('0')).toUpper();
        r.valid = true;
        rows.append(r);
        idx += 1;
    } else if (fc == 0x03 || fc == 0x04 || fc == 0x02 || fc == 0x01) {
        // 读请求 vs 读响应区分：请求 n==8, 响应 n > 8
        if (n == 8) {
            // 请求: StartAddr(2) + Quantity(2)
            {
                FieldRow r;
                r.fieldName = "StartAddr";
                r.offset = idx; r.length = 2;
                r.hex = byteHex((quint8)raw[idx]) + " " + byteHex((quint8)raw[idx+1]);
                r.dec = QString::number(((quint8)raw[idx] << 8) | (quint8)raw[idx+1]);
                r.valid = true;
                rows.append(r);
                idx += 2;
            }
            {
                FieldRow r;
                r.fieldName = "Quantity";
                r.offset = idx; r.length = 2;
                r.hex = byteHex((quint8)raw[idx]) + " " + byteHex((quint8)raw[idx+1]);
                r.dec = QString::number(((quint8)raw[idx] << 8) | (quint8)raw[idx+1]);
                r.valid = true;
                rows.append(r);
                idx += 2;
            }
        } else {
            // 响应: ByteCount(1) + Data(N)
            int byteCount = (quint8)raw[idx];
            {
                FieldRow r;
                r.fieldName = "ByteCount";
                r.offset = idx; r.length = 1;
                r.hex = byteHex((quint8)raw[idx]);
                r.dec = QString::number(byteCount);
                r.valid = true;
                rows.append(r);
                idx += 1;
            }
            int dataLen = pduEnd - idx;
            if (dataLen > 0) {
                FieldRow r;
                r.fieldName = "Data";
                r.offset = idx; r.length = dataLen;
                QString h;
                for (int i = 0; i < dataLen; ++i) {
                    if (i > 0) h += " ";
                    h += byteHex((quint8)raw[idx + i]);
                }
                r.hex = h;
                r.dec = QString("%1 reg(s)").arg(dataLen / 2);
                r.valid = true;
                rows.append(r);
                idx += dataLen;
            }
        }
    } else if (fc == 0x06) {
        // 写单寄存器: Addr(2) + Value(2)
        {
            FieldRow r;
            r.fieldName = "Addr";
            r.offset = idx; r.length = 2;
            r.hex = byteHex((quint8)raw[idx]) + " " + byteHex((quint8)raw[idx+1]);
            r.dec = QString::number(((quint8)raw[idx] << 8) | (quint8)raw[idx+1]);
            r.valid = true;
            rows.append(r);
            idx += 2;
        }
        {
            FieldRow r;
            r.fieldName = "Value";
            r.offset = idx; r.length = 2;
            r.hex = byteHex((quint8)raw[idx]) + " " + byteHex((quint8)raw[idx+1]);
            r.dec = QString::number(((quint8)raw[idx] << 8) | (quint8)raw[idx+1]);
            r.valid = true;
            rows.append(r);
            idx += 2;
        }
    } else if (fc == 0x10) {
        // 写多寄存器: 请求 vs 响应
        if (n == 8) {
            // 响应: StartAddr(2) + Quantity(2)
            {
                FieldRow r;
                r.fieldName = "StartAddr";
                r.offset = idx; r.length = 2;
                r.hex = byteHex((quint8)raw[idx]) + " " + byteHex((quint8)raw[idx+1]);
                r.dec = QString::number(((quint8)raw[idx] << 8) | (quint8)raw[idx+1]);
                r.valid = true;
                rows.append(r);
                idx += 2;
            }
            {
                FieldRow r;
                r.fieldName = "Quantity";
                r.offset = idx; r.length = 2;
                r.hex = byteHex((quint8)raw[idx]) + " " + byteHex((quint8)raw[idx+1]);
                r.dec = QString::number(((quint8)raw[idx] << 8) | (quint8)raw[idx+1]);
                r.valid = true;
                rows.append(r);
                idx += 2;
            }
        } else {
            // 请求: StartAddr(2) + Quantity(2) + ByteCount(1) + Data(N)
            {
                FieldRow r;
                r.fieldName = "StartAddr";
                r.offset = idx; r.length = 2;
                r.hex = byteHex((quint8)raw[idx]) + " " + byteHex((quint8)raw[idx+1]);
                r.dec = QString::number(((quint8)raw[idx] << 8) | (quint8)raw[idx+1]);
                r.valid = true;
                rows.append(r);
                idx += 2;
            }
            {
                FieldRow r;
                r.fieldName = "Quantity";
                r.offset = idx; r.length = 2;
                r.hex = byteHex((quint8)raw[idx]) + " " + byteHex((quint8)raw[idx+1]);
                r.dec = QString::number(((quint8)raw[idx] << 8) | (quint8)raw[idx+1]);
                r.valid = true;
                rows.append(r);
                idx += 2;
            }
            int byteCount = (quint8)raw[idx];
            {
                FieldRow r;
                r.fieldName = "ByteCount";
                r.offset = idx; r.length = 1;
                r.hex = byteHex((quint8)raw[idx]);
                r.dec = QString::number(byteCount);
                r.valid = true;
                rows.append(r);
                idx += 1;
            }
            int dataLen = pduEnd - idx;
            if (dataLen > 0) {
                FieldRow r;
                r.fieldName = "RegValues";
                r.offset = idx; r.length = dataLen;
                QString h;
                for (int i = 0; i < dataLen; ++i) {
                    if (i > 0) h += " ";
                    h += byteHex((quint8)raw[idx + i]);
                }
                r.hex = h;
                r.dec = QString("%1 reg(s)").arg(dataLen / 2);
                r.valid = true;
                rows.append(r);
                idx += dataLen;
            }
        }
    }

    // CRC16(2)
    {
        FieldRow r;
        r.fieldName = "CRC16";
        r.offset = n - 2; r.length = 2;
        r.hex = byteHex((quint8)raw[n-2]) + " " + byteHex((quint8)raw[n-1]);
        quint16 crcCalc = Crc16::calculate(raw.left(n - 2));
        quint16 crcIn = (quint8)raw[n-2] | ((quint8)raw[n-1] << 8);
        r.dec = (crcCalc == crcIn) ? "OK" : "MISMATCH";
        r.valid = (crcCalc == crcIn);
        rows.append(r);
    }

    return rows;
}

QVector<PacketParserHighlighter::FieldRow>
PacketParserHighlighter::parseTcpFields(const QByteArray &raw)
{
    QVector<FieldRow> rows;
    const int n = raw.size();
    if (n < 8) return rows;

    int idx = 0;
    // MBAP 7 字节
    {
        FieldRow r;
        r.fieldName = "MBAP";
        r.offset = 0; r.length = 7;
        QString h;
        for (int i = 0; i < 7; ++i) {
            if (i > 0) h += " ";
            h += byteHex((quint8)raw[i]);
        }
        r.hex = h;
        r.dec = "TransactionId+ProtocolId+Length+UnitId";
        r.valid = true;
        rows.append(r);
        idx = 7;
    }
    // 拆细: TID(2) + PID(2) + Length(2) + UnitId(1)
    {
        FieldRow r;
        r.fieldName = "TransactionId";
        r.offset = 0; r.length = 2;
        r.hex = byteHex((quint8)raw[0]) + " " + byteHex((quint8)raw[1]);
        r.dec = QString::number(((quint8)raw[0] << 8) | (quint8)raw[1]);
        r.valid = true;
        rows.append(r);
    }
    {
        FieldRow r;
        r.fieldName = "ProtocolId";
        r.offset = 2; r.length = 2;
        r.hex = byteHex((quint8)raw[2]) + " " + byteHex((quint8)raw[3]);
        r.dec = QString::number(((quint8)raw[2] << 8) | (quint8)raw[3]);
        r.valid = true;
        rows.append(r);
    }
    {
        FieldRow r;
        r.fieldName = "Length";
        r.offset = 4; r.length = 2;
        r.hex = byteHex((quint8)raw[4]) + " " + byteHex((quint8)raw[5]);
        int len = ((quint8)raw[4] << 8) | (quint8)raw[5];
        r.dec = QString::number(len);
        r.valid = (6 + len == n);
        rows.append(r);
    }
    {
        FieldRow r;
        r.fieldName = "UnitId";
        r.offset = 6; r.length = 1;
        r.hex = byteHex((quint8)raw[6]);
        r.dec = QString::number((quint8)raw[6]);
        r.valid = true;
        rows.append(r);
    }

    // PDU: FC(1) + ...
    quint8 fc = (quint8)raw[7];
    bool isException = (fc & 0x80);
    {
        FieldRow r;
        r.fieldName = "FunctionCode";
        r.offset = 7; r.length = 1;
        r.hex = byteHex(fc);
        r.dec = isException ? QString("Exception 0x%1").arg(fc & 0x7F, 2, 16, QChar('0')).toUpper()
                            : QString("0x%1").arg(fc, 2, 16, QChar('0')).toUpper();
        r.valid = true;
        rows.append(r);
        idx = 8;
    }

    int pduEnd = n; // TCP 无 CRC
    if (isException) {
        FieldRow r;
        r.fieldName = "ExceptionCode";
        r.offset = idx; r.length = 1;
        r.hex = byteHex((quint8)raw[idx]);
        r.dec = QString("0x%1").arg((quint8)raw[idx], 2, 16, QChar('0')).toUpper();
        r.valid = true;
        rows.append(r);
    } else if (fc == 0x03 || fc == 0x04 || fc == 0x02 || fc == 0x01) {
        if (n == 12) {
            // 请求: StartAddr(2) + Quantity(2)
            {
                FieldRow r;
                r.fieldName = "StartAddr";
                r.offset = idx; r.length = 2;
                r.hex = byteHex((quint8)raw[idx]) + " " + byteHex((quint8)raw[idx+1]);
                r.dec = QString::number(((quint8)raw[idx] << 8) | (quint8)raw[idx+1]);
                r.valid = true;
                rows.append(r);
                idx += 2;
            }
            {
                FieldRow r;
                r.fieldName = "Quantity";
                r.offset = idx; r.length = 2;
                r.hex = byteHex((quint8)raw[idx]) + " " + byteHex((quint8)raw[idx+1]);
                r.dec = QString::number(((quint8)raw[idx] << 8) | (quint8)raw[idx+1]);
                r.valid = true;
                rows.append(r);
            }
        } else {
            // 响应: ByteCount(1) + Data(N)
            int byteCount = (quint8)raw[idx];
            {
                FieldRow r;
                r.fieldName = "ByteCount";
                r.offset = idx; r.length = 1;
                r.hex = byteHex((quint8)raw[idx]);
                r.dec = QString::number(byteCount);
                r.valid = true;
                rows.append(r);
                idx += 1;
            }
            int dataLen = pduEnd - idx;
            if (dataLen > 0) {
                FieldRow r;
                r.fieldName = "Data";
                r.offset = idx; r.length = dataLen;
                QString h;
                for (int i = 0; i < dataLen; ++i) {
                    if (i > 0) h += " ";
                    h += byteHex((quint8)raw[idx + i]);
                }
                r.hex = h;
                r.dec = QString("%1 reg(s)").arg(dataLen / 2);
                r.valid = true;
                rows.append(r);
            }
        }
    } else if (fc == 0x06) {
        // 写单寄存器
        {
            FieldRow r;
            r.fieldName = "Addr";
            r.offset = idx; r.length = 2;
            r.hex = byteHex((quint8)raw[idx]) + " " + byteHex((quint8)raw[idx+1]);
            r.dec = QString::number(((quint8)raw[idx] << 8) | (quint8)raw[idx+1]);
            r.valid = true;
            rows.append(r);
            idx += 2;
        }
        {
            FieldRow r;
            r.fieldName = "Value";
            r.offset = idx; r.length = 2;
            r.hex = byteHex((quint8)raw[idx]) + " " + byteHex((quint8)raw[idx+1]);
            r.dec = QString::number(((quint8)raw[idx] << 8) | (quint8)raw[idx+1]);
            r.valid = true;
            rows.append(r);
        }
    }

    return rows;
}

QVector<PacketParserHighlighter::FieldRow>
PacketParserHighlighter::parseBinaryFrameFields(const QByteArray &raw)
{
    QVector<FieldRow> rows;
    const int n = raw.size();
    if (n < 9) return rows;

    int idx = 0;
    // Header(2): 0xAA 0x55
    {
        FieldRow r;
        r.fieldName = "Header";
        r.offset = 0; r.length = 2;
        r.hex = byteHex((quint8)raw[0]) + " " + byteHex((quint8)raw[1]);
        r.dec = "0xAA 0x55";
        r.valid = ((quint8)raw[0] == 0xAA && (quint8)raw[1] == 0x55);
        rows.append(r);
        idx = 2;
    }
    // Length(2, 小端)
    int length = (quint8)raw[2] | ((quint8)raw[3] << 8);
    {
        FieldRow r;
        r.fieldName = "Length";
        r.offset = 2; r.length = 2;
        r.hex = byteHex((quint8)raw[2]) + " " + byteHex((quint8)raw[3]);
        r.dec = QString::number(length);
        r.valid = true;
        rows.append(r);
        idx = 4;
    }
    // Payload(N)
    int payloadLen = n - 4 - 2 - 2; // 减 Header(2) Length(2) CRC(2) Tail(2)
    if (payloadLen > 0) {
        FieldRow r;
        r.fieldName = "Payload";
        r.offset = idx; r.length = payloadLen;
        QString h;
        for (int i = 0; i < payloadLen; ++i) {
            if (i > 0) h += " ";
            h += byteHex((quint8)raw[idx + i]);
        }
        r.hex = h;
        r.dec = QString("%1 bytes").arg(payloadLen);
        r.valid = (payloadLen == length);
        rows.append(r);
        idx += payloadLen;
    }
    // CRC16(2)
    {
        FieldRow r;
        r.fieldName = "CRC16";
        r.offset = n - 4; r.length = 2;
        r.hex = byteHex((quint8)raw[n-4]) + " " + byteHex((quint8)raw[n-3]);
        quint16 crcCalc = Crc16::calculate(raw.mid(4, payloadLen));
        quint16 crcIn = (quint8)raw[n-4] | ((quint8)raw[n-3] << 8);
        r.dec = (crcCalc == crcIn) ? "OK" : "MISMATCH";
        r.valid = (crcCalc == crcIn);
        rows.append(r);
    }
    // Tail(2): 0x0D 0x0A
    {
        FieldRow r;
        r.fieldName = "Tail";
        r.offset = n - 2; r.length = 2;
        r.hex = byteHex((quint8)raw[n-2]) + " " + byteHex((quint8)raw[n-1]);
        r.dec = "0x0D 0x0A";
        r.valid = ((quint8)raw[n-2] == 0x0D && (quint8)raw[n-1] == 0x0A);
        rows.append(r);
    }

    return rows;
}

// ============================================================================
// 着色渲染
// ============================================================================

QString PacketParserHighlighter::formatBytesAsHtml(const QByteArray &raw, PacketProto proto)
{
    if (raw.isEmpty()) return QString();

    QVector<FieldRow> rows = parseFieldTable(raw, proto);
    if (rows.isEmpty()) {
        // 无字段信息，按灰色输出
        QString html;
        for (int i = 0; i < raw.size(); ++i) {
            if (i > 0) html += " ";
            quint8 b = (quint8)raw[i];
            html += "<span style=\"background-color:#F2F2F2\">" + byteHex(b) + "</span>";
        }
        return html;
    }

    // 构建字节偏移 -> 字段名 映射
    QHash<int, QString> offsetToField;
    for (const FieldRow &r : rows) {
        for (int i = 0; i < r.length; ++i) {
            offsetToField[r.offset + i] = r.fieldName;
        }
    }

    QString html;
    for (int i = 0; i < raw.size(); ++i) {
        if (i > 0) html += " ";
        quint8 b = (quint8)raw[i];
        QString fieldName = offsetToField.value(i, "Default");
        QString color = colorForField(fieldName);
        html += "<span style=\"background-color:" + color + "\">" + byteHex(b) + "</span>";
    }
    return html;
}

QString PacketParserHighlighter::formatFieldTableAsText(const QVector<FieldRow> &rows)
{
    if (rows.isEmpty()) return QString();

    // 计算列宽
    int nameW = 4, hexW = 3;
    for (const FieldRow &r : rows) {
        nameW = qMax(nameW, r.fieldName.size());
        hexW = qMax(hexW, r.hex.size());
    }

    QString out;
    // 表头
    out += QString("  %1 | %2 | %3\n")
               .arg(QString("Field").leftJustified(nameW),
                    QString("Hex").leftJustified(hexW),
                    QString("Value/Meaning"));
    out += QString("  %1-+-%2-+-%3\n")
               .arg(QString("").leftJustified(nameW, '-'),
                    QString("").leftJustified(hexW, '-'),
                    QString("").leftJustified(20, '-'));
    // 行
    for (const FieldRow &r : rows) {
        QString mark = r.valid ? "  " : "!!";
        out += QString("%1 %2 | %3 | %4\n")
                   .arg(mark,
                        r.fieldName.leftJustified(nameW),
                        r.hex.leftJustified(hexW),
                        r.dec);
    }
    return out;
}
