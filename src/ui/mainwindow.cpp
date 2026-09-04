#include "ui/mainwindow.h"
#include "ui/devicetreewidget.h"
#include "ui/datamonitorwidget.h"
#include "ui/commandpanelwidget.h"
#include "ui/logwidget.h"
#include "ui/adddevicedialog.h"
#include "ui/configdialog.h"
#include "ui/historyquerydialog.h"
#include "ui/alertconfigdialog.h"
#include "device/devicemanager.h"
#include "data/logmanager.h"
#include "data/alertmanager.h"
#include "app/servicelocator.h"
#include "protocol/modbustcpprotocol.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QStatusBar>
#include <QToolBar>
#include <QSplitter>
#include <QDateTime>
#include <QElapsedTimer>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonValue>
#include <QEvent>
#include <QMouseEvent>
#include <QScrollArea>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QStringList>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
    setupToolbar();
    setupStatusBar();
    connectSignals();
    loadAlertCounterFromDb();

    setWindowTitle(QStringLiteral("%1 v%2")
        .arg(DEBUGPLATFORM_NAME, DEBUGPLATFORM_VERSION));
    resize(1280, 800);
}

MainWindow::~MainWindow() = default;

bool MainWindow::eventFilter(QObject *obj, QEvent *evt)
{
    if (obj == m_alertCountLabel && evt->type() == QEvent::MouseButtonRelease) {
        auto *me = static_cast<QMouseEvent *>(evt);
        if (me->button() == Qt::LeftButton) {
            onAlertCounterClicked();
            return true;
        }
    }
    return QMainWindow::eventFilter(obj, evt);
}

void MainWindow::setupUi()
{
    // 子组件
    m_deviceTree   = new DeviceTreeWidget(this);
    m_dataMonitor  = new DataMonitorWidget(this);
    m_commandPanel = new CommandPanelWidget(this);
    m_logWidget    = new LogWidget(this);

    // 左侧垂直：设备树 + 命令面板
    m_leftSplitter = new QSplitter(Qt::Vertical, this);
    m_leftSplitter->addWidget(m_deviceTree);
    m_leftSplitter->addWidget(m_commandPanel);
    m_leftSplitter->setStretchFactor(0, 3);
    m_leftSplitter->setStretchFactor(1, 2);

    // 主水平分栏：左侧 | 中间数据监控
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainSplitter->addWidget(m_leftSplitter);
    m_mainSplitter->addWidget(m_dataMonitor);
    m_mainSplitter->setStretchFactor(0, 2);
    m_mainSplitter->setStretchFactor(1, 5);

    // 垂直分栏：上方主内容 | 底部日志
    m_verticalSplitter = new QSplitter(Qt::Vertical, this);
    m_verticalSplitter->addWidget(m_mainSplitter);
    m_verticalSplitter->addWidget(m_logWidget);
    m_verticalSplitter->setStretchFactor(0, 8);
    m_verticalSplitter->setStretchFactor(1, 2);

    setCentralWidget(m_verticalSplitter);
}

void MainWindow::setupToolbar()
{
    m_toolBar = addToolBar(tr("主工具栏"));
    m_toolBar->setMovable(false);

    m_actAddDevice = m_toolBar->addAction(tr("添加设备"));
    m_actAddDevice->setToolTip(tr("注册一个新的设备"));

    m_actConfig = m_toolBar->addAction(tr("配置"));
    m_actConfig->setToolTip(tr("打开当前选中设备的配置对话框"));
    m_actConfig->setEnabled(false);

    m_toolBar->addSeparator();

    m_actHistory = m_toolBar->addAction(tr("历史查询"));
    m_actHistory->setToolTip(tr("按时间范围查询寄存器历史数据"));

    m_actAlertCfg = m_toolBar->addAction(tr("告警配置"));
    m_actAlertCfg->setToolTip(tr("配置寄存器阈值告警"));
    m_actAlertCfg->setEnabled(false);

    m_toolBar->addSeparator();

    m_actStartAll = m_toolBar->addAction(tr("全部启动"));
    m_actStartAll->setToolTip(tr("连接所有已启用的设备"));

    m_actStopAll = m_toolBar->addAction(tr("全部停止"));
    m_actStopAll->setToolTip(tr("断开所有设备"));

    m_toolBar->addSeparator();

    m_actToggleLog = m_toolBar->addAction(tr("显示/隐藏日志"));
    m_actToggleLog->setCheckable(true);
    m_actToggleLog->setChecked(true);

    m_toolBar->addSeparator();

    m_actAbout = m_toolBar->addAction(tr("关于"));
}

