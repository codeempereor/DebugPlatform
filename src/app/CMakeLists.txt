# ==============================================================================
# 主程序入口 (App Module)
# ==============================================================================

set(APP_SOURCES
    main.cpp
    servicelocator.cpp
)

set(APP_HEADERS
    ${INC_DIR}/app/servicelocator.h
)

# 可执行程序
add_executable(${PROJECT_NAME} WIN32
    ${APP_SOURCES}
    ${APP_HEADERS}
    ${RES_FILES}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    core
    protocol
    data
    device
    ui
)

# Windows 下隐藏控制台窗口
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()