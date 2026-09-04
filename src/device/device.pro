# ==============================================================================
# 设备管理层 (Device Module)
# ==============================================================================

TEMPLATE = lib
CONFIG += staticlib c++11
QT += core network serialport

TARGET = device

# 依赖的模块库
LIBS += \
    $$PWD/../core/core.lib \
    $$PWD/../protocol/protocol.lib

# 头文件
INCLUDEPATH += \
    $$PWD/../../include \
    $$PWD/../../include/device \
    $$PWD/../../include/core \
    $$PWD/../../include/protocol \
    $$PWD/../../include/data \
    $$PWD/../core \
    $$PWD/../protocol

HEADERS += \
    $$PWD/../../include/device/deviceconfig.h \
    $$PWD/../../include/device/devicemanager.h \
    $$PWD/../../include/device/reconnectcontroller.h

# 源文件
SOURCES += \
    deviceconfig.cpp \
    devicemanager.cpp \
    reconnectcontroller.cpp