void MainWindow::setupStatusBar()
{
    m_statusBar = statusBar();

    m_deviceCountLabel = new QLabel(this);
    m_runtimeLabel     = new QLabel(this);
    m_alertCountLabel  = new QLabel(QStringLiteral("告警: 0"), this);
    m_alertCountLabel->setCursor(Qt::PointingHandCursor);
    m_alertCountLabel->setToolTip(tr("点击查看最近 10 条告警"));
    m_alertCountLabel->installEventFilter(this);

    m_statusBar->addWidget(m_deviceCountLabel, 1);
    m_statusBar->addPermanentWidget(m_runtimeLabel);
    m_statusBar->addPermanentWidget(m_alertCountLabel);

    refreshStatusBar();
    refreshAlertCountLabel();
}

void MainWindow::connectSignals()
{
    // 工具栏 -> 槽
    connect(m_actAddDevice,    &QAction::triggered, this, &MainWindow::onAddDevice);
    connect(m_actConfig,       &QAction::triggered, this, &MainWindow::onOpenConfig);
    connect(m_actHistory,      &QAction::triggered, this, &MainWindow::onOpenHistoryQuery);
    connect(m_actAlertCfg,     &QAction::triggered, this, &MainWindow::onOpenAlertConfig);
    connect(m_actStartAll,     &QAction::triggered, this, &MainWindow::onStartAll);
    connect(m_actStopAll,      &QAction::triggered, this, &MainWindow::onStopAll);
    connect(m_actToggleLog,    &QAction::toggled,   this, &MainWindow::onToggleLogView);
    connect(m_actAbout,        &QAction::triggered, this, &MainWindow::onAbout);

    // 业务层 -> MainWindow（经由 MainWindow 再分发到子组件）
    auto &dm = DeviceManager::instance();
    connect(&dm, &DeviceManager::deviceStatusChanged,
            this, &MainWindow::onDeviceStatusChanged);
    connect(&dm, &DeviceManager::deviceDataReceived,
            this, &MainWindow::onDataReceived);

    // 设备连接成功 → 自动在数据监控创建标签页
    connect(&dm, &DeviceManager::deviceConnected,
            this, [this](const QString &deviceId) {
                DeviceConfig cfg = DeviceManager::instance().getDeviceConfig(deviceId);
                m_dataMonitor->addDeviceTab(deviceId, cfg.name);
                m_deviceTree->refresh();
                refreshStatusBar();
            });

    // 设备断开 → 刷新设备树
    connect(&dm, &DeviceManager::deviceDisconnected,
            this, [this](const QString &deviceId) {
                Q_UNUSED(deviceId);
                m_deviceTree->refresh();
                refreshStatusBar();
            });

    auto &lm = LogManager::instance();
    connect(&lm, &LogManager::logMessage,
            this, &MainWindow::onLogMessage);

    // ServiceLocator → UI（协议解析后的寄存器更新 / 响应回显）
    auto &sl = ServiceLocator::instance();
    connect(&sl, &ServiceLocator::deviceRegisterUpdated,
            m_dataMonitor, &DataMonitorWidget::updateValue);
    connect(&sl, &ServiceLocator::modbusResponseReceived,
            this, [this](const QString &deviceId,
                         const QString &txHex,
                         const QString &rxHex,
                         int responseTimeMs) {
                if (deviceId == m_currentDeviceId) {
                    if (!txHex.isEmpty()) {
                        // 若携带 TX 历史一起回显（RTU/TCP），则一行完整
                        m_commandPanel->appendHistory(txHex, rxHex, responseTimeMs);
                    } else {
                        m_commandPanel->appendHistory(QString(), rxHex, responseTimeMs);
                    }
                } else {
                    Q_UNUSED(txHex);
                    m_commandPanel->appendHistory(QString(), rxHex, responseTimeMs);
                }
            });
    connect(&sl, &ServiceLocator::modbusRequestTimedOut,
            this, &MainWindow::onModbusRequestTimedOut);
    connect(&sl, &ServiceLocator::alertOccurred,
            this, &MainWindow::onAlertOccurred);

    // 子组件之间的协同
    connect(m_deviceTree, &DeviceTreeWidget::deviceSelected,
            this, &MainWindow::onDeviceSelected);
    connect(m_deviceTree, &DeviceTreeWidget::deviceStatusChanged,
            m_dataMonitor, &DataMonitorWidget::onDeviceStatusChanged);

    // 子组件 -> 业务层（经由 DeviceManager 单例）
    connect(m_deviceTree, &DeviceTreeWidget::deviceConnectRequested,
            &dm, &DeviceManager::connectDevice);
    connect(m_deviceTree, &DeviceTreeWidget::deviceDisconnectRequested,
            &dm, &DeviceManager::disconnectDevice);
    connect(m_deviceTree, &DeviceTreeWidget::deviceRemoveRequested,
            &dm, &DeviceManager::removeDevice);

    // ====================================================================
    // 设备配置持久化：DeviceManager 变化 → SQLite 同步写回
    // ====================================================================
    auto *dp = ServiceLocator::instance().dataPersistent();
    // 注意：不要按引用捕获局部变量 dm（它只是 connectSignals 栈上的引用别名）
    // lambda 内部统一通过 DeviceManager::instance() 访问单例，避免悬空引用崩溃
    auto persistDevice = [dp](const QString &deviceId, const QString &logTip) {
        if (!dp) return;
        const DeviceConfig cfg = DeviceManager::instance().getDeviceConfig(deviceId);
        if (cfg.id.isEmpty()) return;
        const QJsonDocument doc(QJsonObject::fromVariantMap(cfg.toMap()));
        const QString json = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
        const QString typeStr = QString::number(static_cast<int>(cfg.protocolType));
        if (dp->addDevice(cfg.id, cfg.name, typeStr, json)) {
            LogManager::instance().log(LogLevel::Info, LogCategory::Device,
                tr("设备配置已保存：%1（%2）").arg(cfg.id, logTip));
        } else {
            LogManager::instance().log(LogLevel::Warning, LogCategory::Device,
                tr("设备配置保存失败：%1").arg(cfg.id));
        }
    };

    connect(&dm, &DeviceManager::deviceAdded,
            this, [=](const QString &deviceId) { persistDevice(deviceId, tr("新增")); });
    connect(&dm, &DeviceManager::deviceConfigChanged,
            this, [=](const QString &deviceId) { persistDevice(deviceId, tr("修改")); });
    connect(&dm, &DeviceManager::deviceRemoved,
            this, [=](const QString &deviceId) {
                if (!dp) return;
                dp->removeDevice(deviceId);
                const DeviceConfig cfg = DeviceManager::instance().getDeviceConfig(deviceId);
                const QString dn = (!cfg.name.isEmpty() && cfg.name != cfg.id) ? cfg.name : deviceId;
                LogManager::instance().log(LogLevel::Info, LogCategory::Device,
                    tr("设备已从数据库删除：%1").arg(dn));
            });

    // 命令面板 -> 业务层（记录发送时间戳，便于计算响应耗时）
    connect(m_commandPanel, &CommandPanelWidget::sendBinaryFrameRequested,
            this, [this](const QByteArray &bytes) {
                if (!m_currentDeviceId.isEmpty()) {
                    ServiceLocator::instance().markSendTimestamp(
                        m_currentDeviceId, bytes, 3000);
                    DeviceManager::instance().sendCommand(m_currentDeviceId, bytes);
                    m_commandPanel->appendHistory(
                        QString::fromLatin1(bytes.toHex(' ').toUpper()),
                        QString(), 0);
                }
            });

    // 命令面板 Modbus → 业务层（构建 Modbus 帧后发送 + 测量响应耗时）
    connect(m_commandPanel, &CommandPanelWidget::sendModbusCommandRequested,
            this, [this](quint8 functionCode, quint16 address,
                         quint16 quantity, quint16 value, quint8 slaveAddress) {
                if (m_currentDeviceId.isEmpty()) return;

                // 获取设备配置，根据协议类型构建不同的帧
                DeviceConfig cfg = DeviceManager::instance().getDeviceConfig(m_currentDeviceId);
                QByteArray frame;

                if (cfg.protocolType == ProtocolType::ModbusTcp) {
                    // ---- Modbus TCP：构建 MBAP + PDU 帧 ----
                    ModbusTcpProtocol tcpProto(slaveAddress);

                    if (functionCode == 0x10) {
                        // 写多个寄存器：构建完整的多寄存器帧
                        // PDU: [0x10][地址高][地址低][数量高][数量低][字节数][值1高][值1低]...
                        QVector<quint16> values;
                        for (quint16 i = 0; i < quantity; ++i) {
                            values.append(value + i);  // 起始值递增
                        }
                        frame = tcpProto.buildWriteMultipleRegisters(
                            slaveAddress, address, values);
                    } else {
                        // 单寄存器读写：使用 buildRequest
                        // 注意：buildRequest 内部会添加功能码，所以 params 只包含地址和数量/值
                        quint16 qtyOrVal = (functionCode == 0x03 || functionCode == 0x04 ||
                                            functionCode == 0x01 || functionCode == 0x02)
                                            ? quantity : value;
                        QByteArray params;
                        params.append(static_cast<char>((address >> 8) & 0xFF));
                        params.append(static_cast<char>(address & 0xFF));
                        params.append(static_cast<char>((qtyOrVal >> 8) & 0xFF));
                        params.append(static_cast<char>(qtyOrVal & 0xFF));

                        frame = tcpProto.buildRequest(
                            static_cast<ModbusFunctionCode>(functionCode),
                            slaveAddress, params);
                    }
                } else {
                    // ---- Modbus RTU：构建 RTU 帧（含 CRC）----
                    if (functionCode == 0x10) {
                        // 写多个寄存器 RTU 格式
                        frame.append(static_cast<char>(slaveAddress));
                        frame.append(static_cast<char>(functionCode));
                        frame.append(static_cast<char>((address >> 8) & 0xFF));
                        frame.append(static_cast<char>(address & 0xFF));
                        frame.append(static_cast<char>((quantity >> 8) & 0xFF));
                        frame.append(static_cast<char>(quantity & 0xFF));
                        quint8 byteCount = quantity * 2;
                        frame.append(static_cast<char>(byteCount));
                        for (quint16 i = 0; i < quantity; ++i) {
                            quint16 v = value + i;
                            frame.append(static_cast<char>((v >> 8) & 0xFF));
                            frame.append(static_cast<char>(v & 0xFF));
                        }
                        // 附加 CRC16
                        quint16 crc = 0xFFFF;
                        for (int i = 0; i < frame.size(); ++i) {
                            crc ^= static_cast<quint8>(frame[i]);
                            for (int j = 0; j < 8; ++j) {
                                crc = (crc & 0x0001) ? (crc >> 1) ^ 0xA001 : (crc >> 1);
                            }
                        }
                        frame.append(static_cast<char>(crc & 0xFF));
                        frame.append(static_cast<char>((crc >> 8) & 0xFF));
                    } else {
                        quint16 qtyOrVal = (functionCode == 0x03 || functionCode == 0x04 ||
                                            functionCode == 0x01 || functionCode == 0x02)
                                            ? quantity : value;
                        frame.append(static_cast<char>(slaveAddress));
                        frame.append(static_cast<char>(functionCode));
                        frame.append(static_cast<char>((address >> 8) & 0xFF));
                        frame.append(static_cast<char>(address & 0xFF));
                        frame.append(static_cast<char>((qtyOrVal >> 8) & 0xFF));
                        frame.append(static_cast<char>(qtyOrVal & 0xFF));
                        // 附加 CRC16
                        quint16 crc = 0xFFFF;
                        for (int i = 0; i < frame.size(); ++i) {
                            crc ^= static_cast<quint8>(frame[i]);
                            for (int j = 0; j < 8; ++j) {
                                crc = (crc & 0x0001) ? (crc >> 1) ^ 0xA001 : (crc >> 1);
                            }
                        }
                        frame.append(static_cast<char>(crc & 0xFF));
                        frame.append(static_cast<char>((crc >> 8) & 0xFF));
                    }
                }

                ServiceLocator::instance().markSendTimestamp(
                    m_currentDeviceId, frame, 3000);
                // 发送命令，历史记录由 modbusResponseReceived / modbusRequestTimedOut 统一回显
                DeviceManager::instance().sendCommand(m_currentDeviceId, frame);
            });

    // 主窗口数据接收 -> 数据监控
    connect(this, &MainWindow::dataReceived,
            m_dataMonitor, &DataMonitorWidget::appendDataChunk);

    // 主窗口日志 -> 日志组件
    connect(this, &MainWindow::logMessage,
            m_logWidget, &LogWidget::appendLog);

    // ====================================================================
    // 初始化：刷新设备树 + 自动选中第一个设备（保证命令面板"目标设备"不为空）
    // ====================================================================
    m_deviceTree->refresh();
    refreshStatusBar();
    const auto ids = DeviceManager::instance().getAllDeviceIds();
    if (!ids.isEmpty()) {
        // 主动调 onDeviceSelected 兜底（避免用户没点设备时"目标设备：未选择"）
        onDeviceSelected(ids.first());
    }
}

