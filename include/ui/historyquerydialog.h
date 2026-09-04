#ifndef DEBUGPLATFORM_HISTORYQUERYDIALOG_H
#define DEBUGPLATFORM_HISTORYQUERYDIALOG_H

/**
 * @file historyquerydialog.h
 * @brief 历史数据查询对话框
 *
 * 允许用户选择时间范围、设备、寄存器地址范围，
 * 并以 QTableWidget 展示查询结果。实际数据查询由业务层
 *（DataPersistent / PlaybackEngine）执行；本对话框仅保留 UI。
 */

#include <QDialog>
#include <QDateTimeEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QTableWidget>
#include <QDialogButtonBox>
#include <QStringList>
#include <QVector>
#include "core/config.h"

/**
 * @struct HistoryRow
 * @brief 查询结果行
 */
struct HistoryRow {
    QDateTime timestamp;
    QString   deviceId;
    quint16   registerAddress = 0;
    double    value           = 0.0;
    QString   unit;
};

/**
 * @class HistoryQueryDialog
 * @brief 历史数据查询对话框
 */
class HistoryQueryDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param deviceIds 可选设备 ID 列表（用于设备下拉框）
     * @param parent 父对象
     */
    explicit HistoryQueryDialog(const QStringList &deviceIds = {},
                                QWidget *parent = nullptr);

    /**
     * @brief 获取当前查询结果（供外部调用）
     */
    QVector<HistoryRow> results() const;

    /**
     * @brief 由外部（业务层）调用以填充结果
     */
    void setResults(const QVector<HistoryRow> &rows);

signals:
    /**
     * @brief 用户点击"查询"按钮后触发
     */
    void queryRequested(const QDateTime &start, const QDateTime &end,
                        const QString &deviceId,
                        quint16 addrBegin, quint16 addrEnd);

    /**
     * @brief 用户点击导出按钮
     */
    void exportRequested();

private slots:
    void onQueryClicked();
    void onExportClicked();
    void onQuickRangeClicked(int mode); // 0=1h,1=6h,2=today,3=yesterday,4=7d,5=30d

private:
    void setupUi();
    void applyQuickRange(int mode);

private:
    QDateTimeEdit    *m_startEdit    = nullptr;
    QDateTimeEdit    *m_endEdit      = nullptr;
    QComboBox        *m_deviceCombo  = nullptr;
    QSpinBox         *m_addrBeginSpin = nullptr;
    QSpinBox         *m_addrEndSpin  = nullptr;

    QPushButton      *m_queryBtn     = nullptr;
    QPushButton      *m_exportBtn    = nullptr;
    QList<QPushButton*> m_quickBtns;  ///< 快速时间区间按钮组（顺序同 mode）

    QTableWidget     *m_resultTable  = nullptr;
    QDialogButtonBox *m_buttonBox    = nullptr;

    QVector<HistoryRow> m_results;

    enum Columns {
        ColTime = 0,
        ColDevice,
        ColAddr,
        ColValue,
        ColUnit,
        ColCount
    };
};

#endif // DEBUGPLATFORM_HISTORYQUERYDIALOG_H
