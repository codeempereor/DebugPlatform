#include "ui/devicetreewidget.h"
#include "device/devicemanager.h"
#include "device/deviceconfig.h"

#include <QVBoxLayout>
#include <QHeaderView>
#include <QContextMenuEvent>
#include <QColor>
#include <QBrush>
#include <QPainter>
#include <QPixmap>

namespace {
    // 分组标识
    constexpr int kGroupSerial = 0;
    constexpr int kGroupTcp    = 1;
    constexpr int kGroupUdp    = 2;
    constexpr int kGroupMqtt   = 3;
}

DeviceTreeWidget::DeviceTreeWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_tree = new QTreeWidget(this);
    m_tree->setHeaderLabels(QStringList{
        tr("名称"), tr("状态"), tr("配置")
    });
    m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
    m_tree->setUniformRowHeights(true);
    m_tree->header()->setStretchLastSection(true);
    m_tree->setColumnWidth(0, 140);
    m_tree->setColumnWidth(1, 60);
    layout->addWidget(m_tree);

    // 构建分组
    m_groupSerial = ensureGroupItem(tr("串口 (Serial)"), kGroupSerial);
    m_groupTcp    = ensureGroupItem(tr("TCP"), kGroupTcp);
    m_groupUdp    = ensureGroupItem(tr("UDP"), kGroupUdp);
    m_groupMqtt   = ensureGroupItem(tr("MQTT"), kGroupMqtt);

    // 右键菜单
    m_contextMenu = new QMenu(this);
    m_actCtxConnect    = m_contextMenu->addAction(tr("连接"));
    m_actCtxDisconnect = m_contextMenu->addAction(tr("断开"));
    m_contextMenu->addSeparator();
    m_actCtxConfig     = m_contextMenu->addAction(tr("配置…"));
    m_actCtxRemove     = m_contextMenu->addAction(tr("删除"));

    // 信号
    connect(m_tree, &QTreeWidget::itemClicked,
            this, &DeviceTreeWidget::onItemClicked);
    connect(m_tree, &QTreeWidget::customContextMenuRequested,
            this, &DeviceTreeWidget::onCustomContextMenu);

    connect(m_actCtxConnect,    &QAction::triggered,
            this, &DeviceTreeWidget::onActConnect);
    connect(m_actCtxDisconnect, &QAction::triggered,
            this, &DeviceTreeWidget::onActDisconnect);
    connect(m_actCtxConfig,     &QAction::triggered,
            this, &DeviceTreeWidget::onActConfig);
    connect(m_actCtxRemove,     &QAction::triggered,
            this, &DeviceTreeWidget::onActRemove);

    refresh();
}

void DeviceTreeWidget::refresh()
{
    // 清空分组下的子项
    for (QTreeWidgetItem *group : {m_groupSerial, m_groupTcp, m_groupUdp, m_groupMqtt}) {
        if (group) {
            const int n = group->childCount();
            for (int i = n - 1; i >= 0; --i) {
                delete group->takeChild(i);
            }
        }
    }

    auto &dm = DeviceManager::instance();
    const QStringList allIds = dm.getAllDeviceIds();
    for (const QString &id : allIds) {
        DeviceConfig cfg = dm.getDeviceConfig(id);
        if (cfg.id.isEmpty()) continue;
        QTreeWidgetItem *item = buildDeviceItem(cfg);
        QTreeWidgetItem *group = nullptr;
        switch (cfg.commType) {
        case CommunicationType::Serial: group = m_groupSerial; break;
        case CommunicationType::Tcp:    group = m_groupTcp;    break;
        case CommunicationType::Udp:    group = m_groupUdp;    break;
        case CommunicationType::Mqtt:   group = m_groupMqtt;   break;
        }
        if (group) group->addChild(item);
        updateDeviceStatus(id, dm.getDeviceStatus(id));
    }

    if (m_groupSerial) m_groupSerial->setExpanded(true);
    if (m_groupTcp)    m_groupTcp->setExpanded(true);
    if (m_groupUdp)    m_groupUdp->setExpanded(true);
    if (m_groupMqtt)   m_groupMqtt->setExpanded(true);
}

void DeviceTreeWidget::updateDeviceStatus(const QString &id, DeviceStatus status)
{
    for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
        QTreeWidgetItem *group = m_tree->topLevelItem(i);
        for (int j = 0; j < group->childCount(); ++j) {
            QTreeWidgetItem *item = group->child(j);
            if (deviceIdOf(item) == id) {
                item->setData(0, DeviceStatusRole,
                              static_cast<int>(status));
                item->setIcon(1, iconForStatus(status));
                item->setText(1, [status]() -> QString {
                    switch (status) {
                    case DeviceStatus::Offline:    return tr("离线");
                    case DeviceStatus::Connecting: return tr("连接中");
                    case DeviceStatus::Online:     return tr("在线");
                    case DeviceStatus::Error:      return tr("错误");
                    }
                    return {};
                }());
                emit deviceStatusChanged(id, status);
                return;
            }
        }
    }
}