void MainWindow::refreshStatusBar()
{
    const int total = m_deviceTree->deviceCount();
    const int online = m_deviceTree->onlineCount();
    m_deviceCountLabel->setText(
        tr("设备总数：%1  |  在线：%2").arg(total).arg(online));

    m_runtimeLabel->setText(tr("就绪"));
}

// ============================================================================
// 槽实现
// ============================================================================

void MainWindow::onAddDevice()
{
    AddDeviceDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        DeviceConfig cfg = dlg.getConfig();
        if (DeviceManager::instance().addDevice(cfg)) {
            m_deviceTree->refresh();
            LogManager::instance().log(LogLevel::Info, LogCategory::UI,
                tr("已添加设备：%1").arg(cfg.name));
        } else {
            QMessageBox::warning(this, tr("添加失败"),
                tr("设备添加失败（可能已存在或配置非法）。"));
        }
    }
}

void MainWindow::onOpenConfig()
{
    if (m_currentDeviceId.isEmpty()) {
        QMessageBox::information(this, tr("提示"), tr("请先在左侧选择一个设备。"));
        return;
    }
    DeviceConfig cfg = DeviceManager::instance().getDeviceConfig(m_currentDeviceId);
    ConfigDialog dlg(cfg, this);
    if (dlg.exec() == QDialog::Accepted) {
        DeviceManager::instance().updateDeviceConfig(dlg.getConfig());
        m_deviceTree->refresh();
    }
}

