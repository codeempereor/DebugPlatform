#include "datapersistent.h"

#include <cstdio>

#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QUuid>
#include <QDebug>
#include <QDateTime>
#include <QTimeZone>

/**
 * @file datapersistent.cpp
 * @brief SQLite 数据持久化实现
 */

DataPersistent::DataPersistent(QObject *parent)
    : QObject(parent)
{
}

DataPersistent::~DataPersistent()
{
    // 必须先让 QSqlDatabase 实例离开作用域/被清除，
    // 再调用 removeDatabase(connectionName)，否则会输出"still in use"警告。
    const QString conn = std::move(m_connectionName);
    m_db = QSqlDatabase();
    if (!conn.isEmpty() && QSqlDatabase::contains(conn)) {
        QSqlDatabase::removeDatabase(conn);
    }
}

bool DataPersistent::initialize(const QString &dbPath)
{
    fprintf(stderr, "[DB] DataPersistent::initialize called with path: %s\n",
            dbPath.toUtf8().constData());
    fflush(stderr);

    if (m_initialized) {
        if (m_db.databaseName() == dbPath && m_db.isOpen()) return true;
        const QString oldConn = std::move(m_connectionName);
        m_db = QSqlDatabase();
        if (!oldConn.isEmpty() && QSqlDatabase::contains(oldConn)) {
            QSqlDatabase::removeDatabase(oldConn);
        }
        m_connectionName.clear();
        m_initialized = false;
    }

    m_connectionName = QStringLiteral("debug_platform_data_%1")
                           .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));

    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    m_db.setDatabaseName(dbPath);

    fprintf(stderr, "[DB] Opening database...\n");
    fflush(stderr);

    if (!m_db.open()) {
        fprintf(stderr, "[DB] FAILED to open: %s\n",
                m_db.lastError().text().toUtf8().constData());
        fflush(stderr);
        qWarning() << "DataPersistent: Failed to open database" << dbPath
                   << m_db.lastError().text();
        const QString conn = std::move(m_connectionName);
        m_db = QSqlDatabase();
        if (!conn.isEmpty() && QSqlDatabase::contains(conn)) {
            QSqlDatabase::removeDatabase(conn);
        }
        m_connectionName.clear();
        return false;
    }

    fprintf(stderr, "[DB] Opened successfully. Setting pragmas...\n");
    fflush(stderr);

    QSqlQuery pragmas(m_db);
    pragmas.exec(QStringLiteral("PRAGMA journal_mode=WAL"));
    pragmas.exec(QStringLiteral("PRAGMA synchronous=NORMAL"));
    pragmas.exec(QStringLiteral("PRAGMA foreign_keys=ON"));

    if (!createTables()) {
        fprintf(stderr, "[DB] createTables FAILED!\n");
        fflush(stderr);
        return false;
    }

    fprintf(stderr, "[DB] Initialized successfully!\n");
    fflush(stderr);

    m_initialized = true;
    return true;
}

bool DataPersistent::createTables()
{
    QSqlQuery q(m_db);

    const QString createDevices =
        QStringLiteral("CREATE TABLE IF NOT EXISTS devices ("
                       "id TEXT PRIMARY KEY, "
                       "name TEXT NOT NULL, "
                       "type TEXT, "
                       "config TEXT, "
                       "created_at DATETIME DEFAULT CURRENT_TIMESTAMP)");

    const QString createRegisterData =
        QStringLiteral("CREATE TABLE IF NOT EXISTS register_data ("
                       "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                       "device_id TEXT NOT NULL, "
                       "register_addr INTEGER NOT NULL, "
                       "value REAL, "
                       "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP, "
                       "FOREIGN KEY(device_id) REFERENCES devices(id) ON DELETE CASCADE)");

    const QString createRegisterIdx =
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_regdata_device_addr_time "
                       "ON register_data(device_id, register_addr, timestamp)");

    const QString createAlerts =
        QStringLiteral("CREATE TABLE IF NOT EXISTS alerts ("
                       "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                       "device_id TEXT NOT NULL, "
                       "register_addr INTEGER, "
                       "value REAL, "
                       "level TEXT, "
                       "message TEXT, "
                       "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP, "
                       "FOREIGN KEY(device_id) REFERENCES devices(id) ON DELETE CASCADE)");

    const QString createAlertsIdx =
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_alerts_device_time "
                       "ON alerts(device_id, timestamp)");

    const QString createConfigs =
        QStringLiteral("CREATE TABLE IF NOT EXISTS configs ("
                       "key TEXT PRIMARY KEY, "
                       "value TEXT, "
                       "updated_at DATETIME DEFAULT CURRENT_TIMESTAMP)");

    const QStringList statements = {
        createDevices,
        createRegisterData,
        createRegisterIdx,
        createAlerts,
        createAlertsIdx,
        createConfigs
    };

    for (const QString &stmt : statements) {
        if (!q.exec(stmt)) {
            qWarning() << "DataPersistent: Failed to execute:" << stmt
                       << q.lastError().text();
            return false;
        }
    }
    return true;
}

