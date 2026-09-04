#ifndef DEBUGPLATFORM_ALERTCONFIGDIALOG_H
#define DEBUGPLATFORM_ALERTCONFIGDIALOG_H

/**
 * @file alertconfigdialog.h
 * @brief 告警配置对话框
 *
 * 允许用户为一组寄存器配置阈值上下限与启用开关。
 * 对话框内部以 QTableWidget 形式展示寄存器条目，
 * 每行包含：地址、名称、下限、上限、启用。
 */

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QStringList>
#include <QMap>

/**
 * @struct AlertRule
 * @brief 单条告警规则
 */
struct AlertRule {
    quint16 registerAddress = 0;
    QString  name;
    double   minValue       = 0.0;
    double   maxValue       = 100.0;
    bool     enabled        = true;
};

/**
 * @class AlertConfigDialog
 * @brief 告警配置对话框
 */
class AlertConfigDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param deviceId 目标设备 ID（仅用于标题/显示）
     * @param rules 初始规则集合
     * @param parent 父对象
     */
    explicit AlertConfigDialog(const QString &deviceId,
                              const QList<AlertRule> &rules = {},
                              QWidget *parent = nullptr);

    /**
     * @brief 获取编辑后的规则列表
     */
    QList<AlertRule> rules() const;

public slots:
    void addRule();
    void removeSelectedRule();

private slots:
    void onAccepted();

private:
    void setupUi();
    void populateTable(const QList<AlertRule> &rules);

private:
    QString           m_deviceId;
    QTableWidget     *m_table       = nullptr;
    QPushButton       *m_btnAdd     = nullptr;
    QPushButton       *m_btnRemove  = nullptr;
    QDialogButtonBox  *m_buttonBox  = nullptr;

    enum Columns {
        ColAddr   = 0,
        ColName   = 1,
        ColMin    = 2,
        ColMax    = 3,
        ColEnable = 4,
        ColCount
    };
};

#endif // DEBUGPLATFORM_ALERTCONFIGDIALOG_H
