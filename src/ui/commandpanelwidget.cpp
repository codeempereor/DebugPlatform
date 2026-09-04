#include "ui/commandpanelwidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QDateTime>
#include "protocol/binaryframebuilder.h"
#include "device/devicemanager.h"

CommandPanelWidget::CommandPanelWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);

    // 目标设备标签
    m_targetLabel = new QLabel(tr("目标设备：<i>未选择</i>"), this);
    layout->addWidget(m_targetLabel);

    m_tabs = new QTabWidget(this);
    layout->addWidget(m_tabs, 1);

    // ========================================================================
    // Tab 1: 自定义二进制帧
    // ========================================================================
    {
        auto *tab = new QWidget(this);
        auto *vlay = new QVBoxLayout(tab);

        vlay->addWidget(new QLabel(tr("十六进制字节（空格分隔，例如：01 03 00 00 00 0A）：")));
        m_hexEdit = new QLineEdit(tab);
        m_hexEdit->setPlaceholderText(tr("输入十六进制字节"));
        vlay->addWidget(m_hexEdit);

        m_previewLabel = new QLabel(tab);
        m_previewLabel->setWordWrap(true);
        m_previewLabel->setTextFormat(Qt::PlainText);
        vlay->addWidget(new QLabel(tr("预览（含 CRC16）：")));
        vlay->addWidget(m_previewLabel);

        auto *row = new QHBoxLayout();
        row->addWidget(new QLabel(tr("重复次数：")));
        m_repeatSpin = new QSpinBox(tab);
        m_repeatSpin->setRange(1, 10000);
        m_repeatSpin->setValue(1);
        row->addWidget(m_repeatSpin);
        row->addStretch();

        m_sendBtn = new QPushButton(tr("发送"), tab);
        row->addWidget(m_sendBtn);
        vlay->addLayout(row);
        vlay->addStretch();

        connect(m_hexEdit, &QLineEdit::textChanged,
                this, &CommandPanelWidget::onBinaryTextChanged);
        connect(m_sendBtn, &QPushButton::clicked,
                this, &CommandPanelWidget::onSendBinaryClicked);
        connect(m_repeatSpin, QOverload<int>::of(&QSpinBox::valueChanged),
                this, &CommandPanelWidget::onRepeatValueChanged);

        m_tabs->addTab(tab, tr("二进制帧"));
    }

    // ========================================================================
    // Tab 2: Modbus 快捷操作
    // ========================================================================
    {
        auto *tab = new QWidget(this);
        auto *grid = new QGridLayout(tab);

        grid->addWidget(new QLabel(tr("功能码：")), 0, 0);
        m_functionCombo = new QComboBox(tab);
        m_functionCombo->addItem(tr("读线圈 (0x01)"),    ReadCoils);
        m_functionCombo->addItem(tr("读离散输入 (0x02)"), ReadDiscreteInputs);
        m_functionCombo->addItem(tr("读保持寄存器 (0x03)"), ReadHoldingRegisters);
        m_functionCombo->addItem(tr("读输入寄存器 (0x04)"), ReadInputRegisters);
        m_functionCombo->addItem(tr("写单线圈 (0x05)"),  WriteSingleCoil);
        m_functionCombo->addItem(tr("写单寄存器 (0x06)"), WriteSingleRegister);
        m_functionCombo->addItem(tr("写多线圈 (0x0F)"),  WriteMultipleCoils);
        m_functionCombo->addItem(tr("写多寄存器 (0x10)"), WriteMultipleRegisters);
        grid->addWidget(m_functionCombo, 0, 1, 1, 3);

        grid->addWidget(new QLabel(tr("从站地址：")), 1, 0);
        m_slaveSpin = new QSpinBox(tab);
        m_slaveSpin->setRange(0, 247);
        m_slaveSpin->setValue(1);
        grid->addWidget(m_slaveSpin, 1, 1);

        grid->addWidget(new QLabel(tr("寄存器地址：")), 2, 0);
        m_regAddrSpin = new QSpinBox(tab);
        m_regAddrSpin->setRange(0, 65535);
        m_regAddrSpin->setValue(0);
        grid->addWidget(m_regAddrSpin, 2, 1);

        grid->addWidget(new QLabel(tr("数量：")), 3, 0);
        m_quantitySpin = new QSpinBox(tab);
        m_quantitySpin->setRange(1, 65535);
        m_quantitySpin->setValue(1);
        grid->addWidget(m_quantitySpin, 3, 1);

        grid->addWidget(new QLabel(tr("值（写入时）：")), 4, 0);
        m_valueSpin = new QSpinBox(tab);
        m_valueSpin->setRange(-32768, 65535);
        grid->addWidget(m_valueSpin, 4, 1);

        m_modbusHint = new QLabel(tab);
        m_modbusHint->setWordWrap(true);
        grid->addWidget(m_modbusHint, 5, 0, 1, 4);

        m_modbusSendBtn = new QPushButton(tr("发送 Modbus"), tab);
        grid->addWidget(m_modbusSendBtn, 6, 0, 1, 4);

        grid->setRowStretch(7, 1);

        connect(m_functionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &CommandPanelWidget::onModbusFunctionChanged);
        connect(m_modbusSendBtn, &QPushButton::clicked,
                this, &CommandPanelWidget::onModbusSendClicked);

        onModbusFunctionChanged(0);
        m_tabs->addTab(tab, tr("Modbus"));
    }

    // ========================================================================
    // Tab 3: 历史
    // ========================================================================
    {
        auto *tab = new QWidget(this);
        auto *vlay = new QVBoxLayout(tab);

        m_historyList = new QListWidget(tab);
        vlay->addWidget(m_historyList, 1);

        auto *row = new QHBoxLayout();
        row->addStretch();
        m_clearHistBtn = new QPushButton(tr("清空历史"), tab);
        row->addWidget(m_clearHistBtn);
        vlay->addLayout(row);

        connect(m_clearHistBtn, &QPushButton::clicked,
                this, &CommandPanelWidget::onClearHistory);

        m_tabs->addTab(tab, tr("历史"));
    }

    updatePreview();
}

