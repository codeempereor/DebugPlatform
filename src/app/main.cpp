#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDateTime>
#include <QDir>
#include <QException>
#include <QMessageBox>
#include <QStandardPaths>
#include <QWidget>
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonValue>
#include <exception>

#ifdef Q_OS_WIN
#  include <windows.h>
#  include <stdlib.h>
#  include <time.h>
#  if defined(_MSC_VER)
#    include <dbghelp.h>
#    pragma comment(lib, "dbghelp.lib")
#    include <crtdbg.h>
#  endif
#endif

#include "app/servicelocator.h"
#include "data/logmanager.h"
#include "data/datapersistent.h"
#include "device/devicemanager.h"
#include "device/deviceconfig.h"
#include "ui/mainwindow.h"
#include "config.h"

// ============================================================================
// Win32 原生崩溃捕获（SEH：Access Violation / Stack Overflow 等）
//   - 原因：Qt 的 std::set_terminate 只能抓 C++ throw；访问违例(0xC0000005)
//     这种真正的 crash（例如野指针）会直接退出，应用程序输出只有
//     "crashed"，没有任何线索。挂接后会把"崩溃模块+地址+偏移"打印
//     到应用程序输出面板，贴出来就能定位代码行。
// ============================================================================
#ifdef Q_OS_WIN
static LONG WINAPI myUnhandledFilter(EXCEPTION_POINTERS *pEx)
{
    const DWORD exCode = pEx->ExceptionRecord->ExceptionCode;
    const void *exAddr = pEx->ExceptionRecord->ExceptionAddress;

    char moduleNameBuf[MAX_PATH] = {0};
    DWORD64 baseAddr = 0;
    // 查崩溃地址属于哪个模块（DLL / EXE）
    HMODULE hMod = nullptr;
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCSTR)exAddr, &hMod)) {
        GetModuleFileNameA(hMod, moduleNameBuf, _countof(moduleNameBuf));
        baseAddr = (DWORD64)hMod;
    }

    char buf[1024];
    _snprintf_s(buf, _countof(buf), _TRUNCATE,
      "[CRASH] ===== FATAL SEH EXCEPTION =====\n"
      "[CRASH] Code=0x%08X %s\n"
      "[CRASH] Addr=%p (Module=%s RVA=0x%llX)\n"
#ifdef _WIN64
      "[CRASH] Context: RIP=%p RSP=%p\n"
#else
      "[CRASH] Context: EIP=%p ESP=%p\n"
#endif
      "[CRASH] 请把以上 [CRASH] 行完整贴给开发者。",
      (unsigned)exCode,
      exCode == 0xC0000005 ? "ACCESS_VIOLATION(野指针/越界)" :
      exCode == 0xC00000FD ? "STACK_OVERFLOW" :
      exCode == 0xC0000409 ? "STACK_BUFFER_OVERRUN" :
      exCode == 0xE06D7363 ? "THROWN_CPP_EXCEPTION" :
      "UNKNOWN",
      exAddr,
      moduleNameBuf[0] ? moduleNameBuf : "?",
      (unsigned long long)((DWORD64)exAddr - baseAddr),
#ifdef _WIN64
      (void*)pEx->ContextRecord->Rip,
      (void*)pEx->ContextRecord->Rsp);
#else
      (void*)pEx->ContextRecord->Eip,
      (void*)pEx->ContextRecord->Esp);
#endif

    // 1. 先打到 stderr → Qt Creator 应用程序输出面板可直接看到
    fputs(buf, stderr);
    fflush(stderr);
    // 2. OutputDebugString → Visual Studio / DebugView 也能收到
    OutputDebugStringA(buf);
    // 3. 额外写一个 crash_log.txt 在 exe 旁，方便事后查
    char exePath[MAX_PATH] = {0};
    GetModuleFileNameA(nullptr, exePath, _countof(exePath));
    char *pSlash = strrchr(exePath, '\\');
    if (pSlash) { *pSlash = '\0'; }
    char logPath[MAX_PATH];
    _snprintf_s(logPath, _countof(logPath), _TRUNCATE,
                "%s\\crash_log.txt", exePath);
    FILE *fp = nullptr;
    if (fopen_s(&fp, logPath, "a") == 0 && fp) {
        const time_t now = time(nullptr);
        char timeBuf[32] = {0};
        strftime(timeBuf, sizeof(timeBuf),
                 "%Y-%m-%d %H:%M:%S", localtime(&now));
        fprintf(fp, "\n===== %s =====\n%s\n", timeBuf, buf);
        fclose(fp);
    }
    return EXCEPTION_EXECUTE_HANDLER;  // 终止进程
}
#endif  // Q_OS_WIN

/**
 * @brief 全局异常处理器
 *
 * 捕获未处理的 C++ 异常，记录错误日志并显示错误对话框。
 */
