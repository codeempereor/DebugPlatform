#include "ui/historyquerydialog.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QHeaderView>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QTableWidgetItem>
#include "device/devicemanager.h"

HistoryQueryDialog::HistoryQueryDialog(const QStringList &deviceIds,
                                       QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("历史数据查询"));
    resize(720, 520);
    setupUi();

    for (const QString &id : deviceIds) {
        const DeviceConfig cfg = DeviceManager::instance().getDeviceConfig(id);
        const QString displayText = (!cfg.name.isEmpty() && cfg.name != cfg.id)
            ? QStringLiteral("%1 (%2)").arg(cfg.name, id)
            : id;
        m_deviceCombo->addItem(displayText, id);
    }

    // 默认时间范围：最近 1 小时
    const QDateTime now = QDateTime::currentDateTime();
    m_endEdit->setDateTime(now);
    m_startEdit->setDateTime(now.addSecs(-3600));
}

// ============================================================================
// 公共 API
// ============================================================================

QVector<HistoryRow> HistoryQueryDialog::results() const
{
    return m_results;
}

void HistoryQueryDialog::setResults(const QVector<HistoryRow> &rows)
{
    m_results = rows;
    m_resultTable->setRowCount(0);
    for (const HistoryRow &r : rows) {
        const int row = m_resultTable->rowCount();
        m_resultTable->insertRow(row);

        auto *timeItem = new QTableWidgetItem(
            r.timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz"));
        m_resultTable->setItem(row, ColTime, timeItem);

        const DeviceConfig cfg = DeviceManager::instance().getDeviceConfig(r.deviceId);
        const QString devDisplay = (!cfg.name.isEmpty() && cfg.name != cfg.id)
            ? QStringLiteral("%1").arg(cfg.name)
            : r.deviceId;
        auto *deviceItem = new QTableWidgetItem(devDisplay);
        m_resultTable->setItem(row, ColDevice, deviceItem);

        auto *addrItem = new QTableWidgetItem(
            QStringLiteral("0x%1").arg(r.registerAddress, 4, 16, QLatin1Char('0')).toUpper());
        m_resultTable->setItem(row, ColAddr, addrItem);

        auto *valueItem = new QTableWidgetItem(
            QString::number(r.value, 'g', 6));
        m_resultTable->setItem(row, ColValue, valueItem);

        auto *unitItem = new QTableWidgetItem(r.unit);
        m_resultTable->setItem(row, ColUnit, unitItem);
    }
}

// ============================================================================
// 槽
// ============================================================================

void HistoryQueryDialog::onQueryClicked()
{
    const QDateTime start = m_startEdit->dateTime();
    const QDateTime end   = m_endEdit->dateTime();
    if (start >= end) {
        QMessageBox::warning(this, tr("校验失败"),
            tr("起始时间必须早于结束时间。"));
        return;
    }
    const QString deviceId = m_deviceCombo->currentData().toString();
    const quint16 addrBegin = static_cast<quint16>(m_addrBeginSpin->value());
    const quint16 addrEnd   = static_cast<quint16>(m_addrEndSpin->value());
    if (addrBegin > addrEnd) {
        QMessageBox::warning(this, tr("校验失败"),
            tr("寄存器地址起始不得大于结束。"));
        return;
    }
    emit queryRequested(start, end, deviceId, addrBegin, addrEnd);
}

void HistoryQueryDialog::onExportClicked()
{
    if (m_results.isEmpty()) {
        QMessageBox::information(this, tr("提示"), tr("当前没有可导出的结果。"));
        return;
    }
    const QString defaultName = QStringLiteral("history_%1.csv")
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    const QString path = QFileDialog::getSaveFileName(
        this, tr("导出 CSV"), defaultName, tr("CSV 文件 (*.csv)"));
    if (path.isEmpty()) return;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("导出失败"),
                             tr("无法写入文件：%1").arg(path));
        return;
    }
    // 写 UTF-8 BOM，避免 Excel 打开中文乱码
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
    ts << "Timestamp,Device,Address,Value,Unit\n";
    for (const HistoryRow &r : m_results) {
        ts << escape(r.timestamp.toString(Qt::ISODate)) << ','
           << escape(r.deviceId) << ','
           << QStringLiteral("0x%1").arg(r.registerAddress, 4, 16, QLatin1Char('0')).toUpper() << ','
           << QString::number(r.value, 'g', 6) << ','
           << escape(r.unit) << '\n';
    }
    f.close();
    emit exportRequested();
    QMessageBox::information(this, tr("导出成功"),
        tr("已导出 %1 条记录到：\n%2").arg(m_results.size()).arg(path));
}

void HistoryQueryDialog::onQuickRangeClicked(int mode)
{
    applyQuickRange(mode);
}

