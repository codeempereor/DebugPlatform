#ifndef DEBUGPLATFORM_LOGMANAGER_H
#define DEBUGPLATFORM_LOGMANAGER_H

/**
 * @file logmanager.h
 * @brief 日志管理器（单例模式）
 *
 * 提供统一的日志输出接口，支持多级别过滤、按日期滚动日志文件、
 * 通过 Qt 信号将日志消息转发给 UI 等其他模块。
 *
 * 日志格式：[时间戳] [级别] [模块] 消息
 */

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QMutex>
#include <QFile>
#include <QTextStream>
#include "config.h"

/**
 * @class LogManager
 * @brief 日志管理器（单例）
 *
 * 使用 SINGLETON 宏实现单例。通过 log() 写入日志，通过
 * logMessage 信号将日志广播给其它模块（如 UI 日志面板）。
 */
class LogManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 输出一条日志
     * @param level 日志级别
     * @param category 日志分类（模块名）
     * @param message 日志内容
     */
    void log(LogLevel level, const QString &category, const QString &message);

    /**
     * @brief 设置日志文件路径
     * @param path 日志文件完整路径
     *
     * 若路径为目录则按日期滚动；若为文件路径则直接写入该文件。
     */
    void setLogFile(const QString &path);

    /**
     * @brief 设置最低输出级别
     * @param level 仅输出大于等于该级别的日志
     */
    void setMinLevel(LogLevel level);

    /**
     * @brief 设置最多保留的日志条数
     * @param maxEntries 最大条数；0 表示不限制
     */
    void setMaxEntries(int maxEntries);

    /**
     * @brief 将 Qt 全局日志消息转发到本管理器
     *
     * 需在应用启动时调用 qInstallMessageHandler(LogManager::qtMessageHandler)。
     */
    static void qtMessageHandler(QtMsgType type, const QMessageLogContext &context,
                                 const QString &msg);

signals:
    /**
     * @brief 日志消息信号
     * @param level 日志级别
     * @param category 日志分类
     * @param message 日志内容
     * @param timestamp 时间戳
     */
    void logMessage(LogLevel level, const QString &category,
                    const QString &message, const QDateTime &timestamp);

private:
    /**
     * @brief 获取指定级别对应的字符串
     * @param level 日志级别
     * @return 级别短字符串
     */
    static QString levelToString(LogLevel level);

    /**
     * @brief 检查并执行日志文件按日期滚动
     */
    void rotateIfNeeded();

    mutable QMutex  m_mutex;            ///< 互斥锁，保护文件写入
    QFile           m_logFile;         ///< 当前日志文件
    QTextStream     m_stream;          ///< 文件流
    QString         m_logPath;         ///< 日志文件路径（可能为目录）
    QString         m_currentFileName; ///< 当前打开的文件名
    LogLevel        m_minLevel = LogLevel::Debug; ///< 最低输出级别
    int             m_maxEntries = 0;  ///< 最多保留条数（0 = 不限）
    int             m_currentEntries = 0; ///< 当前已写入条数

    SINGLETON(LogManager)
};

#endif // DEBUGPLATFORM_LOGMANAGER_H