void MainWindow::onOpenHistoryQuery()
{
    auto &sl = ServiceLocator::instance();
    auto &dm = DeviceManager::instance();
    const QStringList allIds = dm.getAllDeviceIds();

    HistoryQueryDialog dlg(allIds, this);

    // 查询 → 调 DataPersistent::queryRegisterData（每个地址一次，地址区间遍历）
    connect(&dlg, &HistoryQueryDialog::queryRequested,
            &dlg, [&, this](const QDateTime &start, const QDateTime &end,
                             const QString &deviceId,
                             quint16 addrBegin, quint16 addrEnd) {
                QVector<HistoryRow> rows;
                if (!sl.dataPersistent()) return;

                // 若选中"全部设备"，需要查询的设备列表
                QStringList devices;
                if (deviceId.isEmpty()) {
                    devices = allIds;
                } else {
                    devices.append(deviceId);
                }

                const int stepCount = static_cast<int>(addrEnd) - static_cast<int>(addrBegin) + 1;
                const int kMaxQueries = 256;  // 防止用户选过大区间卡死
                const int effective = qMin(stepCount, kMaxQueries);

                for (const QString &dev : devices) {
                    for (int a = 0; a < effective; ++a) {
                        const quint16 addr = static_cast<quint16>(addrBegin + a);
                        const auto results = sl.dataPersistent()->queryRegisterData(dev, addr, start, end);
                        for (const QVariantMap &m : results) {
                            HistoryRow row;
                            row.timestamp       = m.value(QStringLiteral("timestamp")).toDateTime();
                            row.deviceId        = m.value(QStringLiteral("device_id")).toString();
                            row.registerAddress = static_cast<quint16>(
                                m.value(QStringLiteral("register_addr")).toUInt());
                            row.value           = m.value(QStringLiteral("value")).toDouble();
                            rows.append(row);
                        }
                    }
                }
                // 使用内部指针（因为 &dlg 是 lambda 捕获的 const）
                const_cast<HistoryQueryDialog*>(&dlg)->setResults(rows);
                LogManager::instance().log(LogLevel::Info, LogCategory::UI,
                    tr("历史查询完成：共 %1 条记录").arg(rows.size()));
            });

    dlg.exec();
}