bool DataPersistent::isReady() const
{
    return m_initialized && m_db.isOpen();
}

bool DataPersistent::addDevice(const QString &id, const QString &name,
                               const QString &type, const QString &config)
{
    if (!isReady()) {
        return false;
    }

    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO devices(id, name, type, config) VALUES(?, ?, ?, ?) "
        "ON CONFLICT(id) DO UPDATE SET name=excluded.name, type=excluded.type, config=excluded.config"));
    q.addBindValue(id);
    q.addBindValue(name);
    q.addBindValue(type);
    q.addBindValue(config);

    if (!q.exec()) {
        qWarning() << "DataPersistent::addDevice failed:" << q.lastError().text();
        return false;
    }
    return true;
}

bool DataPersistent::removeDevice(const QString &id)
{
    if (!isReady()) {
        return false;
    }

    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("DELETE FROM devices WHERE id = ?"));
    q.addBindValue(id);

    if (!q.exec()) {
        qWarning() << "DataPersistent::removeDevice failed:" << q.lastError().text();
        return false;
    }
    return true;
}

bool DataPersistent::insertRegisterData(const QString &deviceId, quint16 addr, double value)
{
    if (!isReady()) {
        return false;
    }

    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO register_data(device_id, register_addr, value, timestamp) "
        "VALUES(?, ?, ?, CURRENT_TIMESTAMP)"));
    q.addBindValue(deviceId);
    q.addBindValue(static_cast<int>(addr));
    q.addBindValue(value);

    if (!q.exec()) {
        qWarning() << "DataPersistent::insertRegisterData failed:" << q.lastError().text();
        return false;
    }
    return true;
}

bool DataPersistent::insertRegisterDataBatch(
    const QVector<QPair<QString, QPair<quint16, double>>> &batch)
{
    if (!isReady() || batch.isEmpty()) {
        return false;
    }

    QSqlQuery q(m_db);
    if (!q.exec(QStringLiteral("BEGIN IMMEDIATE"))) {
        qWarning() << "DataPersistent::insertRegisterDataBatch begin failed:"
                   << q.lastError().text();
        return false;
    }

    // QSQLITE driver in Qt 5.14 has issues with execBatch(), use per-row exec() instead
    q.prepare(QStringLiteral(
        "INSERT INTO register_data(device_id, register_addr, value, timestamp) "
        "VALUES(?, ?, ?, CURRENT_TIMESTAMP)"));

    int okCount = 0;
    for (const auto &item : batch) {
        q.addBindValue(item.first);                        // device_id
        q.addBindValue(static_cast<int>(item.second.first)); // register_addr
        q.addBindValue(item.second.second);                // value
        if (q.exec()) {
            ++okCount;
        } else {
            qWarning() << "DataPersistent::insertRegisterDataBatch row failed:"
                       << q.lastError().text();
        }
    }

    if (okCount == 0) {
        qWarning() << "DataPersistent::insertRegisterDataBatch: all"
                   << batch.size() << "rows failed";
        q.exec(QStringLiteral("ROLLBACK"));
        return false;
    }

    q.exec(QStringLiteral("COMMIT"));
    return true;
}

QVector<QVariantMap> DataPersistent::loadAllDevices()
{
    QVector<QVariantMap> result;
    if (!isReady()) return result;

    QSqlQuery q(m_db);
    const QString sql = QStringLiteral(
        "SELECT id, name, type, config FROM devices ORDER BY created_at ASC");
    if (!q.exec(sql)) {
        qWarning() << "DataPersistent::loadAllDevices failed:" << q.lastError().text();
        return result;
    }

    while (q.next()) {
        QVariantMap row;
        row.insert(QStringLiteral("id"),     q.value(0));
        row.insert(QStringLiteral("name"),   q.value(1));
        row.insert(QStringLiteral("type"),   q.value(2));
        row.insert(QStringLiteral("config"), q.value(3));
        result.append(row);
    }
    return result;
}

