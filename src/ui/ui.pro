# ==============================================================================
# UI 层 (UI Module)
# ==============================================================================

TEMPLATE = lib
CONFIG += staticlib c++11
QT += core gui widgets network serialport

TARGET = ui

# 依赖的模块库
LIBS += \
    $$PWD/../device/device.lib \
    $$PWD/../data/data.lib \
    $$PWD/../core/core.lib \
    $$PWD/../protocol/protocol.lib

# 头文件
INCLUDEPATH += \
    $$PWD/../../include \
    $$PWD/../../include/ui \
    $$PWD/../../include/core \
    $$PWD/../../include/protocol \
    $$PWD/../../include/data \
    $$PWD/../../include/device \
    $$PWD/../device \
    $$PWD/../data

HEADERS += \
    $$PWD/../../include/ui/mainwindow.h \
    $$PWD/../../include/ui/devicetreewidget.h \
    $$PWD/../../include/ui/datamonitorwidget.h \
    $$PWD/../../include/ui/commandpanelwidget.h \
    $$PWD/../../include/ui/logwidget.h \
    $$PWD/../../include/ui/adddevicedialog.h \
    $$PWD/../../include/ui/configdialog.h \
    $$PWD/../../include/ui/alertconfigdialog.h \
    $$PWD/../../include/ui/historyquerydialog.h

# 源文件
SOURCES += \
    mainwindow.cpp \
    devicetreewidget.cpp \
    datamonitorwidget.cpp \
    commandpanelwidget.cpp \
    logwidget.cpp \
    adddevicedialog.cpp \
    configdialog.cpp \
    alertconfigdialog.cpp \
    historyquerydialog.cpp
