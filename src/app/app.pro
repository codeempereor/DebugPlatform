# ==============================================================================
# 主程序入口 (App Module)
# ==============================================================================

TEMPLATE = app
CONFIG += c++11
QT += core gui widgets sql network serialport

TARGET = DebugPlatform

# 依赖的模块库（从构建目录链接）
LIBS += \
    -L$$OUT_PWD/../core -lcore \
    -L$$OUT_PWD/../protocol -lprotocol \
    -L$$OUT_PWD/../data -ldata \
    -L$$OUT_PWD/../device -ldevice \
    -L$$OUT_PWD/../ui -lui

# 头文件
INCLUDEPATH += \
    $$PWD/../../include \
    $$PWD/../../include/app \
    $$PWD/../../include/core \
    $$PWD/../../include/protocol \
    $$PWD/../../include/data \
    $$PWD/../../include/device \
    $$PWD/../../include/ui \
    $$PWD/../ui \
    $$PWD/../device \
    $$PWD/../data \
    $$PWD/../core \
    $$PWD/../protocol

HEADERS += \
    $$PWD/../../include/app/servicelocator.h

# 源文件
SOURCES += \
    main.cpp \
    servicelocator.cpp

# 资源文件
RESOURCES += \
    $$PWD/../../resources/app.qrc

# Windows 下隐藏控制台窗口
win32 {
    CONFIG -= console
    CONFIG += windows
}