// ============================================================================
// 公共 API
// ============================================================================

void CommandPanelWidget::setDeviceId(const QString &deviceId)
{
    m_deviceId = deviceId;
    if (deviceId.isEmpty()) {
        m_targetLabel->setText(tr("目标设备：<i>未选择</i>"));
        m_sendBtn->setEnabled(false);
        m_modbusSendBtn->setEnabled(false);
    } else {
        // 优先显示「设备名 (UUID)」：设备名是用户取的，比 UUID 直观；
        // 如果 name 为空或 name==id 就退化成纯 UUID 显示。
        const DeviceConfig cfg =
            DeviceManager::instance().getDeviceConfig(deviceId);
        if (!cfg.name.isEmpty() && cfg.name != cfg.id) {
            m_targetLabel->setText(
                tr("目标设备：<b>%1</b> (%2)").arg(cfg.name, deviceId));
        } else {
            m_targetLabel->setText(tr("目标设备：<b>%1</b>").arg(deviceId));
        }
        m_sendBtn->setEnabled(true);
        m_modbusSendBtn->setEnabled(true);
    }
}

void CommandPanelWidget::appendHistory(const QString &command,
                                       const QString &response,
                                       int responseTime)
{
    const QString ts = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    const QString summary = tr("[%1] TX: %2  |  RX: %3  |  %4 ms")
        .arg(ts, command, response).arg(responseTime);
    auto *item = new QListWidgetItem;
    item->setText(summary);
    item->setToolTip(summary);
    m_historyList->addItem(item);
    m_historyList->scrollToBottom();
}

// ============================================================================
// Tab 1 槽
// ============================================================================

void CommandPanelWidget::onBinaryTextChanged()
{
    updatePreview();
}

void CommandPanelWidget::onSendBinaryClicked()
{
    if (m_deviceId.isEmpty()) {
        QMessageBox::information(this, tr("提示"),
            tr("请先在左侧设备树中选择一个设备。"));
        return;
    }
    const QByteArray payload = fromHexString(m_hexEdit->text());
    if (payload.isEmpty()) {
        QMessageBox::warning(this, tr("错误"),
            tr("十六进制字符串为空或格式不正确。"));
        return;
    }

    // 使用 BinaryFrameBuilder 构建完整帧（自动添加帧头、长度、CRC、帧尾）
    QByteArray frame = BinaryFrameBuilder::buildFrame(payload);

    const int times = m_repeatSpin->value();
    for (int i = 0; i < times; ++i) {
        emit sendBinaryFrameRequested(frame);
    }
    // 历史记录由 MainWindow lambda 统一处理，避免重复添加
}

void CommandPanelWidget::onRepeatValueChanged(int value)
{
    Q_UNUSED(value);
}

// ============================================================================
// Tab 2 槽
// ============================================================================

