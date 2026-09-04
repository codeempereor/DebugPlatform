#ifndef DEBUGPLATFORM_LOGWIDGET_H
#define DEBUGPLATFORM_LOGWIDGET_H

/**
 * @file logwidget.h
 * @brief 日志窗口组件
 *
 * 使用 QPlainTextEdit 显示 LogManager 转发过来的日志。
 * 支持级别（Debug/Info/Warning/Error）与模块过滤，
 * 支持自动滚动、清空、保存，采用颜色编码（Debug=灰、Info=黑、
 * Warning=黄、Error=红）。
 */

#include <QWidget>
#include <QPlainTextEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QDateTime>
#include <QMap>
#include "core/config.h"
#include "ui/packetparserhighlighter.h"

/**
 * @class LogWidget
 * @brief 日志显示窗口
 */
class LogWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit LogWidget(QWidget *parent = nullptr);

    /**
     * @brief 追加一条日志
     */
    void appendLog(LogLevel level, const QString &category,
                   const QString &message, const QDateTime &timestamp);

public slots:
    /**
     * @brief 清空日志显示
     */
    void clearLogs();

    /**
     * @brief 保存当前日志到文件
     */
    void saveLogs();

private slots:
    void onLevelFilterChanged();
    void onCategoryFilterChanged();

private:
    /**
     * @brief 根据日志级别返回对应颜色
     */
    QColor colorForLevel(LogLevel level) const;

    /**
     * @brief 重新应用过滤规则（清空并从 m_buffer 中过滤）
     */
    void reapplyFilter();

    /**
     * @brief 尝试追加报文解析块（若消息含 hex 串则着色+字段表）
     * @return true 表示已追加解析块
     */
    bool tryAppendPacketParseBlock(const QString &message, QTextCursor &cursor);

private:
    QPlainTextEdit *m_editor = nullptr;

    // 过滤控件
    QCheckBox      *m_chkDebug   = nullptr;
    QCheckBox      *m_chkInfo    = nullptr;
    QCheckBox      *m_chkWarn    = nullptr;
    QCheckBox      *m_chkError   = nullptr;
    QComboBox      *m_catCombo   = nullptr;

    // 操作按钮
    QPushButton    *m_btnClear   = nullptr;
    QPushButton    *m_btnSave    = nullptr;

    // 内部缓冲，便于过滤切换时重建视图
    struct LogEntry {
        LogLevel   level;
        QString    category;
        QString    message;
        QDateTime  timestamp;
    };
    QList<LogEntry> m_buffer;

    // 最多保留条数
    static constexpr int kMaxEntries = 5000;
};

#endif // DEBUGPLATFORM_LOGWIDGET_H
