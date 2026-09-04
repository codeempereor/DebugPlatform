# ==============================================================================
# 数据管理层模块 (Data Module)
# ==============================================================================

set(DATA_SOURCES
    datacache.cpp
    datapersistent.cpp
    logmanager.cpp
    alertmanager.cpp
    playbackengine.cpp
)

set(DATA_HEADERS
    ${INC_DIR}/data/ringbuffer.h
    ${INC_DIR}/data/datacache.h
    ${INC_DIR}/data/datapersistent.h
    ${INC_DIR}/data/logmanager.h
    ${INC_DIR}/data/alertmanager.h
    ${INC_DIR}/data/playbackengine.h
)

add_library(data STATIC
    ${DATA_SOURCES}
    ${DATA_HEADERS}
)

target_link_libraries(data PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Sql
)
