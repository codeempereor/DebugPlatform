#include "ui/logwidget.h"
#include "protocol/crc16.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QTextCursor>
#include <QScrollBar>

LogWidget::LogWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);

    // 顶部过滤栏
    auto *filterBar = new QHBoxLayout();
    filterBar->addWidget(new QLabel(tr("日志过滤：")));

    m_chkDebug = new QCheckBox(tr("Debug"), this);
    m_chkDebug->setChecked(true);
    filterBar->addWidget(m_chkDebug);

    m_chkInfo = new QCheckBox(tr("Info"), this);
    m_chkInfo->setChecked(true);
    filterBar->addWidget(m_chkInfo);

    m_chkWarn = new QCheckBox(tr("Warning"), this);
    m_chkWarn->setChecked(true);
    filterBar->addWidget(m_chkWarn);

    m_chkError = new QCheckBox(tr("Error"), this);
    m_chkError->setChecked(true);
    filterBar->addWidget(m_chkError);

    filterBar->addSpacing(12);
    filterBar->addWidget(new QLabel(tr("模块：")));

    m_catCombo = new QComboBox(this);
    m_catCombo->addItem(tr("All"), QString{});
    m_catCombo->addItem(tr("Core"),     QStringLiteral("core"));
    m_catCombo->addItem(tr("Protocol"), QStringLiteral("protocol"));
    m_catCombo->addItem(tr("Data"),     QStringLiteral("data"));
    m_catCombo->addItem(tr("Device"),   QStringLiteral("device"));
    m_catCombo->addItem(tr("UI"),       QStringLiteral("ui"));
    m_catCombo->addItem(tr("App"),      QStringLiteral("app"));
    m_catCombo->addItem(tr("Alert"),    QStringLiteral("alert"));
    m_catCombo->addItem(tr("Playback"), QStringLiteral("playback"));
    // ↓ 新增 3 个缺漏分类（避免 qInstallMessageHandler 路由过来的日志
    //   因为 category 不在下拉里而无法被单独筛选）
    m_catCombo->addItem(tr("MQTT"),     QStringLiteral("mqtt"));
    m_catCombo->addItem(tr("Default"),  QStringLiteral("default"));
    m_catCombo->addItem(tr("Qt"),       QStringLiteral("qt"));
    filterBar->addWidget(m_catCombo);

    filterBar->addStretch();

    m_btnClear = new QPushButton(tr("清空"), this);
    filterBar->addWidget(m_btnClear);

    m_btnSave = new QPushButton(tr("保存…"), this);
    filterBar->addWidget(m_btnSave);

    layout->addLayout(filterBar);

    // 日志编辑器
    m_editor = new QPlainTextEdit(this);
    m_editor->setReadOnly(true);
    m_editor->setMaximumBlockCount(20000);
    m_editor->document()->setDefaultFont(QFont("Consolas", 9));
    layout->addWidget(m_editor, 1);

    // 信号
    connect(m_chkDebug,  SIGNAL(stateChanged(int)),
            this, SLOT(onLevelFilterChanged()));
    connect(m_chkInfo,   SIGNAL(stateChanged(int)),
            this, SLOT(onLevelFilterChanged()));
    connect(m_chkWarn,   SIGNAL(stateChanged(int)),
            this, SLOT(onLevelFilterChanged()));
    connect(m_chkError,  SIGNAL(stateChanged(int)),
            this, SLOT(onLevelFilterChanged()));
    connect(m_catCombo,  SIGNAL(currentIndexChanged(int)),
            this, SLOT(onCategoryFilterChanged()));
    connect(m_btnClear,  &QPushButton::clicked,
            this, &LogWidget::clearLogs);
    connect(m_btnSave,   &QPushButton::clicked,
            this, &LogWidget::saveLogs);
}

// ============================================================================
// 公共 API
// ============================================================================

void LogWidget::appendLog(LogLevel level, const QString &category,
                         const QString &message, const QDateTime &timestamp)
{
    LogEntry entry{level, category, message, timestamp};
    m_buffer.append(entry);

    // 控制长度
    if (m_buffer.size() > kMaxEntries * 2) {
        m_buffer.removeFirst();
    }

    // 若通过当前过滤，则直接追加
    const bool show = [this, &entry]() {
        // 级别过滤
        if (entry.level == LogLevel::Debug   && !m_chkDebug->isChecked())  return false;
        if (entry.level == LogLevel::Info    && !m_chkInfo->isChecked())   return false;
        if (entry.level == LogLevel::Warning && !m_chkWarn->isChecked())   return false;
        if (entry.level == LogLevel::Error   && !m_chkError->isChecked())  return false;
        // 模块过滤
        const QString catFilter = m_catCombo->currentData().toString();
        if (!catFilter.isEmpty() && entry.category != catFilter) return false;
        return true;
    }();

    if (show) {
        QTextCursor cursor = m_editor->textCursor();
        cursor.movePosition(QTextCursor::End);

        QTextCharFormat fmt;
        fmt.setForeground(colorForLevel(entry.level));
        cursor.setCharFormat(fmt);

        const QString line = QStringLiteral("[%1] [%2] [%3] %4")
            .arg(entry.timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz"),
                 [](LogLevel lv) -> QString {
                     switch (lv) {
                     case LogLevel::Debug:   return QStringLiteral("DEBUG  ");
                     case LogLevel::Info:    return QStringLiteral("INFO   ");
                     case LogLevel::Warning: return QStringLiteral("WARN   ");
                     case LogLevel::Error:   return QStringLiteral("ERROR  ");
                     }
                     return {};
                 }(entry.level),
                 entry.category,
                 entry.message);
        cursor.insertText(line + QLatin1Char('\n'));
        tryAppendPacketParseBlock(entry.message, cursor);
        m_editor->setTextCursor(cursor);
        m_editor->ensureCursorVisible();
    }
}

