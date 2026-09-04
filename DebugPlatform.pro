# ==============================================================================
# 多协议嵌入式设备调试平台 - 顶层工程文件
# 单项目模式：直接编译所有源文件，避免子项目链接问题
# ==============================================================================

TEMPLATE = app
CONFIG += c++11 console
QT += core gui widgets sql network serialport

TARGET = DebugPlatform

# 头文件包含路径
INCLUDEPATH += \
    $$PWD/include \
    $$PWD/include/app \
    $$PWD/include/core \
    $$PWD/include/protocol \
    $$PWD/include/data \
    $$PWD/include/device \
    $$PWD/include/ui \
    $$PWD/src/core \
    $$PWD/src/protocol \
    $$PWD/src/data \
    $$PWD/src/device \
    $$PWD/src/ui

# ============================================================================
# 源文件汇总（所有模块）
# ============================================================================

# App 模块
SOURCES += \
    $$PWD/src/app/main.cpp \
    $$PWD/src/app/servicelocator.cpp

# Core 模块
SOURCES += \
    $$PWD/src/core/icommunication.cpp \
    $$PWD/src/core/serialportcommunication.cpp \
    $$PWD/src/core/tcpcommunication.cpp \
    $$PWD/src/core/udpcommunication.cpp \
    $$PWD/src/core/mqttcommunication.cpp \
    $$PWD/src/core/communicationfactory.cpp

# Protocol 模块
SOURCES += \
    $$PWD/src/protocol/crc16.cpp \
    $$PWD/src/protocol/binaryframeparser.cpp \
    $$PWD/src/protocol/binaryframebuilder.cpp \
    $$PWD/src/protocol/modbusrtuprotocol.cpp \
    $$PWD/src/protocol/modbustcpprotocol.cpp \
    $$PWD/src/protocol/mqttprotocol.cpp

# Data 模块
SOURCES += \
    $$PWD/src/data/datacache.cpp \
    $$PWD/src/data/datapersistent.cpp \
    $$PWD/src/data/logmanager.cpp \
    $$PWD/src/data/alertmanager.cpp \
    $$PWD/src/data/playbackengine.cpp

# Device 模块
SOURCES += \
    $$PWD/src/device/deviceconfig.cpp \
    $$PWD/src/device/devicemanager.cpp \
    $$PWD/src/device/reconnectcontroller.cpp

# UI 模块
SOURCES += \
    $$PWD/src/ui/mainwindow.cpp \
    $$PWD/src/ui/devicetreewidget.cpp \
    $$PWD/src/ui/datamonitorwidget.cpp \
    $$PWD/src/ui/commandpanelwidget.cpp \
    $$PWD/src/ui/logwidget.cpp \
    $$PWD/src/ui/packetparserhighlighter.cpp \
    $$PWD/src/ui/adddevicedialog.cpp \
    $$PWD/src/ui/configdialog.cpp \
    $$PWD/src/ui/alertconfigdialog.cpp \
    $$PWD/src/ui/historyquerydialog.cpp

# ============================================================================
# 头文件汇总（用于 qmake 元对象编译和 IDE 显示）
# ============================================================================

HEADERS += \
    $$PWD/include/app/servicelocator.h \
    $$PWD/include/core/config.h \
    $$PWD/include/core/communication_config.h \
    $$PWD/include/core/icommunication.h \
    $$PWD/include/core/serialportcommunication.h \
    $$PWD/include/core/tcpcommunication.h \
    $$PWD/include/core/udpcommunication.h \
    $$PWD/include/core/mqttcommunication.h \
    $$PWD/include/core/communicationfactory.h \
    $$PWD/include/protocol/crc16.h \
    $$PWD/include/protocol/binaryframeparser.h \
    $$PWD/include/protocol/binaryframebuilder.h \
    $$PWD/include/protocol/modbusrtuprotocol.h \
    $$PWD/include/protocol/mqttprotocol.h \
    $$PWD/include/protocol/modbustcpprotocol.h \
    $$PWD/include/data/ringbuffer.h \
    $$PWD/include/data/datacache.h \
    $$PWD/include/data/datapersistent.h \
    $$PWD/include/data/logmanager.h \
    $$PWD/include/data/alertmanager.h \
    $$PWD/include/data/playbackengine.h \
    $$PWD/include/device/deviceconfig.h \
    $$PWD/include/device/devicemanager.h \
    $$PWD/include/device/reconnectcontroller.h \
    $$PWD/include/ui/mainwindow.h \
    $$PWD/include/ui/devicetreewidget.h \
    $$PWD/include/ui/datamonitorwidget.h \
    $$PWD/include/ui/commandpanelwidget.h \
    $$PWD/include/ui/logwidget.h \
    $$PWD/include/ui/packetparserhighlighter.h \
    $$PWD/include/ui/adddevicedialog.h \
    $$PWD/include/ui/configdialog.h \
    $$PWD/include/ui/alertconfigdialog.h \
    $$PWD/include/ui/historyquerydialog.h

# 资源文件
RESOURCES += \
    $$PWD/resources/app.qrc

# Windows 下隐藏控制台窗口
win32 {
    CONFIG -= console
    CONFIG += windows
}