void MainWindow::onOpenAlertConfig()
{
    if (m_currentDeviceId.isEmpty()) {
        QMessageBox::information(this, tr("提示"), tr("请先在左侧选择一个设备。"));
        return;
    }

    auto &sl = ServiceLocator::instance();
    if (!sl.alertManager()) return;

    // 把现有 AlertManager::AlertConfig 转为 AlertConfigDialog::AlertRule
    QList<AlertRule> initialRules;
    auto *am = sl.alertManager();
    if (am) {
        const auto map = am->allConfigsForDevice(m_currentDeviceId);
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            AlertRule rule;
            rule.registerAddress = it.key();
            rule.minValue        = it.value().minValue;
            rule.maxValue        = it.value().maxValue;
            rule.enabled         = it.value().enabled;
            initialRules.append(rule);
        }
    }

    AlertConfigDialog dlg(m_currentDeviceId, initialRules, this);
    if (dlg.exec() == QDialog::Accepted) {
        const QList<AlertRule> rules = dlg.rules();
        // 全量替换：先清空该设备原有配置，再重新 add
        if (am) am->clearDeviceConfigs(m_currentDeviceId);
        for (const AlertRule &r : rules) {
            AlertConfig cfg;
            cfg.registerAddr = r.registerAddress;
            cfg.minValue     = r.minValue;
            cfg.maxValue     = r.maxValue;
            cfg.enabled      = r.enabled;
            if (am) am->addAlertConfig(m_currentDeviceId, cfg);
        }
        LogManager::instance().log(LogLevel::Info, LogCategory::UI,
            tr("告警配置已更新：设备 %1，共 %2 条规则")
                .arg(m_currentDeviceId).arg(rules.size()));
    }
}

