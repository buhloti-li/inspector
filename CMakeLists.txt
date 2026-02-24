cmake_minimum_required(VERSION 3.21)
project(IndustrialRuntimeQt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Test)

enable_testing()

add_library(runtime
    src/workcellcontroller.cpp
    src/workcellcontroller.h
)
target_link_libraries(runtime PUBLIC Qt6::Core)
target_include_directories(runtime PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(runtime_tests
    tests/test_workcellcontroller.cpp
)
target_link_libraries(runtime_tests PRIVATE runtime Qt6::Core Qt6::Test)

add_test(NAME runtime_tests COMMAND runtime_tests)