void CommandPanelWidget::onModbusSendClicked()
{
    if (m_deviceId.isEmpty()) {
        QMessageBox::information(this, tr("提示"),
            tr("请先在左侧设备树中选择一个设备。"));
        return;
    }

    const auto func = static_cast<ModbusFunction>(
        m_functionCombo->currentData().toInt());
    quint8 fc = 0;
    switch (func) {
    case ReadCoils:          fc = 0x01; break;
    case ReadDiscreteInputs: fc = 0x02; break;
    case ReadHoldingRegisters: fc = 0x03; break;
    case ReadInputRegisters: fc = 0x04; break;
    case WriteSingleCoil:    fc = 0x05; break;
    case WriteSingleRegister: fc = 0x06; break;
    case WriteMultipleCoils: fc = 0x0F; break;
    case WriteMultipleRegisters: fc = 0x10; break;
    }

    quint16 addr    = static_cast<quint16>(m_regAddrSpin->value());
    quint16 qty     = static_cast<quint16>(m_quantitySpin->value());
    quint16 val     = static_cast<quint16>(m_valueSpin->value() & 0xFFFF);
    quint8  slave   = static_cast<quint8>(m_slaveSpin->value());

    emit sendModbusCommandRequested(fc, addr, qty, val, slave);
}

void CommandPanelWidget::onModbusFunctionChanged(int index)
{
    Q_UNUSED(index);
    const auto func = static_cast<ModbusFunction>(
        m_functionCombo->currentData().toInt());
    switch (func) {
    case ReadCoils:
    case ReadDiscreteInputs:
    case ReadHoldingRegisters:
    case ReadInputRegisters:
        m_quantitySpin->setEnabled(true);
        m_valueSpin->setEnabled(false);
        m_modbusHint->setText(tr("读取：返回数量个寄存器/线圈。"));
        break;
    case WriteSingleCoil:
    case WriteSingleRegister:
        m_quantitySpin->setEnabled(false);
        m_valueSpin->setEnabled(true);
        m_modbusHint->setText(tr("写单个：值参数为写入值。"));
        break;
    case WriteMultipleCoils:
    case WriteMultipleRegisters:
        m_quantitySpin->setEnabled(true);
        m_valueSpin->setEnabled(true);
        m_modbusHint->setText(tr("写多个：数量为写入项数；值参数仅作为首值参考，具体多寄存器写入需逐字节拼接。"));
        break;
    }
}

// ============================================================================
// Tab 3 槽
// ============================================================================

void CommandPanelWidget::onClearHistory()
{
    m_historyList->clear();
    emit sendHistoryCleared();
}

// ============================================================================
// 辅助方法
// ============================================================================

QByteArray CommandPanelWidget::fromHexString(const QString &hex)
{
    QString cleaned;
    cleaned.reserve(hex.size());
    for (const QChar &c : hex) {
        if (c.isSpace()) continue;
        cleaned.append(c);
    }
    if (cleaned.size() % 2 != 0) return {};

    QByteArray out;
    out.reserve(cleaned.size() / 2);
    bool ok = false;
    for (int i = 0; i < cleaned.size(); i += 2) {
        const QString part = cleaned.mid(i, 2);
        const quint8 v = part.toUInt(&ok, 16);
        if (!ok) return {};
        out.append(static_cast<char>(v));
    }
    return out;
}

QByteArray CommandPanelWidget::crc16Modbus(const QByteArray &data)
{
    quint16 crc = 0xFFFF;
    for (int i = 0; i < data.size(); ++i) {
        crc ^= static_cast<quint8>(data[i]);
        for (int j = 0; j < 8; ++j) {
            if (crc & 0x0001) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    // Modbus: 低字节在前
    QByteArray result(2, Qt::Uninitialized);
    result[0] = static_cast<char>(crc & 0xFF);
    result[1] = static_cast<char>((crc >> 8) & 0xFF);
    return result;
}

void CommandPanelWidget::updatePreview()
{
    const QByteArray payload = fromHexString(m_hexEdit->text());
    if (payload.isEmpty()) {
        m_previewLabel->setText(tr("（等待输入…）"));
        return;
    }
    // 使用 BinaryFrameBuilder 构建完整帧用于预览
    QByteArray frame = BinaryFrameBuilder::buildFrame(payload);

    const QString hex = QString::fromLatin1(frame.toHex(' ').toUpper());
    m_previewLabel->setText(hex);
}