void MainWindow::onStartAll()
{
    DeviceManager::instance().connectAll();
}

void MainWindow::onStopAll()
{
    DeviceManager::instance().disconnectAll();
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, tr("关于"),
        tr("<h3>%1</h3>"
           "<p>版本：%2</p>"
           "<p>多协议嵌入式设备调试平台，支持 Serial / TCP / UDP 通信，"
           "支持 BinaryFrame 与 Modbus 协议。</p>")
            .arg(DEBUGPLATFORM_NAME, DEBUGPLATFORM_VERSION));
}

void MainWindow::onDeviceStatusChanged(const QString &id, DeviceStatus status)
{
    Q_UNUSED(id);
    Q_UNUSED(status);
    // 同步刷新设备树显示
    m_deviceTree->refresh();
    refreshStatusBar();
}

void MainWindow::onDataReceived(const QString &deviceId, const QByteArray &bytes)
{
    emit dataReceived(deviceId, bytes);
}

void MainWindow::onLogMessage(LogLevel level, const QString &category,
                              const QString &message, const QDateTime &timestamp)
{
    emit logMessage(level, category, message, timestamp);
}

void MainWindow::onDeviceSelected(const QString &deviceId)
{
    m_currentDeviceId = deviceId;
    m_commandPanel->setDeviceId(deviceId);
    m_actConfig->setEnabled(!deviceId.isEmpty());
    m_actAlertCfg->setEnabled(!deviceId.isEmpty());
}

