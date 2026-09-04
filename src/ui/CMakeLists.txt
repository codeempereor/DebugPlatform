# ==============================================================================
# UI 层模块 (User Interface Module)
# ==============================================================================

set(UI_SOURCES
    mainwindow.cpp
    devicetreewidget.cpp
    datamonitorwidget.cpp
    commandpanelwidget.cpp
    logwidget.cpp
    packetparserhighlighter.cpp
    adddevicedialog.cpp
    configdialog.cpp
    alertconfigdialog.cpp
    historyquerydialog.cpp
)

set(UI_HEADERS
    ${INC_DIR}/ui/mainwindow.h
    ${INC_DIR}/ui/devicetreewidget.h
    ${INC_DIR}/ui/datamonitorwidget.h
    ${INC_DIR}/ui/commandpanelwidget.h
    ${INC_DIR}/ui/logwidget.h
    ${INC_DIR}/ui/packetparserhighlighter.h
    ${INC_DIR}/ui/adddevicedialog.h
    ${INC_DIR}/ui/configdialog.h
    ${INC_DIR}/ui/alertconfigdialog.h
    ${INC_DIR}/ui/historyquerydialog.h
)

add_library(ui STATIC
    ${UI_SOURCES}
    ${UI_HEADERS}
)

target_link_libraries(ui PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    device
    data
)