bool DataPersistent::insertAlert(const QString &deviceId, quint16 addr, double value,
                                 const QString &level, const QString &message)
{
    if (!isReady()) {
        return false;
    }

    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO alerts(device_id, register_addr, value, level, message, timestamp) "
        "VALUES(?, ?, ?, ?, ?, CURRENT_TIMESTAMP)"));
    q.addBindValue(deviceId);
    q.addBindValue(static_cast<int>(addr));
    q.addBindValue(value);
    q.addBindValue(level);
    q.addBindValue(message);

    if (!q.exec()) {
        qWarning() << "DataPersistent::insertAlert failed:" << q.lastError().text();
        return false;
    }
    return true;
}

// Helper: convert QDateTime to UTC string matching SQLite CURRENT_TIMESTAMP format
static QString toDbTimestamp(const QDateTime &dt)
{
    if (!dt.isValid()) {
        return QStringLiteral("1970-01-01 00:00:00");
    }
    return dt.toUTC().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
}

// Helper: convert SQLite CURRENT_TIMESTAMP string to local QDateTime
static QDateTime fromDbTimestamp(const QString &str)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QDateTime dt = QDateTime::fromString(str, QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    if (dt.isValid()) {
        dt = QDateTime(dt.date(), dt.time(), QTimeZone::utc());
    }
#else
    QDateTime dt = QDateTime::fromString(str, QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    if (dt.isValid()) {
        dt.setTimeSpec(Qt::UTC);
    }
#endif
    if (dt.isValid()) {
        dt = dt.toLocalTime();
    }
    return dt;
}

QVector<QVariantMap> DataPersistent::queryRegisterData(const QString &deviceId, quint16 addr,
                                                        const QDateTime &start, const QDateTime &end)
{
    QVector<QVariantMap> results;
    if (!isReady()) {
        return results;
    }

    const QString sql = QStringLiteral(
        "SELECT device_id, register_addr, value, timestamp "
        "FROM register_data "
        "WHERE device_id = ? AND register_addr = ? "
        "AND timestamp BETWEEN ? AND ? "
        "ORDER BY timestamp ASC");

#ifdef QT_DEBUG
    explainQuery(sql);
#endif

    QSqlQuery q(m_db);
    q.prepare(sql);
    q.addBindValue(deviceId);
    q.addBindValue(static_cast<int>(addr));
    q.addBindValue(toDbTimestamp(start));
    q.addBindValue(toDbTimestamp(end));

    if (!q.exec()) {
        qWarning() << "DataPersistent::queryRegisterData failed:" << q.lastError().text();
        return results;
    }

    while (q.next()) {
        QVariantMap record;
        record.insert(QStringLiteral("device_id"),      q.value(0));
        record.insert(QStringLiteral("register_addr"),  q.value(1));
        record.insert(QStringLiteral("value"),          q.value(2));
        record.insert(QStringLiteral("timestamp"),      fromDbTimestamp(q.value(3).toString()));
        results.append(record);
    }
    return results;
}

QVector<QVariantMap> DataPersistent::queryAlerts(const QString &deviceId,
                                                  const QDateTime &start, const QDateTime &end)
{
    QVector<QVariantMap> results;
    if (!isReady()) {
        return results;
    }

    const QString sql = QStringLiteral(
        "SELECT id, device_id, register_addr, value, level, message, timestamp "
        "FROM alerts "
        "WHERE device_id = ? AND timestamp BETWEEN ? AND ? "
        "ORDER BY timestamp ASC");

#ifdef QT_DEBUG
    explainQuery(sql);
#endif

    QSqlQuery q(m_db);
    q.prepare(sql);
    q.addBindValue(deviceId);
    q.addBindValue(toDbTimestamp(start));
    q.addBindValue(toDbTimestamp(end));

    if (!q.exec()) {
        qWarning() << "DataPersistent::queryAlerts failed:" << q.lastError().text();
        return results;
    }

    while (q.next()) {
        QVariantMap record;
        record.insert(QStringLiteral("id"),             q.value(0));
        record.insert(QStringLiteral("device_id"),      q.value(1));
        record.insert(QStringLiteral("register_addr"),  q.value(2));
        record.insert(QStringLiteral("value"),          q.value(3));
        record.insert(QStringLiteral("level"),          q.value(4));
        record.insert(QStringLiteral("message"),        q.value(5));
        record.insert(QStringLiteral("timestamp"),      fromDbTimestamp(q.value(6).toString()));
        results.append(record);
    }
    return results;
}

QVector<QVariantMap> DataPersistent::queryRecentAlerts(int limit)
{
    QVector<QVariantMap> results;
    if (!isReady() || limit <= 0) return results;

    const QString sql = QStringLiteral(
        "SELECT id, device_id, register_addr, value, level, message, timestamp "
        "FROM alerts ORDER BY id DESC LIMIT %1").arg(limit);

#ifdef QT_DEBUG
    explainQuery(sql);
#endif
    QSqlQuery q(m_db);
    if (!q.exec(sql)) {
        qWarning() << "DataPersistent::queryRecentAlerts failed:" << q.lastError().text();
        return results;
    }
    while (q.next()) {
        QVariantMap record;
        record.insert(QStringLiteral("id"),             q.value(0));
        record.insert(QStringLiteral("device_id"),      q.value(1));
        record.insert(QStringLiteral("register_addr"),  q.value(2));
        record.insert(QStringLiteral("value"),          q.value(3));
        record.insert(QStringLiteral("level"),          q.value(4));
        record.insert(QStringLiteral("message"),        q.value(5));
        record.insert(QStringLiteral("timestamp"),      fromDbTimestamp(q.value(6).toString()));
        results.append(record);
    }
    return results;
}

bool DataPersistent::saveConfig(const QString &key, const QString &value)
{
    if (!isReady()) {
        return false;
    }

    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO configs(key, value, updated_at) VALUES(?, ?, CURRENT_TIMESTAMP) "
        "ON CONFLICT(key) DO UPDATE SET value=excluded.value, updated_at=CURRENT_TIMESTAMP"));
    q.addBindValue(key);
    q.addBindValue(value);

    if (!q.exec()) {
        qWarning() << "DataPersistent::saveConfig failed:" << q.lastError().text();
        return false;
    }
    return true;
}

