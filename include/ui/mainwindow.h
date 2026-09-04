#ifndef DEBUGPLATFORM_MAINWINDOW_H
#define DEBUGPLATFORM_MAINWINDOW_H

/**
 * @file mainwindow.h
 * @brief 应用程序主窗口
 *
 * 主窗口负责组合设备树、数据监控、命令面板和日志窗口等子组件，
 * 提供顶层菜单、工具栏和状态栏，并将业务层（DeviceManager/LogManager）
 * 的信号转发给各个 UI 子组件。本窗口不直接持有业务对象，仅通过
 * 信号槽机制进行通信，遵循 MVC 解耦原则。
 */

#include <QMainWindow>
#include <QSplitter>
#include <QStatusBar>
#include <QToolBar>
#include <QAction>
#include <QLabel>
#include <QString>
#include <QByteArray>
#include <QDateTime>
#include "core/config.h"

class DeviceTreeWidget;
class DataMonitorWidget;
class CommandPanelWidget;
class LogWidget;

/**
 * @class MainWindow
 * @brief 应用程序主窗口
 *
 * 采用中央 QSplitter 三栏布局：
 *   左栏  - 设备列表树 + 命令面板（垂直布局）
 *   中栏  - 数据监控表格
 *   底部  - 日志窗口（可隐藏）
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit MainWindow(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~MainWindow() override;

private:
    /**
     * @brief 初始化中心布局与子组件
     */
    void setupUi();

    /**
     * @brief 初始化工具栏动作
     */
    void setupToolbar();

    /**
     * @brief 初始化状态栏
     */
    void setupStatusBar();

    /**
     * @brief 连接业务层与 UI 组件的信号槽
     */
    void connectSignals();

    /**
     * @brief 根据当前状态刷新状态栏文字
     */
    void refreshStatusBar();

private slots:
    void onAddDevice();
    void onOpenConfig();
    void onOpenHistoryQuery();
    void onOpenAlertConfig();
    void onStartAll();
    void onStopAll();
    void onAbout();

    /**
     * @brief 设备状态变更槽
     */
    void onDeviceStatusChanged(const QString &id, DeviceStatus status);

    /**
     * @brief 数据接收槽
     */
    void onDataReceived(const QString &deviceId, const QByteArray &bytes);

    /**
     * @brief 日志输出槽
     */
    void onLogMessage(LogLevel level, const QString &category,
                      const QString &message, const QDateTime &timestamp);

    /**
     * @brief 设备选中时同步命令面板的上下文
     */
    void onDeviceSelected(const QString &deviceId);

    /**
     * @brief 切换日志窗口可见性
     */
    void onToggleLogView();

    /**
     * @brief 接收告警并更新状态栏计数器
     */
    void onAlertOccurred(const QString &deviceId, quint16 registerAddr, double value,
                         const QString &level, const QString &message,
                         const QDateTime &timestamp);

    /**
     * @brief 响应命令对端超时（写入历史面板）
     */
    void onModbusRequestTimedOut(const QString &deviceId, const QString &txHex);

    /**
     * @brief 点击状态栏告警标签 → 弹窗显示最近 10 条告警
     */
    void onAlertCounterClicked();

    /**
     * @brief 告警窗口内清空累计数
     */
    void onClearAlertCounter();

protected:
    /**
     * @brief 监听告警 label 的鼠标释放事件（等价点击）
     */
    bool eventFilter(QObject *obj, QEvent *evt) override;

private:
    /**
     * @brief 从 DataPersistent::configs 表恢复告警计数器
     */
    void loadAlertCounterFromDb();

    /**
     * @brief 将告警计数器写回 configs 表（key=alert_counter_total）
     */
    void saveAlertCounterToDb() const;

    /**
     * @brief 刷新告警计数 label 的颜色和文本
     */
    void refreshAlertCountLabel();

    // 子组件
    DeviceTreeWidget      *m_deviceTree   = nullptr;
    DataMonitorWidget     *m_dataMonitor  = nullptr;
    CommandPanelWidget    *m_commandPanel = nullptr;
    LogWidget             *m_logWidget    = nullptr;

    // 布局容器
    QSplitter             *m_mainSplitter  = nullptr;
    QSplitter             *m_leftSplitter  = nullptr;
    QSplitter             *m_verticalSplitter = nullptr;

    // 状态栏/工具栏
    QStatusBar            *m_statusBar     = nullptr;
    QToolBar              *m_toolBar      = nullptr;

    QLabel                *m_deviceCountLabel = nullptr;
    QLabel                *m_runtimeLabel     = nullptr;
    QLabel                *m_alertCountLabel  = nullptr;   ///< 告警计数（可点击）

    // 内部状态
    int                    m_totalAlertCount  = 0;        ///< 累计告警数

    // 工具栏动作
    QAction               *m_actAddDevice  = nullptr;
    QAction               *m_actConfig    = nullptr;
    QAction               *m_actHistory   = nullptr;
    QAction               *m_actAlertCfg  = nullptr;
    QAction               *m_actStartAll  = nullptr;
    QAction               *m_actStopAll   = nullptr;
    QAction               *m_actToggleLog = nullptr;
    QAction               *m_actAbout    = nullptr;

signals:
    /**
     * @brief 数据接收广播信号（转发业务层数据给子组件）
     */
    void dataReceived(const QString &deviceId, const QByteArray &data);

    /**
     * @brief 日志消息广播信号
     */
    void logMessage(LogLevel level, const QString &category,
                    const QString &message, const QDateTime &timestamp);

    /**
     * @brief 请求打开设备配置对话框
     */
    void openDeviceConfigRequested(const QString &deviceId);

private:
    // 当前选中设备 ID
    QString                m_currentDeviceId;
};

#endif // DEBUGPLATFORM_MAINWINDOW_H