void MainWindow::onToggleLogView()
{
    const bool visible = m_actToggleLog->isChecked();
    if (visible) {
        m_verticalSplitter->setSizes(QList<int>() << 600 << 200);
    } else {
        m_verticalSplitter->setSizes(QList<int>() << 800 << 0);
    }
    m_logWidget->setVisible(visible);
}

void MainWindow::onAlertOccurred(const QString &deviceId, quint16 registerAddr,
                                 double value, const QString &level,
                                 const QString &message, const QDateTime &timestamp)
{
    Q_UNUSED(deviceId);
    Q_UNUSED(registerAddr);
    Q_UNUSED(value);
    Q_UNUSED(level);
    Q_UNUSED(message);
    Q_UNUSED(timestamp);
    ++m_totalAlertCount;
    refreshAlertCountLabel();
    saveAlertCounterToDb();
}

void MainWindow::onModbusRequestTimedOut(const QString &deviceId, const QString &txHex)
{
    if (deviceId == m_currentDeviceId) {
        m_commandPanel->appendHistory(txHex, tr("(响应超时)"), -1);
    }
}

void MainWindow::refreshAlertCountLabel()
{
    if (!m_alertCountLabel) return;
    m_alertCountLabel->setText(tr("告警: %1").arg(m_totalAlertCount));
    if (m_totalAlertCount > 0) {
        m_alertCountLabel->setStyleSheet(
            QStringLiteral("QLabel { color: #D0021B; font-weight: 600; padding: 0 6px; }"));
    } else {
        m_alertCountLabel->setStyleSheet(
            QStringLiteral("QLabel { color: #333333; padding: 0 6px; }"));
    }
}

void MainWindow::loadAlertCounterFromDb()
{
    auto *dp = ServiceLocator::instance().dataPersistent();
    if (!dp || !dp->isReady()) return;
    const QString v = dp->getConfig(QStringLiteral("alert_counter_total"));
    bool ok = false;
    const int n = v.toInt(&ok);
    if (ok && n > 0) m_totalAlertCount = n;
    refreshAlertCountLabel();
}

void MainWindow::saveAlertCounterToDb() const
{
    auto *dp = ServiceLocator::instance().dataPersistent();
    if (!dp || !dp->isReady()) return;
    dp->setConfig(QStringLiteral("alert_counter_total"),
                  QString::number(m_totalAlertCount));
}