void globalExceptionHandler()
{
    std::exception_ptr p = std::current_exception();
    try {
        if (p) {
            std::rethrow_exception(p);
        }
    } catch (const QException &e) {
        qCritical() << "Unhandled Qt exception:" << e.what();
    } catch (const std::exception &e) {
        qCritical() << "Unhandled exception:" << e.what();
    } catch (...) {
        qCritical() << "Unhandled unknown exception";
    }

    // 尝试使用消息框显示错误
    QMessageBox::critical(nullptr,
                         QObject::tr("Critical Error"),
                         QObject::tr("An unhandled exception occurred.\n"
                                     "The application will now terminate."));
    std::abort();
}

/**
 * @brief 主函数
 * @param argc 参数数量
 * @param argv 参数列表
 * @return 退出码
 */
int main(int argc, char *argv[])
{
    // 设置全局异常处理器
    std::set_terminate(globalExceptionHandler);

    try {
        // ====================================================================
        // 1. 创建 QApplication
        // ====================================================================
        QApplication app(argc, argv);

        // ====================================================================
        // 1b. 挂接 Win32 SEH 崩溃捕获（P0：精确抓到访问违例 / 栈溢出位置）
        //     只在 Windows 生效，Linux/macOS 走原有 terminate handler。
        // ====================================================================
#ifdef Q_OS_WIN
        SetUnhandledExceptionFilter(myUnhandledFilter);
#  if defined(_MSC_VER)
        // MSVC 专用：让 Debug 版 CRT 也把 abort 送到上面的过滤器
        _set_abort_behavior(0, _WRITE_ABORT_MSG);
#  endif
#endif

        // ====================================================================
        // 2. 设置应用属性
        // ====================================================================
        QApplication::setApplicationName(DEBUGPLATFORM_NAME);
        QApplication::setApplicationVersion(DEBUGPLATFORM_VERSION);
        QApplication::setOrganizationName(DEBUGPLATFORM_ORGANIZATION);
        QApplication::setOrganizationDomain("debugplatform.local");

        // ====================================================================
        // 3. 命令行参数处理
        // ====================================================================
        QCommandLineParser parser;
        parser.setApplicationDescription(
            QObject::tr("Multi-Protocol Embedded Device Debug Platform\n"
                        "A Qt-based platform for debugging embedded devices\n"
                        "supporting multiple communication protocols."));
        parser.addHelpOption();
        parser.addVersionOption();

        // 自定义选项
        QCommandLineOption configOption(
            QStringList() << "c" << "config",
            QObject::tr("Set the configuration file path"),
            QObject::tr("path"));
        parser.addOption(configOption);

        QCommandLineOption logOption(
            QStringList() << "l" << "log",
            QObject::tr("Set the log file path"),
            QObject::tr("path"));
        parser.addOption(logOption);

        QCommandLineOption dbOption(
            QStringList() << "d" << "db",
            QObject::tr("Set the database file path"),
            QObject::tr("path"));
        parser.addOption(dbOption);

        QCommandLineOption styleOption(
            "style",
            QObject::tr("Set the Qt application style (e.g. Fusion, WindowsVista)"),
            QObject::tr("style"));
        parser.addOption(styleOption);

        parser.process(app);
        // ====================================================================
        // 3b. 设置应用样式（Windows 下默认用 windowsvista 使界面贴合系统）
        //     Qt 内置样式有: Windows / WindowsVista / Fusion / macOS / Plastique
        //     优先用命令行 --style 指定的；否则 Windows 默认用 windowsvista
        //     不直接在 QApplication 构造时设是因为参数解析必须在 app 创建后
        // ====================================================================
        if (parser.isSet(styleOption)) {
            app.setStyle(parser.value(styleOption));
        } else {
        #ifdef Q_OS_WIN
            app.setStyle("windowsvista");
        #endif
        }

        // ====================================================================
        // 4. 输出启动日志
        // ====================================================================
        qInfo() << "============================================";
        qInfo() << "  Starting" << DEBUGPLATFORM_NAME;
        qInfo() << "  Version:" << DEBUGPLATFORM_VERSION;
        qInfo() << "  Build:" << QDateTime::currentDateTime().toString(Qt::ISODate);
        qInfo() << "  Working Dir:" << QDir::currentPath();
        qInfo() << "  Arguments:" << app.arguments().join(" ");
        qInfo() << "============================================";

        // ====================================================================
        // 5. 初始化日志管理器
        //    - 先安装 qInstallMessageHandler：这样之后所有 qInfo/qWarning/qDebug
        //      都会被路由到 LogManager → 再经 signal 显示到 LogWidget 面板。
        //      否则只有直接调用 logMgr.log() 的消息能到 UI，MQTT/协议层的
        //      qInfo() 全部只进 stderr（应用程序输出面板），用户不可见。
        // ====================================================================
        qInstallMessageHandler(LogManager::qtMessageHandler);
        LogManager &logMgr = LogManager::instance();

        // 设置日志文件路径（如果指定了 --log 参数）
        QString logPath;
        if (parser.isSet(logOption)) {
            logPath = parser.value(logOption);
        } else {
            // 默认日志路径：应用程序目录下的 logs 文件夹
            QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
            if (logDir.isEmpty()) {
                logDir = QDir::currentPath() + "/logs";
            }
            logPath = logDir;
        }
        logMgr.setLogFile(logPath);

        logMgr.log(LogLevel::Info, LogCategory::App,
                   QString("%1 v%2 starting...")
                       .arg(DEBUGPLATFORM_NAME)
                       .arg(DEBUGPLATFORM_VERSION));

        logMgr.log(LogLevel::Debug, LogCategory::App,
                   QString("Working directory: %1").arg(QDir::currentPath()));

        // ====================================================================
        // 6. 初始化 ServiceLocator
        // ====================================================================
        logMgr.log(LogLevel::Info, LogCategory::App,
                   "Initializing ServiceLocator...");

        ServiceLocator &serviceLocator = ServiceLocator::instance();
        serviceLocator.initialize(&app);

        // 设置数据库路径
        QString dbPath;
        if (parser.isSet(dbOption)) {
            dbPath = parser.value(dbOption);
        } else {
            dbPath = Constants::DEFAULT_DB_PATH;
        }

        if (serviceLocator.dataPersistent()) {
            if (serviceLocator.dataPersistent()->initialize(dbPath)) {
                logMgr.log(LogLevel::Info, LogCategory::App,
                           QString("Database initialized: %1").arg(dbPath));

                // ----------------------------------------------------------------
                // 从数据库恢复上次保存的设备配置（重启设备不丢）
                // ----------------------------------------------------------------
                auto &dm = DeviceManager::instance();
                const auto rows = serviceLocator.dataPersistent()->loadAllDevices();
                int restored = 0;
                for (const QVariantMap &row : rows) {
                    const QString id     = row.value(QStringLiteral("id")).toString();
                    const QString name   = row.value(QStringLiteral("name")).toString();
                    const QString type   = row.value(QStringLiteral("type")).toString();
                    const QString config = row.value(QStringLiteral("config")).toString();
                    if (id.isEmpty() || config.isEmpty()) continue;

                    // 从 JSON 文本反序列化 DeviceConfig
                    const QJsonParseError ok = {}; // Qt5: 默认构造无 error
                    Q_UNUSED(ok);
                    const QJsonDocument doc = QJsonDocument::fromJson(config.toUtf8());
                    if (doc.isNull() || !doc.isObject()) continue;
                    DeviceConfig cfg;
                    cfg.fromMap(doc.object().toVariantMap());
                    // 保证 id/name 与 DB 主键一致（避免配置内容被改乱）
                    cfg.id   = id;
                    cfg.name = name.isEmpty() ? cfg.name : name;
                    cfg.commConfig.type = cfg.commType;
                    Q_UNUSED(type);
                    if (dm.addDevice(cfg)) {
                        ++restored;
                    }
                }
                if (restored > 0) {
                    logMgr.log(LogLevel::Info, LogCategory::App,
                               QString("Restored %1 devices from database").arg(restored));
                }
            } else {
                logMgr.log(LogLevel::Error, LogCategory::App,
                           QString("Failed to initialize database: %1").arg(dbPath));
            }
        }

        // ====================================================================
        // 7. 创建并显示主窗口
        // ====================================================================
        logMgr.log(LogLevel::Info, LogCategory::App,
                   "Creating main window...");

        MainWindow mainWindow;
        mainWindow.show();

        logMgr.log(LogLevel::Info, LogCategory::App,
                   "Main window displayed");

        // ====================================================================
        // 8. 进入事件循环
        // ====================================================================
        logMgr.log(LogLevel::Info, LogCategory::App,
                   "Entering event loop...");

        int result = app.exec();

        // ====================================================================
        // 9. 退出清理
        // ====================================================================
        logMgr.log(LogLevel::Info, LogCategory::App,
                   QString("Application exiting with code: %1").arg(result));

        serviceLocator.shutdown();

        logMgr.log(LogLevel::Info, LogCategory::App,
                   "Shutdown complete. Goodbye!");

        return result;

    } catch (const QException &e) {
        qCritical() << "Fatal QException:" << e.what();
        QMessageBox::critical(nullptr,
                             QObject::tr("Fatal Error"),
                             QObject::tr("A fatal error occurred:\n%1\n\n"
                                         "The application cannot continue.")
                                 .arg(e.what()));
        return -1;

    } catch (const std::exception &e) {
        qCritical() << "Fatal std::exception:" << e.what();
        QMessageBox::critical(nullptr,
                             QObject::tr("Fatal Error"),
                             QObject::tr("A fatal error occurred:\n%1\n\n"
                                         "The application cannot continue.")
                                 .arg(QString::fromUtf8(e.what())));
        return -1;

    } catch (...) {
        qCritical() << "Fatal unknown exception";
        QMessageBox::critical(nullptr,
                             QObject::tr("Fatal Error"),
                             QObject::tr("An unknown fatal error occurred.\n"
                                         "The application cannot continue."));
        return -1;
    }
}
