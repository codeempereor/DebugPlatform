# ==============================================================================
# DebugPlatform Benchmark Suite
# 3 benchmarks: RingBuffer throughput / Protocol parse rate / SQLite insert+query
# ==============================================================================

TEMPLATE = app
CONFIG += c++11 console
QT += core sql
CONFIG -= app_bundle

TARGET = DebugPlatformBench

# Project root
PROJ = $$PWD/../..

# Include paths
INCLUDEPATH += \
    $$PROJ/include \
    $$PROJ/include/core \
    $$PROJ/include/protocol \
    $$PROJ/include/data \
    $$PROJ/include/device

# Benchmark entry
SOURCES += $$PWD/main.cpp

# Protocol module (CRC16, BinaryFrame, Modbus RTU/TCP)
SOURCES += \
    $$PROJ/src/protocol/crc16.cpp \
    $$PROJ/src/protocol/binaryframeparser.cpp \
    $$PROJ/src/protocol/binaryframebuilder.cpp \
    $$PROJ/src/protocol/modbusrtuprotocol.cpp \
    $$PROJ/src/protocol/modbustcpprotocol.cpp

# Data module (DataPersistent for SQLite benchmark)
SOURCES += \
    $$PROJ/src/data/datapersistent.cpp

# Headers requiring MOC (QObject-derived classes used in benchmark)
HEADERS += \
    $$PROJ/include/protocol/binaryframeparser.h \
    $$PROJ/include/data/datapersistent.h

# QMake build type
CONFIG -= release
CONFIG += release