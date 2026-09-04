#ifndef DEBUGPLATFORM_DEVICETREEWIDGET_H
#define DEBUGPLATFORM_DEVICETREEWIDGET_H

/**
 * @file devicetreewidget.h
 * @brief 设备列表树组件
 *
 * 以 QTreeWidget 展示当前已注册的设备，按通信类型（Serial/TCP/UDP）
 * 分组。每个设备项显示：名称、状态图标、通信配置摘要。支持右键菜单
 * 进行连接/断开/配置/删除等操作。
 */

#include <QWidget>
#include <QTreeWidget>
#include <QMenu>
#include <QAction>
#include <QString>
#include "core/config.h"

class DeviceConfig;

/**
 * @class DeviceTreeWidget
 * @brief 设备列表树
 */
class DeviceTreeWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit DeviceTreeWidget(QWidget *parent = nullptr);

    /**
     * @brief 刷新设备列表（从 DeviceManager 重新拉取）
     */
    void refresh();

    /**
     * @brief 更新单个设备的状态显示
     * @param id 设备 ID
     * @param status 新状态
     */
    void updateDeviceStatus(const QString &id, DeviceStatus status);

    /**
     * @brief 当前设备总数
     */
    int deviceCount() const;

    /**
     * @brief 当前在线设备数
     */
    int onlineCount() const;

signals:
    void deviceSelected(const QString &deviceId);
    void deviceConnectRequested(const QString &deviceId);
    void deviceDisconnectRequested(const QString &deviceId);
    void deviceConfigRequested(const QString &deviceId);
    void deviceRemoveRequested(const QString &deviceId);
    void deviceStatusChanged(const QString &deviceId, DeviceStatus status);

private slots:
    void onItemClicked(QTreeWidgetItem *item, int column);
    void onCustomContextMenu(const QPoint &pos);

    // 右键菜单动作
    void onActConnect();
    void onActDisconnect();
    void onActConfig();
    void onActRemove();

private:
    /**
     * @brief 根据 DeviceConfig 构建 QTreeWidgetItem
     */
    QTreeWidgetItem *buildDeviceItem(const DeviceConfig &cfg);

    /**
     * @brief 确保分组节点存在
     */
    QTreeWidgetItem *ensureGroupItem(const QString &label, int groupRole);

    /**
     * @brief 根据状态返回图标
     */
    QIcon iconForStatus(DeviceStatus status) const;

    /**
     * @brief 从 QTreeWidgetItem 取出 deviceId
     */
    static QString deviceIdOf(const QTreeWidgetItem *item);

private:
    QTreeWidget      *m_tree     = nullptr;

    // 分组项
    QTreeWidgetItem  *m_groupSerial = nullptr;
    QTreeWidgetItem  *m_groupTcp    = nullptr;
    QTreeWidgetItem  *m_groupUdp    = nullptr;
    QTreeWidgetItem  *m_groupMqtt   = nullptr;

    // 右键菜单
    QMenu            *m_contextMenu    = nullptr;
    QAction          *m_actCtxConnect = nullptr;
    QAction          *m_actCtxDisconnect = nullptr;
    QAction          *m_actCtxConfig   = nullptr;
    QAction          *m_actCtxRemove   = nullptr;

    // 当前右键操作的设备 ID
    QString           m_contextDeviceId;

    // ItemData 角色
    enum TreeRoles {
        DeviceIdRole = Qt::UserRole + 1,
        DeviceStatusRole,
        GroupTypeRole
    };
};

#endif // DEBUGPLATFORM_DEVICETREEWIDGET_H
