cmake_minimum_required(VERSION 3.21)
project(IndustrialMonitorApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Quick QuickControls2)

# QML resource file
qt_add_resources(QML_RESOURCES qml.qrc)

add_executable(IndustrialMonitor
    main.cpp
    ${QML_RESOURCES}
)

# Link the core runtime library from parent project
target_link_libraries(IndustrialMonitor PRIVATE
    runtime
    Qt6::Core
    Qt6::Quick
    Qt6::QuickControls2
)

# iOS-specific settings
if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set_target_properties(IndustrialMonitor PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/ios/Info.plist"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.industrialruntime.monitor"
        MACOSX_BUNDLE_BUNDLE_VERSION "1"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0.0"
        XCODE_ATTRIBUTE_DEVELOPMENT_TEAM ""
        XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
        QT_IOS_LAUNCH_SCREEN "${CMAKE_CURRENT_SOURCE_DIR}/ios/LaunchScreen.storyboard"
    )
endif()
