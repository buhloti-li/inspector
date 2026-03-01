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

    # Mobile client
    src/mobile/mobileclient.h
    src/mobile/mobileclient.cpp
    src/mobile/workcellstatusprovider.h
    src/mobile/workcellstatusprovider.cpp
    src/mobile/remotecommandservice.h
    src/mobile/remotecommandservice.cpp
    src/mobile/alertnotificationservice.h
    src/mobile/alertnotificationservice.cpp
    src/mobile/devicelistmodel.h
    src/mobile/devicelistmodel.cpp
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

# --- Test: SimDrivers ---
add_executable(test_simdrivers
    tests/test_simdrivers.cpp
)
target_link_libraries(test_simdrivers PRIVATE runtime Qt6::Core Qt6::Test)
add_test(NAME test_simdrivers COMMAND test_simdrivers)

# --- Test: TrajectoryPlanner ---
add_executable(test_trajectoryplanner
    tests/test_trajectoryplanner.cpp
)
target_link_libraries(test_trajectoryplanner PRIVATE runtime Qt6::Core Qt6::Test)
add_test(NAME test_trajectoryplanner COMMAND test_trajectoryplanner)

# --- Test: Integration Pick Cycle ---
add_executable(test_integration_pickcycle
    tests/test_integration_pickcycle.cpp
)
target_link_libraries(test_integration_pickcycle PRIVATE runtime Qt6::Core Qt6::Test)
add_test(NAME test_integration_pickcycle COMMAND test_integration_pickcycle)

# --- Test: MobileClient ---
add_executable(test_mobileclient
    tests/test_mobileclient.cpp
)
target_link_libraries(test_mobileclient PRIVATE runtime Qt6::Core Qt6::Test)
add_test(NAME test_mobileclient COMMAND test_mobileclient)

# --- Test: WorkcellStatusProvider ---
add_executable(test_workcellstatusprovider
    tests/test_workcellstatusprovider.cpp
)
target_link_libraries(test_workcellstatusprovider PRIVATE runtime Qt6::Core Qt6::Test)
add_test(NAME test_workcellstatusprovider COMMAND test_workcellstatusprovider)

# --- Test: RemoteCommandService ---
add_executable(test_remotecommandservice
    tests/test_remotecommandservice.cpp
)
target_link_libraries(test_remotecommandservice PRIVATE runtime Qt6::Core Qt6::Test)
add_test(NAME test_remotecommandservice COMMAND test_remotecommandservice)

# --- Test: AlertNotificationService ---
add_executable(test_alertnotificationservice
    tests/test_alertnotificationservice.cpp
)
target_link_libraries(test_alertnotificationservice PRIVATE runtime Qt6::Core Qt6::Test)
add_test(NAME test_alertnotificationservice COMMAND test_alertnotificationservice)

# --- Test: DeviceListModel ---
add_executable(test_devicelistmodel
    tests/test_devicelistmodel.cpp
)
target_link_libraries(test_devicelistmodel PRIVATE runtime Qt6::Core Qt6::Test)
add_test(NAME test_devicelistmodel COMMAND test_devicelistmodel)

# --- Mobile App (iOS / Desktop with Qt Quick) ---
option(BUILD_MOBILE_APP "Build the mobile monitoring app (requires Qt Quick)" OFF)
if(BUILD_MOBILE_APP)
    add_subdirectory(mobile)
endif()