QString DataPersistent::loadConfig(const QString &key)
{
    if (!isReady()) {
        return {};
    }

    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT value FROM configs WHERE key = ?"));
    q.addBindValue(key);

    if (!q.exec()) {
        qWarning() << "DataPersistent::loadConfig failed:" << q.lastError().text();
        return {};
    }

    if (q.next()) {
        return q.value(0).toString();
    }
    return {};
}

void DataPersistent::cleanupOldData(int daysRetention)
{
    if (!isReady() || daysRetention <= 0) {
        return;
    }

    const QDateTime cutoff = QDateTime::currentDateTime().addDays(-daysRetention);

    // 使用事务批量删除（参数化查询避免 SQL 注入）
    QSqlQuery q(m_db);
    if (!q.exec(QStringLiteral("BEGIN IMMEDIATE"))) {
        qWarning() << "DataPersistent::cleanupOldData begin transaction failed:"
                   << q.lastError().text();
        return;
    }

    bool ok = true;

    q.prepare(QStringLiteral("DELETE FROM register_data WHERE timestamp < ?"));
    q.addBindValue(toDbTimestamp(cutoff));
    ok &= q.exec();

    q.prepare(QStringLiteral("DELETE FROM alerts WHERE timestamp < ?"));
    q.addBindValue(toDbTimestamp(cutoff));
    ok &= q.exec();

    if (ok) {
        q.exec(QStringLiteral("COMMIT"));
        q.exec(QStringLiteral("VACUUM"));
    } else {
        qWarning() << "DataPersistent::cleanupOldData delete failed:" << q.lastError().text();
        q.exec(QStringLiteral("ROLLBACK"));
    }
}

void DataPersistent::explainQuery(const QString &sql) const
{
    if (!m_db.isOpen()) {
        return;
    }

    QSqlQuery q(m_db);
    const QString explainSql = QStringLiteral("EXPLAIN QUERY PLAN ") + sql;
    if (q.exec(explainSql)) {
        while (q.next()) {
            qDebug() << "EXPLAIN:" << q.value(0).toString();
        }
    }
}
