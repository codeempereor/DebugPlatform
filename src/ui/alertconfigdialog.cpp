#include "ui/alertconfigdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QTableWidgetItem>
#include <QSpinBox>
#include "device/devicemanager.h"

AlertConfigDialog::AlertConfigDialog(const QString &deviceId,
                                     const QList<AlertRule> &rules,
                                     QWidget *parent)
    : QDialog(parent)
    , m_deviceId(deviceId)
{
    const DeviceConfig cfg = DeviceManager::instance().getDeviceConfig(deviceId);
    const QString displayName = (!cfg.name.isEmpty() && cfg.name != cfg.id)
        ? QStringLiteral("%1 (%2)").arg(cfg.name, deviceId)
        : deviceId;
    setWindowTitle(tr("告警配置 - %1").arg(displayName));
    resize(520, 420);
    setupUi();
    populateTable(rules);
}

// ============================================================================
// Public API
// ============================================================================

QList<AlertRule> AlertConfigDialog::rules() const
{
    QList<AlertRule> out;
    for (int r = 0; r < m_table->rowCount(); ++r) {
        AlertRule rule;

        // Read from cell widgets (consistent with populateTable/onAccepted)
        // Fix: addr/min/max columns use QSpinBox/QDoubleSpinBox cell widgets,
        // values are NOT stored in QTableWidgetItem data(UserRole).
        // Old code read from Item.data() which always returned 0, causing all
        // 3 rules to overlap on addr=0 instead of their configured addresses.
        auto *addrSpin = qobject_cast<QSpinBox*>(m_table->cellWidget(r, ColAddr));
        rule.registerAddress = addrSpin
            ? static_cast<quint16>(addrSpin->value()) : 0;

        rule.name = m_table->item(r, ColName)
            ? m_table->item(r, ColName)->text() : QString{};

        auto *minSpin = qobject_cast<QDoubleSpinBox*>(m_table->cellWidget(r, ColMin));
        rule.minValue = minSpin ? minSpin->value() : 0.0;

        auto *maxSpin = qobject_cast<QDoubleSpinBox*>(m_table->cellWidget(r, ColMax));
        rule.maxValue = maxSpin ? maxSpin->value() : 0.0;

        auto *enableItem = m_table->item(r, ColEnable);
        rule.enabled = enableItem
            && enableItem->checkState() == Qt::Checked;

        out.append(rule);
    }
    return out;
}

void AlertConfigDialog::addRule()
{
    const int row = m_table->rowCount();
    m_table->insertRow(row);

    auto *addrSpin = new QSpinBox(m_table);
    addrSpin->setRange(0, 65535);
    addrSpin->setValue(0);
    m_table->setCellWidget(row, ColAddr, addrSpin);

    auto *nameItem = new QTableWidgetItem(tr("Register_%1").arg(row + 1));
    m_table->setItem(row, ColName, nameItem);

    auto *minSpin = new QDoubleSpinBox(m_table);
    minSpin->setRange(-1e9, 1e9);
    minSpin->setDecimals(3);
    minSpin->setValue(0.0);
    m_table->setCellWidget(row, ColMin, minSpin);

    auto *maxSpin = new QDoubleSpinBox(m_table);
    maxSpin->setRange(-1e9, 1e9);
    maxSpin->setDecimals(3);
    maxSpin->setValue(100.0);
    m_table->setCellWidget(row, ColMax, maxSpin);

    auto *enableItem = new QTableWidgetItem();
    enableItem->setFlags(enableItem->flags() | Qt::ItemIsUserCheckable);
    enableItem->setCheckState(Qt::Checked);
    m_table->setItem(row, ColEnable, enableItem);
}

void AlertConfigDialog::removeSelectedRule()
{
    const auto selected = m_table->selectedItems();
    QSet<int> rows;
    for (auto *item : selected) rows.insert(item->row());
    if (rows.isEmpty()) {
        // Fallback: remove last row if nothing selected
        if (m_table->rowCount() > 0) rows.insert(m_table->rowCount() - 1);
    }
    const QList<int> sorted = rows.values();
    for (int i = sorted.size() - 1; i >= 0; --i) {
        m_table->removeRow(sorted[i]);
    }
}

// ============================================================================
// Private implementation
// ============================================================================

void AlertConfigDialog::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);

    // Description
    mainLayout->addWidget(new QLabel(
        tr("Configure register threshold alerts for device <b>%1</b>. "
           "Alerts trigger when value falls outside [min, max] range.")
            .arg(m_deviceId), this));

    // Table
    m_table = new QTableWidget(this);
    m_table->setColumnCount(ColCount);
    m_table->setHorizontalHeaderLabels(QStringList{
        tr("Register Addr"), tr("Name"), tr("Min"), tr("Max"), tr("Enable")
    });
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::DoubleClicked
                           | QAbstractItemView::EditKeyPressed);
    mainLayout->addWidget(m_table, 1);

    // Buttons
    auto *btnRow = new QHBoxLayout();
    m_btnAdd    = new QPushButton(tr("Add Rule"), this);
    m_btnRemove = new QPushButton(tr("Remove Selected"), this);
    btnRow->addWidget(m_btnAdd);
    btnRow->addWidget(m_btnRemove);
    btnRow->addStretch();
    mainLayout->addLayout(btnRow);

    m_buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(m_buttonBox);

    connect(m_btnAdd,    &QPushButton::clicked,
            this, &AlertConfigDialog::addRule);
    connect(m_btnRemove, &QPushButton::clicked,
            this, &AlertConfigDialog::removeSelectedRule);
    connect(m_buttonBox, &QDialogButtonBox::accepted,
            this, &AlertConfigDialog::onAccepted);
    connect(m_buttonBox, &QDialogButtonBox::rejected,
            this, &QDialog::reject);
}

void AlertConfigDialog::populateTable(const QList<AlertRule> &rules)
{
    m_table->setRowCount(0);
    for (const AlertRule &r : rules) {
        addRule();
        const int row = m_table->rowCount() - 1;

        auto *addrSpin = qobject_cast<QSpinBox*>(
            m_table->cellWidget(row, ColAddr));
        if (addrSpin) addrSpin->setValue(r.registerAddress);

        auto *nameItem = m_table->item(row, ColName);
        if (nameItem) nameItem->setText(r.name);

        auto *minSpin = qobject_cast<QDoubleSpinBox*>(
            m_table->cellWidget(row, ColMin));
        if (minSpin) minSpin->setValue(r.minValue);

        auto *maxSpin = qobject_cast<QDoubleSpinBox*>(
            m_table->cellWidget(row, ColMax));
        if (maxSpin) maxSpin->setValue(r.maxValue);

        auto *enableItem = m_table->item(row, ColEnable);
        if (enableItem) {
            enableItem->setCheckState(
                r.enabled ? Qt::Checked : Qt::Unchecked);
        }
    }
}

void AlertConfigDialog::onAccepted()
{
    for (int r = 0; r < m_table->rowCount(); ++r) {
        auto *addrSpin = qobject_cast<QSpinBox*>(
            m_table->cellWidget(r, ColAddr));
        auto *minSpin  = qobject_cast<QDoubleSpinBox*>(
            m_table->cellWidget(r, ColMin));
        auto *maxSpin  = qobject_cast<QDoubleSpinBox*>(
            m_table->cellWidget(r, ColMax));
        if (!addrSpin || !minSpin || !maxSpin) continue;
        if (minSpin->value() > maxSpin->value()) {
            QMessageBox::warning(this, tr("Validation Error"),
                tr("Row %1: min value must not exceed max value.").arg(r + 1));
            return;
        }
    }
    accept();
}
