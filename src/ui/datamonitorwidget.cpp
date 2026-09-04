#include "ui/datamonitorwidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QHeaderView>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QTableWidgetItem>
#include <QTimer>
#include <QBrush>
#include "device/devicemanager.h"

DataMonitorWidget::DataMonitorWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // 顶部工具栏
    auto *topBar = new QHBoxLayout();
    auto *title = new QLabel(tr("数据监控"), this);
    QFont font = title->font();
    font.setBold(true);
    title->setFont(font);
    topBar->addWidget(title);
    topBar->addStretch();

    auto *btnExport = new QPushButton(tr("导出 CSV"), this);
    topBar->addWidget(btnExport);
    layout->addLayout(topBar);

    // 标签页
    m_tabs = new QTabWidget(this);
    m_tabs->setTabsClosable(false);
    layout->addWidget(m_tabs, 1);

    connect(btnExport, &QPushButton::clicked,
            this, &DataMonitorWidget::exportCurrentTab);
    connect(m_tabs, &QTabWidget::currentChanged,
            this, &DataMonitorWidget::onTabChanged);

    // 自动刷新定时器（每 1 秒触发，供后续扩展轮询刷新用）
    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(1000);
    connect(m_refreshTimer, &QTimer::timeout,
            this, &DataMonitorWidget::onAutoRefreshTimer);
    m_refreshTimer->start();
}

// ============================================================================
// 公共 API
// ============================================================================

void DataMonitorWidget::addDeviceTab(const QString &deviceId,
                                     const QString &deviceName)
{
    if (m_tables.contains(deviceId)) {
        // 已存在则更新标签
        int idx = indexOfDevice(deviceId);
        if (idx >= 0) m_tabs->setTabText(idx, deviceName);
        return;
    }
    QTableWidget *table = ensureTable(deviceId);
    m_tables.insert(deviceId, table);
    m_tabs->addTab(table, deviceName);
}

void DataMonitorWidget::removeDeviceTab(const QString &deviceId)
{
    if (!m_tables.contains(deviceId)) return;
    QTableWidget *table = m_tables.take(deviceId);
    int idx = m_tabs->indexOf(table);
    if (idx >= 0) m_tabs->removeTab(idx);
    delete table;
    m_lastValues.remove(deviceId);
}

void DataMonitorWidget::updateValue(const QString &deviceId, quint16 addr,
                                    double value, const QString &name,
                                    const QString &unit)
{
    QTableWidget *table = ensureTable(deviceId);
    upsertRow(table, addr, value, name, unit);

    // 与上次值比较决定是否闪烁
    auto &last = m_lastValues[deviceId];
    const bool changed = !last.contains(addr) || !qFuzzyCompare(last[addr], value);
    last[addr] = value;

    if (changed) {
        // 查找该行并闪烁
        for (int r = 0; r < table->rowCount(); ++r) {
            auto *addrItem = table->item(r, ColAddr);
            if (addrItem && addrItem->data(Qt::UserRole).toUInt() == addr) {
                for (int c = 0; c < ColCount; ++c) flashCell(table, r, c);
                break;
            }
        }
    }
}

void DataMonitorWidget::appendDataChunk(const QString &deviceId,
                                        const QByteArray &bytes)
{
    Q_UNUSED(deviceId);
    Q_UNUSED(bytes);
    // 原始数据到寄存器值的解析由业务层完成；这里仅作为接入槽位。
    // 未来可在 DeviceManager 解析完成后直接调用 updateValue。
}

void DataMonitorWidget::onDeviceStatusChanged(const QString &deviceId,
                                               DeviceStatus status)
{
    int idx = indexOfDevice(deviceId);
    if (idx < 0) return;
    QTableWidget *table = m_tables.value(deviceId);
    if (!table) return;

    QString statusText;
    QBrush brush(Qt::black);
    switch (status) {
    case DeviceStatus::Online:
        statusText = tr("在线");
        brush = QBrush(QColor(60, 170, 60));
        break;
    case DeviceStatus::Connecting:
        statusText = tr("连接中");
        brush = QBrush(QColor(200, 140, 0));
        break;
    case DeviceStatus::Error:
        statusText = tr("错误");
        brush = QBrush(QColor(200, 40, 40));
        break;
    case DeviceStatus::Offline:
    default:
        statusText = tr("离线");
        brush = QBrush(QColor(120, 120, 120));
        break;
    }
    // 更新每一行的状态列
    for (int r = 0; r < table->rowCount(); ++r) {
        auto *item = table->item(r, ColStatus);
        if (!item) {
            item = new QTableWidgetItem(statusText);
            item->setForeground(brush);
            table->setItem(r, ColStatus, item);
        } else {
            item->setText(statusText);
            item->setForeground(brush);
        }
    }
}