void LogWidget::clearLogs()
{
    m_buffer.clear();
    m_editor->clear();
}

void LogWidget::saveLogs()
{
    const QString defaultName = QStringLiteral("log_%1.txt")
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    const QString path = QFileDialog::getSaveFileName(
        this, tr("保存日志"), defaultName, tr("文本文件 (*.txt);;所有文件 (*)"));
    if (path.isEmpty()) return;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream ts(&f);
    for (const LogEntry &e : m_buffer) {
        ts << QStringLiteral("[%1] [%2] [%3] %4\n")
                  .arg(e.timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz"),
                       [](LogLevel lv) -> QString {
                           switch (lv) {
                           case LogLevel::Debug:   return "DEBUG";
                           case LogLevel::Info:    return "INFO";
                           case LogLevel::Warning: return "WARN";
                           case LogLevel::Error:   return "ERROR";
                           }
                           return {};
                       }(e.level),
                       e.category, e.message);
    }
    f.close();
}

// ============================================================================
// 过滤槽
// ============================================================================

void LogWidget::onLevelFilterChanged()
{
    reapplyFilter();
}

void LogWidget::onCategoryFilterChanged()
{
    reapplyFilter();
}

void LogWidget::reapplyFilter()
{
    m_editor->clear();
    for (const LogEntry &e : m_buffer) {
        const bool show = [this, &e]() {
            if (e.level == LogLevel::Debug   && !m_chkDebug->isChecked())  return false;
            if (e.level == LogLevel::Info    && !m_chkInfo->isChecked())   return false;
            if (e.level == LogLevel::Warning && !m_chkWarn->isChecked())   return false;
            if (e.level == LogLevel::Error   && !m_chkError->isChecked())  return false;
            const QString catFilter = m_catCombo->currentData().toString();
            if (!catFilter.isEmpty() && e.category != catFilter) return false;
            return true;
        }();
        if (!show) continue;

        QTextCursor cursor = m_editor->textCursor();
        cursor.movePosition(QTextCursor::End);
        QTextCharFormat fmt;
        fmt.setForeground(colorForLevel(e.level));
        cursor.setCharFormat(fmt);
        cursor.insertText(QStringLiteral("[%1] [%2] [%3] %4\n")
                              .arg(e.timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz"),
                                   [](LogLevel lv) -> QString {
                                       switch (lv) {
                                       case LogLevel::Debug:   return "DEBUG  ";
                                       case LogLevel::Info:    return "INFO   ";
                                       case LogLevel::Warning: return "WARN   ";
                                       case LogLevel::Error:   return "ERROR  ";
                                       }
                                       return {};
                                   }(e.level),
                                   e.category, e.message));
        tryAppendPacketParseBlock(e.message, cursor);
    }
    // 滚到底部
    auto *sb = m_editor->verticalScrollBar();
    if (sb) sb->setValue(sb->maximum());
}

bool LogWidget::tryAppendPacketParseBlock(const QString &message, QTextCursor &cursor)
{
    QByteArray raw;
    PacketParserHighlighter::PacketProto proto =
        PacketParserHighlighter::detectFromString(message, &raw);
    if (proto == PacketParserHighlighter::Unknown || raw.isEmpty()) {
        return false;  // 非报文消息，不追加
    }

    // 解析字段表
    QVector<PacketParserHighlighter::FieldRow> rows =
        PacketParserHighlighter::parseFieldTable(raw, proto);

    // 构造 HTML 块（QPlainTextEdit 友好：span + br，避免 div/table/pre）
    QString html = QStringLiteral(
        "<br><span style=\"color:#888\">  └ 报文解析 [%1] (%2 bytes)</span><br>"
    ).arg(PacketParserHighlighter::protoName(proto))
     .arg(raw.size());

    // 字节着色区
    html += QStringLiteral("<span style=\"color:#888\">  字节: </span>");
    html += PacketParserHighlighter::formatBytesAsHtml(raw, proto);
    html += QStringLiteral("<br>");

    // 字段表（纯文本转 HTML，保持对齐）
    QString tableText = PacketParserHighlighter::formatFieldTableAsText(rows);
    // 转义 HTML 特殊字符
    tableText.replace('&', "&amp;")
             .replace('<', "&lt;")
             .replace('>', "&gt;");
    tableText.replace('\n', "<br>");  // 换行转 <br>
    html += QStringLiteral("<span style=\"color:#555\">") + tableText + QStringLiteral("</span><br>");

    cursor.insertHtml(html);
    return true;
}

QColor LogWidget::colorForLevel(LogLevel level) const
{
    switch (level) {
    case LogLevel::Debug:   return QColor("#4A90E2"); // 蓝灰
    case LogLevel::Info:    return QColor("#000000"); // 默认黑
    case LogLevel::Warning: return QColor("#F5A623"); // 橙
    case LogLevel::Error:   return QColor("#D0021B"); // 红
    }
    return QColor("#000000");
}
