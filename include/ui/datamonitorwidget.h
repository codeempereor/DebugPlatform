#ifndef DEBUGPLATFORM_DATAMONITORWIDGET_H
#define DEBUGPLATFORM_DATAMONITORWIDGET_H

/**
 * @file datamonitorwidget.h
 * @brief 数据监控组件
 *
 * 以 QTableWidget 实时展示一个设备的寄存器值。多个设备通过 QTabWidget
 * 分页展示。值变化时闪烁高亮，并支持定时自动刷新与 CSV 导出。
 */

#include <QWidget>
#include <QTableWidget>
#include <QTabWidget>
#include <QMap>
#include <QString>
#include <QByteArray>
#include <QDateTime>
#include "core/config.h"

class DeviceConfig;

/**
 * @class DataMonitorWidget
 * @brief 数据监控面板
 */
class DataMonitorWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit DataMonitorWidget(QWidget *parent = nullptr);

    /**
     * @brief 为指定设备创建一个数据标签页
     */
    void addDeviceTab(const QString &deviceId, const QString &deviceName);

    /**
     * @brief 移除指定设备的标签页
     */
    void removeDeviceTab(const QString &deviceId);

    /**
     * @brief 更新一个寄存器值
     */
    void updateValue(const QString &deviceId, quint16 addr,
                    double value, const QString &name = QString(),
                    const QString &unit = QString());

public slots:
    /**
     * @brief 当接收到原始数据时解析并刷新（占位：实际解析交给业务层）
     */
    void appendDataChunk(const QString &deviceId, const QByteArray &bytes);

    /**
     * @brief 设备状态变更时切换标签页的显示
     */
    void onDeviceStatusChanged(const QString &deviceId, DeviceStatus status);

    /**
     * @brief 导出当前选中设备的数据为 CSV
     */
    void exportCurrentTab();

private slots:
    void onTabChanged(int index);
    void onAutoRefreshTimer();

private:
    /**
     * @brief 查找或创建指定设备的表
     */
    QTableWidget *ensureTable(const QString &deviceId);

    /**
     * @brief 向指定表中添加一行或更新一行
     */
    void upsertRow(QTableWidget *table, quint16 addr,
                   double value, const QString &name, const QString &unit);

    /**
     * @brief 对指定单元格应用闪烁高亮
     */
    void flashCell(QTableWidget *table, int row, int col);

    /**
     * @brief 根据设备 ID 查找标签页索引
     */
    int indexOfDevice(const QString &deviceId) const;

private:
    QTabWidget                               *m_tabs        = nullptr;
    QTimer                                   *m_refreshTimer = nullptr;

    // 每个设备对应的表格
    QMap<QString, QTableWidget*>              m_tables;

    // 每设备每寄存器的上次值（用于变化比较）
    QMap<QString, QMap<quint16, double>>      m_lastValues;

    // 闪烁高亮的初始色
    QColor                                    m_flashColor = QColor(255, 255, 150);

    // 列枚举
    enum Columns {
        ColAddr   = 0,
        ColName   = 1,
        ColValue  = 2,
        ColUnit   = 3,
        ColStatus = 4,
        ColUpdate = 5,
        ColCount
    };
};

#endif // DEBUGPLATFORM_DATAMONITORWIDGET_H
