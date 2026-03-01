cmake_minimum_required(VERSION 3.21)
project(IndustrialDesktopApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

add_executable(IndustrialDesktop
    main.cpp
    mainwindow.h
    mainwindow.cpp
    dashboardwidget.h
    dashboardwidget.cpp
    devicetablewidget.h
    devicetablewidget.cpp
    controlpanelwidget.h
    controlpanelwidget.cpp
    alertwidget.h
    alertwidget.cpp
    connectiondialog.h
    connectiondialog.cpp
)

target_link_libraries(IndustrialDesktop PRIVATE
    runtime
    Qt6::Core
    Qt6::Widgets
)

# Windows-specific settings
if(WIN32)
    set_target_properties(IndustrialDesktop PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()
