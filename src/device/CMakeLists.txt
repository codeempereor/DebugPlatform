# ==============================================================================
# 设备管理模块 (Device Module)
# ==============================================================================

# 多设备管理模块
set(DEVICE_SOURCES
    deviceconfig.cpp
    devicemanager.cpp
    reconnectcontroller.cpp
)

set(DEVICE_HEADERS
    ${INC_DIR}/device/deviceconfig.h
    ${INC_DIR}/device/devicemanager.h
    ${INC_DIR}/device/reconnectcontroller.h
)

add_library(device STATIC
    ${DEVICE_SOURCES}
    ${DEVICE_HEADERS}
)

target_link_libraries(device PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    core
    protocol
)