void HistoryQueryDialog::applyQuickRange(int mode)
{
    const QDateTime now = QDateTime::currentDateTime();
    QDateTime start;
    QDateTime end   = now;
    switch (mode) {
    case 0:  // 最近 1 小时
        start = now.addSecs(-3600);
        break;
    case 1:  // 最近 6 小时
        start = now.addSecs(-6 * 3600);
        break;
    case 2: { // 今天
        const QDate today = QDate::currentDate();
        start = QDateTime(today, QTime(0, 0, 0));
        end   = QDateTime(today, QTime(23, 59, 59, 999));
        break;
    }
    case 3: { // 昨天
        const QDate y = QDate::currentDate().addDays(-1);
        start = QDateTime(y, QTime(0, 0, 0));
        end   = QDateTime(y, QTime(23, 59, 59, 999));
        break;
    }
    case 4:  // 最近 7 天
        start = now.addDays(-7);
        break;
    case 5:  // 最近 30 天
        start = now.addDays(-30);
        break;
    default:
        return;
    }
    m_startEdit->setDateTime(start);
    m_endEdit->setDateTime(end);
}

// ============================================================================
// 私有实现
// ============================================================================

void HistoryQueryDialog::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);

    auto *filterGroup = new QGroupBox(tr("查询条件"), this);
    auto *form = new QFormLayout(filterGroup);

    m_startEdit = new QDateTimeEdit(filterGroup);
    m_startEdit->setCalendarPopup(true);
    m_startEdit->setDisplayFormat(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    form->addRow(tr("起始时间："), m_startEdit);

    m_endEdit = new QDateTimeEdit(filterGroup);
    m_endEdit->setCalendarPopup(true);
    m_endEdit->setDisplayFormat(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    form->addRow(tr("结束时间："), m_endEdit);

    m_deviceCombo = new QComboBox(filterGroup);
    m_deviceCombo->addItem(tr("全部设备"), QString{});
    form->addRow(tr("设备："), m_deviceCombo);

    auto *addrRow = new QHBoxLayout();
    m_addrBeginSpin = new QSpinBox(filterGroup);
    m_addrBeginSpin->setRange(0, 65535);
    m_addrBeginSpin->setValue(0);
    m_addrBeginSpin->setDisplayIntegerBase(16);
    addrRow->addWidget(m_addrBeginSpin);
    addrRow->addWidget(new QLabel(tr("至")));
    m_addrEndSpin = new QSpinBox(filterGroup);
    m_addrEndSpin->setRange(0, 65535);
    m_addrEndSpin->setValue(255);
    m_addrEndSpin->setDisplayIntegerBase(16);
    addrRow->addWidget(m_addrEndSpin);
    addrRow->addStretch();
    form->addRow(tr("寄存器地址："), addrRow);

    // -- 快速时间区间按钮行 --
    auto *quickRow = new QHBoxLayout();
    const QStringList labels{
        tr("最近1小时"), tr("最近6小时"), tr("今天"),
        tr("昨天"),      tr("最近7天"),  tr("最近30天")
    };
    for (int i = 0; i < labels.size(); ++i) {
        auto *btn = new QPushButton(labels[i], filterGroup);
        btn->setFlat(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(QStringLiteral(
            "QPushButton{padding:2px 8px;color:#2B6CB0;}"
            "QPushButton:hover{color:#1A365D;text-decoration:underline;}"));
        const int mode = i;
        connect(btn, &QPushButton::clicked,
                this, [=] { onQuickRangeClicked(mode); });
        quickRow->addWidget(btn);
        m_quickBtns.append(btn);
    }
    quickRow->addStretch();
    form->addRow(tr("快捷选择："), quickRow);

    mainLayout->addWidget(filterGroup);

    // 查询 / 导出按钮
    auto *btnRow = new QHBoxLayout();
    m_queryBtn  = new QPushButton(tr("查询"), this);
    m_exportBtn = new QPushButton(tr("导出 CSV"), this);
    btnRow->addWidget(m_queryBtn);
    btnRow->addWidget(m_exportBtn);
    btnRow->addStretch();
    mainLayout->addLayout(btnRow);

    // 结果表格
    m_resultTable = new QTableWidget(this);
    m_resultTable->setColumnCount(ColCount);
    m_resultTable->setHorizontalHeaderLabels(QStringList{
        tr("时间"), tr("设备"), tr("寄存器地址"), tr("值"), tr("单位")
    });
    m_resultTable->horizontalHeader()->setStretchLastSection(true);
    m_resultTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_resultTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mainLayout->addWidget(m_resultTable, 1);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    mainLayout->addWidget(m_buttonBox);

    connect(m_queryBtn,  &QPushButton::clicked,
            this, &HistoryQueryDialog::onQueryClicked);
    connect(m_exportBtn, &QPushButton::clicked,
            this, &HistoryQueryDialog::onExportClicked);
    connect(m_buttonBox, &QDialogButtonBox::rejected,
            this, &QDialog::reject);

    // 初始显示空结果
    setResults({});
}