int DeviceTreeWidget::deviceCount() const
{
    int total = 0;
    for (QTreeWidgetItem *group : {m_groupSerial, m_groupTcp, m_groupUdp, m_groupMqtt}) {
        if (group) total += group->childCount();
    }
    return total;
}

int DeviceTreeWidget::onlineCount() const
{
    int online = 0;
    for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
        QTreeWidgetItem *group = m_tree->topLevelItem(i);
        for (int j = 0; j < group->childCount(); ++j) {
            QTreeWidgetItem *item = group->child(j);
            const auto st = static_cast<DeviceStatus>(
                item->data(0, DeviceStatusRole).toInt());
            if (st == DeviceStatus::Online) ++online;
        }
    }
    return online;
}

// ============================================================================
// 私有辅助
// ============================================================================

QTreeWidgetItem *DeviceTreeWidget::buildDeviceItem(const DeviceConfig &cfg)
{
    auto *item = new QTreeWidgetItem();
    item->setText(0, cfg.name);
    item->setText(2, cfg.commConfig.toString());
    item->setData(0, DeviceIdRole, cfg.id);
    item->setData(0, DeviceStatusRole,
                  static_cast<int>(DeviceManager::instance().getDeviceStatus(cfg.id)));
    return item;
}

QTreeWidgetItem *DeviceTreeWidget::ensureGroupItem(const QString &label,
                                                   int groupRole)
{
    auto *item = new QTreeWidgetItem(m_tree);
    item->setText(0, label);
    item->setData(0, GroupTypeRole, groupRole);
    item->setExpanded(true);
    return item;
}

QIcon DeviceTreeWidget::iconForStatus(DeviceStatus status) const
{
    // 使用内置标准图标 + 颜色作为状态指示
    QPixmap px(16, 16);
    px.fill(Qt::transparent);
    QPainter p(&px);
    p.setRenderHint(QPainter::Antialiasing);
    QColor color;
    switch (status) {
    case DeviceStatus::Online:     color = QColor(60,  170, 60);  break;
    case DeviceStatus::Connecting: color = QColor(240, 190, 40);  break;
    case DeviceStatus::Error:      color = QColor(220, 60,  60);  break;
    case DeviceStatus::Offline:
    default:                       color = QColor(140, 140, 140); break;
    }
    p.setBrush(color);
    p.setPen(Qt::NoPen);
    p.drawEllipse(2, 2, 12, 12);
    p.end();
    return QIcon(px);
}

QString DeviceTreeWidget::deviceIdOf(const QTreeWidgetItem *item)
{
    return item->data(0, DeviceIdRole).toString();
}

// ============================================================================
// 槽
// ============================================================================

void DeviceTreeWidget::onItemClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    if (!item) return;
    // 只有设备子项才携带 DeviceIdRole
    const QString id = deviceIdOf(item);
    if (!id.isEmpty()) {
        emit deviceSelected(id);
    }
}

void DeviceTreeWidget::onCustomContextMenu(const QPoint &pos)
{
    QTreeWidgetItem *item = m_tree->itemAt(pos);
    if (!item) return;
    const QString id = deviceIdOf(item);
    if (id.isEmpty()) return;

    m_contextDeviceId = id;
    const auto st = static_cast<DeviceStatus>(
        item->data(0, DeviceStatusRole).toInt());
    m_actCtxConnect->setEnabled(st != DeviceStatus::Online);
    m_actCtxDisconnect->setEnabled(st == DeviceStatus::Online);

    m_contextMenu->exec(m_tree->viewport()->mapToGlobal(pos));
}

void DeviceTreeWidget::onActConnect()
{
    if (!m_contextDeviceId.isEmpty()) {
        emit deviceConnectRequested(m_contextDeviceId);
    }
}

void DeviceTreeWidget::onActDisconnect()
{
    if (!m_contextDeviceId.isEmpty()) {
        emit deviceDisconnectRequested(m_contextDeviceId);
    }
}

void DeviceTreeWidget::onActConfig()
{
    if (!m_contextDeviceId.isEmpty()) {
        emit deviceConfigRequested(m_contextDeviceId);
    }
}

void DeviceTreeWidget::onActRemove()
{
    if (!m_contextDeviceId.isEmpty()) {
        emit deviceRemoveRequested(m_contextDeviceId);
    }
}
