#include "logmanager.h"

#include <QDir>
#include <QFileInfo>
#include <QDate>
#include <QDebug>
#include <QCoreApplication>
#include <cstdio>

/**
 * @file logmanager.cpp
 * @brief 日志管理器实现
 */

QString LogManager::levelToString(LogLevel level)
{
    switch (level) {
    case LogLevel::Debug:   return QStringLiteral("DEBUG");
    case LogLevel::Info:    return QStringLiteral("INFO");
    case LogLevel::Warning: return QStringLiteral("WARN");
    case LogLevel::Error:   return QStringLiteral("ERROR");
    }
    return QStringLiteral("UNKNOWN");
}

void LogManager::log(LogLevel level, const QString &category, const QString &message)
{
    if (level < m_minLevel) {
        return;
    }

    const QDateTime now = QDateTime::currentDateTime();
    const QString timestampStr = now.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));
    const QString line = QStringLiteral("[%1] [%2] [%3] %4")
                             .arg(timestampStr,
                                  levelToString(level),
                                  category,
                                  message);

    {
        QMutexLocker locker(&m_mutex);

        rotateIfNeeded();

        if (m_logFile.isOpen()) {
            m_stream << line << '\n';
            m_stream.flush();

            ++m_currentEntries;
            if (m_maxEntries > 0 && m_currentEntries > m_maxEntries) {
                // 简单策略：超限时关闭并打开新文件
                m_logFile.close();
                m_currentEntries = 0;
                rotateIfNeeded();
            }
        }

        // 同时输出到 stderr，便于调试（直接写入避免递归进入消息处理器）
        fprintf(stderr, "%s\n", line.toUtf8().constData());
        fflush(stderr);
    }

    emit logMessage(level, category, message, now);
}

void LogManager::setLogFile(const QString &path)
{
    QMutexLocker locker(&m_mutex);

    if (m_logFile.isOpen()) {
        m_stream.flush();
        m_logFile.close();
    }

    m_logPath = path;
    m_currentEntries = 0;

    rotateIfNeeded();
}

void LogManager::setMinLevel(LogLevel level)
{
    QMutexLocker locker(&m_mutex);
    m_minLevel = level;
}

void LogManager::setMaxEntries(int maxEntries)
{
    QMutexLocker locker(&m_mutex);
    m_maxEntries = maxEntries;
}

void LogManager::rotateIfNeeded()
{
    if (m_logPath.isEmpty()) {
        return;
    }

    const QFileInfo fi(m_logPath);
    QString targetName;

    if (fi.isDir()) {
        // 按日期滚动：debug_yyyy-MM-dd.log
        const QString dateStr = QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd"));
        QDir dir(fi.absoluteFilePath());
        targetName = dir.filePath(QStringLiteral("debug_%1.log").arg(dateStr));
    } else {
        targetName = m_logPath;
    }

    if (targetName == m_currentFileName && m_logFile.isOpen()) {
        return;
    }

    // 创建目录
    QDir dir = fi.isDir() ? QDir(fi.absoluteFilePath()) : fi.absoluteDir();
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }

    if (m_logFile.isOpen()) {
        m_stream.flush();
        m_logFile.close();
    }

    m_logFile.setFileName(targetName);
    if (m_logFile.open(QIODevice::Append | QIODevice::Text)) {
        m_stream.setDevice(&m_logFile);
        m_currentFileName = targetName;
    }
}

void LogManager::qtMessageHandler(QtMsgType type, const QMessageLogContext &context,
                                  const QString &msg)
{
    LogLevel level = LogLevel::Info;
    QString category = QString::fromLocal8Bit(context.category);

    switch (type) {
    case QtDebugMsg:    level = LogLevel::Debug;   break;
    case QtInfoMsg:     level = LogLevel::Info;    break;
    case QtWarningMsg:  level = LogLevel::Warning; break;
    case QtCriticalMsg: level = LogLevel::Error;   break;
    case QtFatalMsg:    level = LogLevel::Error;   break;
    }

    if (category.isEmpty()) {
        category = QStringLiteral("qt");
    }

    instance().log(level, category, msg);

    if (type == QtFatalMsg) {
        abort();
    }
}
