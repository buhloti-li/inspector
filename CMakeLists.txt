cmake_minimum_required(VERSION 3.21)
project(IndustrialRuntimeQt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Test)

enable_testing()

# --- Core runtime library ---
add_library(runtime
    # WorkcellController (original + extended)
    src/workcellcontroller.cpp
    src/workcellcontroller.h

    # Device layer
    src/device/idevicedriver.h
    src/device/icameradriver.h
    src/device/irobotdriver.h
    src/device/iplcdriver.h
    src/device/devicemanager.h
    src/device/devicemanager.cpp
    src/device/sim/simcameradriver.h
    src/device/sim/simcameradriver.cpp
    src/device/sim/simrobotdriver.h
    src/device/sim/simrobotdriver.cpp

    # Vision
    src/vision/visionpipeline.h
    src/vision/visionpipeline.cpp

    # Planning
    src/planning/graspplanner.h
    src/planning/graspplanner.cpp
    src/planning/trajectoryplanner.h
    src/planning/trajectoryplanner.cpp

    # Orchestration
    src/orchestration/workflowengine.h
    src/orchestration/workflowengine.cpp
    src/orchestration/recoverymanager.h
    src/orchestration/recoverymanager.cpp
)
target_link_libraries(runtime PUBLIC Qt6::Core)
target_include_directories(runtime PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# --- Test: WorkcellController (original) ---
add_executable(runtime_tests
    tests/test_workcellcontroller.cpp
)
target_link_libraries(runtime_tests PRIVATE runtime Qt6::Core Qt6::Test)
add_test(NAME runtime_tests COMMAND runtime_tests)

# --- Test: DeviceManager ---
add_executable(test_devicemanager
    tests/test_devicemanager.cpp
)
target_link_libraries(test_devicemanager PRIVATE runtime Qt6::Core Qt6::Test)
add_test(NAME test_devicemanager COMMAND test_devicemanager)

# --- Test: VisionPipeline ---
add_executable(test_visionpipeline
    tests/test_visionpipeline.cpp
)
target_link_libraries(test_visionpipeline PRIVATE runtime Qt6::Core Qt6::Test)
add_test(NAME test_visionpipeline COMMAND test_visionpipeline)

# --- Test: GraspPlanner ---
add_executable(test_graspplanner
    tests/test_graspplanner.cpp
)
target_link_libraries(test_graspplanner PRIVATE runtime Qt6::Core Qt6::Test)
add_test(NAME test_graspplanner COMMAND test_graspplanner)

# --- Test: WorkflowEngine ---
add_executable(test_workflowengine
    tests/test_workflowengine.cpp
)
target_link_libraries(test_workflowengine PRIVATE runtime Qt6::Core Qt6::Test)
add_test(NAME test_workflowengine COMMAND test_workflowengine)

# --- Test: RecoveryManager ---
add_executable(test_recoverymanager
    tests/test_recoverymanager.cpp
)
target_link_libraries(test_recoverymanager PRIVATE runtime Qt6::Core Qt6::Test)
add_test(NAME test_recoverymanager COMMAND test_recoverymanager)