void MainWindow::onAlertCounterClicked()
{
    auto *dp = ServiceLocator::instance().dataPersistent();

    // -- 组装列表文本 --
    QStringList lines;
    lines << QStringLiteral("<h3>最近告警记录</h3>");
    if (!dp) {
        lines << QStringLiteral("<p style='color:#888'>数据持久化未就绪，无法读取记录。</p>");
    } else {
        const QVector<QVariantMap> alerts = dp->queryRecentAlerts(10);
        if (alerts.isEmpty()) {
            lines << QStringLiteral("<p style='color:#888'>暂无告警记录。</p>");
        } else {
            lines << QStringLiteral("<table cellpadding='4' cellspacing='0' "
                                    "style='border-collapse:collapse;'>");
            lines << QStringLiteral("<thead><tr bgcolor='#f0f0f0'>"
                "<th align='left'>时间</th>"
                "<th align='left'>设备</th>"
                "<th align='left'>寄存器</th>"
                "<th align='left'>当前值</th>"
                "<th align='left'>级别</th>"
                "<th align='left'>描述</th></tr></thead><tbody>");
            for (const QVariantMap &m : alerts) {
                const QString level = m.value(QStringLiteral("level")).toString();
                const QString color = (level.compare(QStringLiteral("ERROR"), Qt::CaseInsensitive) == 0)
                                      ? QStringLiteral("#D0021B")
                                      : (level.compare(QStringLiteral("WARNING"), Qt::CaseInsensitive) == 0
                                         ? QStringLiteral("#F5A623")
                                         : QStringLiteral("#333333"));
                const quint16 addr = static_cast<quint16>(
                    m.value(QStringLiteral("register_addr")).toUInt());
                lines << QStringLiteral(
                    "<tr style='border-bottom:1px solid #eee'>"
                    "<td>%1</td><td>%2</td>"
                    "<td>0x%3</td><td>%4</td>"
                    "<td style='color:%5;font-weight:600'>%6</td>"
                    "<td>%7</td></tr>")
                    .arg(m.value(QStringLiteral("timestamp")).toString(),
                         m.value(QStringLiteral("device_id")).toString())
                    .arg(addr, 4, 16, QChar('0'))
                    .arg(m.value(QStringLiteral("value")).toDouble(), 0, 'f', 3)
                    .arg(color, level,
                         m.value(QStringLiteral("message")).toString());
            }
            lines << QStringLiteral("</tbody></table>");
        }
    }

    // -- 构造弹窗（QDialog + QLabel + 清空计数按钮）--
    QDialog dlg(this);
    dlg.setWindowTitle(tr("最近告警"));
    dlg.resize(720, 460);
    auto *lay = new QVBoxLayout(&dlg);

    auto *lab = new QLabel(&dlg);
    lab->setWordWrap(true);
    lab->setTextFormat(Qt::RichText);
    lab->setTextInteractionFlags(Qt::TextBrowserInteraction);
    lab->setStyleSheet(QStringLiteral("background:white;padding:10px;"));
    lab->setText(lines.join(QString()));

    auto *scroll = new QScrollArea(&dlg);
    scroll->setWidgetResizable(true);
    scroll->setWidget(lab);

    auto *btns = new QDialogButtonBox(&dlg);
    auto *btnClear = new QPushButton(tr("清空计数"), &dlg);
    auto *btnClose = new QPushButton(tr("关闭"),   &dlg);
    btns->addButton(btnClear, QDialogButtonBox::ActionRole);
    btns->addButton(btnClose, QDialogButtonBox::RejectRole);
    connect(btnClear, &QPushButton::clicked, this, &MainWindow::onClearAlertCounter);
    connect(btnClear, &QPushButton::clicked, &dlg, &QDialog::accept);
    connect(btnClose, &QPushButton::clicked, &dlg, &QDialog::reject);

    lay->addWidget(scroll, 1);
    lay->addWidget(btns);

    dlg.exec();
}

void MainWindow::onClearAlertCounter()
{
    m_totalAlertCount = 0;
    refreshAlertCountLabel();
    saveAlertCounterToDb();
}