void DataMonitorWidget::exportCurrentTab()
{
    QString deviceId;
    QTableWidget *table = nullptr;
    const int idx = m_tabs->currentIndex();
    if (idx >= 0) {
        table = qobject_cast<QTableWidget*>(m_tabs->widget(idx));
        if (table) {
            for (auto it = m_tables.cbegin(); it != m_tables.cend(); ++it) {
                if (it.value() == table) { deviceId = it.key(); break; }
            }
        }
    }
    // 兜底：即使没有选中 tab / 未选设备，也让用户能导出带表头的空 CSV
    if (deviceId.isEmpty() && !m_tables.isEmpty()) {
        deviceId = m_tables.firstKey();
        table = m_tables.first();
    }

    const int rows = table ? table->rowCount() : 0;

    QString devName = deviceId;
    if (!deviceId.isEmpty()) {
        const DeviceConfig cfg = DeviceManager::instance().getDeviceConfig(deviceId);
        if (!cfg.name.isEmpty() && cfg.name != cfg.id) devName = cfg.name;
    }
    const QString defaultName = QStringLiteral("%1_%2.csv")
        .arg(devName.isEmpty() ? QStringLiteral("device") : devName,
             QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    const QString path = QFileDialog::getSaveFileName(
        this, tr("导出 CSV"), defaultName, tr("CSV 文件 (*.csv)"));
    if (path.isEmpty()) return;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("导出失败"),
                             tr("无法写入文件：%1").arg(path));
        return;
    }
    f.write("\xEF\xBB\xBF", 3);
    QTextStream ts(&f);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    ts.setEncoding(QStringConverter::Utf8);
#else
    ts.setCodec("UTF-8");
#endif
    auto escape = [](const QString &s) -> QString {
        QString out = s;
        if (out.contains(',') || out.contains('"')
            || out.contains('\n') || out.contains('\r')) {
            out.replace('"', QStringLiteral("\"\""));
            out = QStringLiteral("\"%1\"").arg(out);
        }
        return out;
    };
    ts << "Address,Name,Value,Unit,Status,UpdateTime\n";
    for (int r = 0; r < rows; ++r) {
        QStringList row;
        for (int c = 0; c < ColCount; ++c) {
            auto *it = table->item(r, c);
            row << escape(it ? it->text() : QString{});
        }
        ts << row.join(',') << '\n';
    }
    f.close();
    if (rows == 0) {
        QMessageBox::information(this, tr("导出完成"),
            tr("当前暂未采集到寄存器数据，已导出带表头的空 CSV：\n%1").arg(path));
    } else {
        QMessageBox::information(this, tr("导出成功"),
            tr("设备 %1 共导出 %2 条寄存器快照到：\n%3")
                .arg(devName.isEmpty() ? QStringLiteral("-") : devName)
                .arg(rows).arg(path));
    }
}

// ============================================================================
// 私有方法
// ============================================================================

void DataMonitorWidget::onTabChanged(int index)
{
    Q_UNUSED(index);
}

void DataMonitorWidget::onAutoRefreshTimer()
{
    // 供扩展使用：如需要按固定频率批量刷新显示，可在此实现。
}

QTableWidget *DataMonitorWidget::ensureTable(const QString &deviceId)
{
    if (m_tables.contains(deviceId)) return m_tables.value(deviceId);

    auto *table = new QTableWidget(this);
    table->setColumnCount(ColCount);
    table->setHorizontalHeaderLabels(QStringList{
        tr("地址"), tr("名称"), tr("当前值"), tr("单位"),
        tr("状态"), tr("更新时间")
    });
    table->horizontalHeader()->setStretchLastSection(true);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    return table;
}

void DataMonitorWidget::upsertRow(QTableWidget *table, quint16 addr,
                                  double value, const QString &name,
                                  const QString &unit)
{
    // 查找地址行
    int row = -1;
    for (int r = 0; r < table->rowCount(); ++r) {
        auto *item = table->item(r, ColAddr);
        if (item && item->data(Qt::UserRole).toUInt() == addr) { row = r; break; }
    }
    if (row < 0) {
        row = table->rowCount();
        table->insertRow(row);

        auto *addrItem = new QTableWidgetItem(
            QStringLiteral("0x%1").arg(addr, 4, 16, QLatin1Char('0')).toUpper());
        addrItem->setData(Qt::UserRole, addr);
        table->setItem(row, ColAddr, addrItem);

        auto *nameItem = new QTableWidgetItem(name);
        table->setItem(row, ColName, nameItem);

        auto *valueItem = new QTableWidgetItem();
        table->setItem(row, ColValue, valueItem);

        auto *unitItem = new QTableWidgetItem(unit);
        table->setItem(row, ColUnit, unitItem);

        auto *statusItem = new QTableWidgetItem(tr("未知"));
        statusItem->setForeground(QBrush(QColor(120, 120, 120)));
        table->setItem(row, ColStatus, statusItem);

        auto *timeItem = new QTableWidgetItem();
        table->setItem(row, ColUpdate, timeItem);
    } else {
        // 更新已有的 name / unit（若调用方传入）
        if (!name.isEmpty()) table->item(row, ColName)->setText(name);
        if (!unit.isEmpty()) table->item(row, ColUnit)->setText(unit);
    }

    // 更新值和时间
    table->item(row, ColValue)->setText(QString::number(value, 'g', 6));
    table->item(row, ColUpdate)->setText(
        QDateTime::currentDateTime().toString("hh:mm:ss.zzz"));
}

void DataMonitorWidget::flashCell(QTableWidget *table, int row, int col)
{
    auto *item = table->item(row, col);
    if (!item) return;

    // 先高亮
    QBrush oldBrush = item->background();
    item->setBackground(QBrush(m_flashColor));

    // 使用定时器 300ms 后恢复原色
    QTimer::singleShot(300, table, [item, oldBrush]() {
        if (item) item->setBackground(oldBrush);
    });
}

int DataMonitorWidget::indexOfDevice(const QString &deviceId) const
{
    if (!m_tables.contains(deviceId)) return -1;
    return m_tabs->indexOf(m_tables.value(deviceId));
}
