#include <QtTest/QtTest>

#include "workcellcontroller.h"
#include "device/devicemanager.h"
#include "device/sim/simcameradriver.h"
#include "device/sim/simrobotdriver.h"

class WorkcellControllerTest : public QObject {
    Q_OBJECT

private slots:
    // === 生命周期：正常流程 ===
    void testStartStopLifecycle();
    void testStartFromIdle();
    void testStartFromStopped();
    void testStartFromDegraded();
    void testStopFromDegraded();

    // === 生命周期：异常流程 ===
    void testStartWhenAlreadyRunning();
    void testStartInFaultState();
    void testStopWhenIdle();
    void testStopWhenStopped();
    void testStopWhenFault();

    // === 重试与降级 ===
    void testRetryToDegraded();
    void testDegradedRecoveryOnSuccess();
    void testMaxRetryBoundaryExact();
    void testMaxRetryOne();
    void testMaxRetryLargeValue();
    void testInterleavedSuccessFailure();
    void testAllFailuresNeverSuccess();
    void testConsecutiveFailureResetOnSuccess();

    // === 紧急停止与故障恢复 ===
    void testFaultAndRecovery();
    void testEmergencyStopFromIdle();
    void testEmergencyStopFromDegraded();
    void testEmergencyStopFromStopped();
    void testDoubleEmergencyStop();
    void testRecoverFromNonFaultState();
    void testRecoverThenStartAgain();

    // === 统计与重置 ===
    void testStatisticsAndReset();
    void testSuccessRateZeroAttempts();
    void testSuccessRateAllSuccess();
    void testSuccessRateAllFailure();
    void testResetSessionFromIdle();
    void testResetSessionFromRunning();
    void testResetSessionFromDegraded();
    void testResetSessionFromFault();
    void testResetSessionClearsConsecutiveFailures();
    void testRecordResultIgnoredInIdle();
    void testRecordResultIgnoredInStopped();
    void testRecordResultIgnoredInFault();
    void testLargeNumberOfAttempts();

    // === 状态变化信号 ===
    void testStateChangedSignalOnStart();
    void testStateChangedSignalOnDegraded();
    void testNoSignalOnSameState();

    // === 设备绑定 ===
    void testDeviceBinding();
    void testEmptyDeviceIds();

    // === 工艺配方 ===
    void testRecipeLoadAndRetrieve();
    void testEmptyRecipe();

    // === 自动模式 ===
    void testAutoModeRequiresDeviceManager();
    void testAutoModeRequiresBoundDevices();
    void testAutoModeRequiresCameraAndRobot();
    void testAutoModeStartFromIdle();
    void testAutoModeStopViaStopTask();
    void testAutoModeStopViaEmergencyStop();
    void testAutoModeFlagAfterStop();
};

// === 生命周期：正常流程 ===

void WorkcellControllerTest::testStartStopLifecycle() {
    WorkcellController controller;
    QString error;

    QVERIFY(controller.startTask(&error));
    QCOMPARE(controller.state(), WorkcellController::State::Running);

    QVERIFY(!controller.startTask(&error));
    QCOMPARE(error, QString("AlreadyRunning"));

    QVERIFY(controller.stopTask(&error));
    QCOMPARE(controller.state(), WorkcellController::State::Stopped);
}

void WorkcellControllerTest::testStartFromIdle() {
    WorkcellController controller;
    QCOMPARE(controller.state(), WorkcellController::State::Idle);
    QVERIFY(controller.startTask());
    QCOMPARE(controller.state(), WorkcellController::State::Running);
}

void WorkcellControllerTest::testStartFromStopped() {
    WorkcellController controller;
    controller.startTask();
    controller.stopTask();
    QCOMPARE(controller.state(), WorkcellController::State::Stopped);

    // Start again from Stopped
    QVERIFY(controller.startTask());
    QCOMPARE(controller.state(), WorkcellController::State::Running);
}

void WorkcellControllerTest::testStartFromDegraded() {
    WorkcellController controller(1);
    controller.startTask();
    controller.recordPickResult(false); // triggers Degraded
    QCOMPARE(controller.state(), WorkcellController::State::Degraded);

    // Cannot start (already running/degraded) - startTask checks Running only
    // Degraded is treated as a running variant, so it should fail
    QString error;
    // stopTask should work from Degraded
    QVERIFY(controller.stopTask());
    // Then start again
    QVERIFY(controller.startTask(&error));
    QCOMPARE(controller.state(), WorkcellController::State::Running);
}

void WorkcellControllerTest::testStopFromDegraded() {
    WorkcellController controller(1);
    controller.startTask();
    controller.recordPickResult(false);
    QCOMPARE(controller.state(), WorkcellController::State::Degraded);

    QVERIFY(controller.stopTask());
    QCOMPARE(controller.state(), WorkcellController::State::Stopped);
}

// === 生命周期：异常流程 ===

void WorkcellControllerTest::testStartWhenAlreadyRunning() {
    WorkcellController controller;
    controller.startTask();
    QString error;
    QVERIFY(!controller.startTask(&error));
    QCOMPARE(error, QString("AlreadyRunning"));
}

void WorkcellControllerTest::testStartInFaultState() {
    WorkcellController controller;
    controller.emergencyStop(); // Idle -> Fault
    QString error;
    QVERIFY(!controller.startTask(&error));
    QCOMPARE(error, QString("CannotStartInFault"));
}

void WorkcellControllerTest::testStopWhenIdle() {
    WorkcellController controller;
    QString error;
    QVERIFY(!controller.stopTask(&error));
    QCOMPARE(error, QString("NotRunning"));
}

void WorkcellControllerTest::testStopWhenStopped() {
    WorkcellController controller;
    controller.startTask();
    controller.stopTask();
    QString error;
    QVERIFY(!controller.stopTask(&error));
    QCOMPARE(error, QString("NotRunning"));
}

void WorkcellControllerTest::testStopWhenFault() {
    WorkcellController controller;
    controller.emergencyStop();
    QString error;
    QVERIFY(!controller.stopTask(&error));
    QCOMPARE(error, QString("NotRunning"));
}

// === 重试与降级 ===

void WorkcellControllerTest::testRetryToDegraded() {
    WorkcellController controller(3);
    QVERIFY(controller.startTask());

    controller.recordPickResult(false);
    controller.recordPickResult(false);
    QCOMPARE(controller.state(), WorkcellController::State::Running);

    controller.recordPickResult(false);
    QCOMPARE(controller.state(), WorkcellController::State::Degraded);
    QCOMPARE(controller.consecutiveFailures(), 3);

    controller.recordPickResult(true);
    QCOMPARE(controller.state(), WorkcellController::State::Running);
    QCOMPARE(controller.consecutiveFailures(), 0);
}

void WorkcellControllerTest::testDegradedRecoveryOnSuccess() {
    WorkcellController controller(2);
    controller.startTask();
    controller.recordPickResult(false);
    controller.recordPickResult(false);
    QCOMPARE(controller.state(), WorkcellController::State::Degraded);

    // One success should recover back to Running
    controller.recordPickResult(true);
    QCOMPARE(controller.state(), WorkcellController::State::Running);
    QCOMPARE(controller.consecutiveFailures(), 0);
}

void WorkcellControllerTest::testMaxRetryBoundaryExact() {
    // maxRetry=3: at exactly 3 failures -> Degraded
    WorkcellController controller(3);
    controller.startTask();

    // 2 failures: still Running
    controller.recordPickResult(false);
    controller.recordPickResult(false);
    QCOMPARE(controller.state(), WorkcellController::State::Running);
    QCOMPARE(controller.consecutiveFailures(), 2);

    // 3rd failure: transition to Degraded
    controller.recordPickResult(false);
    QCOMPARE(controller.state(), WorkcellController::State::Degraded);
    QCOMPARE(controller.consecutiveFailures(), 3);
}

void WorkcellControllerTest::testMaxRetryOne() {
    // maxRetry=1: first failure immediately degrades
    WorkcellController controller(1);
    controller.startTask();
    controller.recordPickResult(false);
    QCOMPARE(controller.state(), WorkcellController::State::Degraded);
    QCOMPARE(controller.consecutiveFailures(), 1);
}

void WorkcellControllerTest::testMaxRetryLargeValue() {
    WorkcellController controller(100);
    controller.startTask();

    for (int i = 0; i < 99; ++i)
        controller.recordPickResult(false);
    QCOMPARE(controller.state(), WorkcellController::State::Running);
    QCOMPARE(controller.consecutiveFailures(), 99);

    controller.recordPickResult(false); // 100th failure
    QCOMPARE(controller.state(), WorkcellController::State::Degraded);
}

void WorkcellControllerTest::testInterleavedSuccessFailure() {
    WorkcellController controller(3);
    controller.startTask();

    // Fail 2, succeed, fail 2, succeed - should never degrade
    controller.recordPickResult(false);
    controller.recordPickResult(false);
    QCOMPARE(controller.consecutiveFailures(), 2);

    controller.recordPickResult(true); // resets consecutive
    QCOMPARE(controller.consecutiveFailures(), 0);

    controller.recordPickResult(false);
    controller.recordPickResult(false);
    QCOMPARE(controller.consecutiveFailures(), 2);
    QCOMPARE(controller.state(), WorkcellController::State::Running);

    controller.recordPickResult(true);
    QCOMPARE(controller.consecutiveFailures(), 0);
    QCOMPARE(controller.state(), WorkcellController::State::Running);
}

void WorkcellControllerTest::testAllFailuresNeverSuccess() {
    WorkcellController controller(3);
    controller.startTask();

    for (int i = 0; i < 10; ++i)
        controller.recordPickResult(false);

    QCOMPARE(controller.state(), WorkcellController::State::Degraded);
    QCOMPARE(controller.consecutiveFailures(), 10);
    QCOMPARE(controller.stats().failureCount, 10);
    QCOMPARE(controller.stats().successCount, 0);
}

void WorkcellControllerTest::testConsecutiveFailureResetOnSuccess() {
    WorkcellController controller(5);
    controller.startTask();

    controller.recordPickResult(false);
    controller.recordPickResult(false);
    controller.recordPickResult(false);
    QCOMPARE(controller.consecutiveFailures(), 3);

    controller.recordPickResult(true);
    QCOMPARE(controller.consecutiveFailures(), 0);

    // Previous failures don't carry over
    controller.recordPickResult(false);
    QCOMPARE(controller.consecutiveFailures(), 1);
}

// === 紧急停止与故障恢复 ===

void WorkcellControllerTest::testFaultAndRecovery() {
    WorkcellController controller;
    QVERIFY(controller.startTask());
    controller.emergencyStop();
    QCOMPARE(controller.state(), WorkcellController::State::Fault);

    QVERIFY(!controller.startTask());
    QVERIFY(controller.recoverFromFault());
    QCOMPARE(controller.state(), WorkcellController::State::Idle);
    QVERIFY(controller.startTask());
}

void WorkcellControllerTest::testEmergencyStopFromIdle() {
    WorkcellController controller;
    QCOMPARE(controller.state(), WorkcellController::State::Idle);
    controller.emergencyStop();
    QCOMPARE(controller.state(), WorkcellController::State::Fault);
}

void WorkcellControllerTest::testEmergencyStopFromDegraded() {
    WorkcellController controller(1);
    controller.startTask();
    controller.recordPickResult(false);
    QCOMPARE(controller.state(), WorkcellController::State::Degraded);

    controller.emergencyStop();
    QCOMPARE(controller.state(), WorkcellController::State::Fault);
}

void WorkcellControllerTest::testEmergencyStopFromStopped() {
    WorkcellController controller;
    controller.startTask();
    controller.stopTask();
    controller.emergencyStop();
    QCOMPARE(controller.state(), WorkcellController::State::Fault);
}

void WorkcellControllerTest::testDoubleEmergencyStop() {
    WorkcellController controller;
    controller.emergencyStop();
    QCOMPARE(controller.state(), WorkcellController::State::Fault);
    controller.emergencyStop(); // idempotent
    QCOMPARE(controller.state(), WorkcellController::State::Fault);
}

void WorkcellControllerTest::testRecoverFromNonFaultState() {
    WorkcellController controller;
    QVERIFY(!controller.recoverFromFault()); // Idle
    controller.startTask();
    QVERIFY(!controller.recoverFromFault()); // Running
    controller.stopTask();
    QVERIFY(!controller.recoverFromFault()); // Stopped
}

void WorkcellControllerTest::testRecoverThenStartAgain() {
    WorkcellController controller;
    controller.startTask();
    controller.emergencyStop();
    QVERIFY(controller.recoverFromFault());
    QCOMPARE(controller.consecutiveFailures(), 0); // reset after recover
    QVERIFY(controller.startTask());
    QCOMPARE(controller.state(), WorkcellController::State::Running);
}

// === 统计与重置 ===

void WorkcellControllerTest::testStatisticsAndReset() {
    WorkcellController controller;
    QVERIFY(controller.startTask());

    controller.recordPickResult(true);
    controller.recordPickResult(false);
    controller.recordPickResult(true);

    const auto stats = controller.stats();
    QCOMPARE(stats.totalAttempts, 3);
    QCOMPARE(stats.successCount, 2);
    QCOMPARE(stats.failureCount, 1);
    QCOMPARE(stats.successRate(), 2.0 / 3.0);

    controller.stopTask();
    controller.resetSession();

    const auto resetStats = controller.stats();
    QCOMPARE(resetStats.totalAttempts, 0);
    QCOMPARE(resetStats.successCount, 0);
    QCOMPARE(resetStats.failureCount, 0);
    QCOMPARE(controller.state(), WorkcellController::State::Idle);
}

void WorkcellControllerTest::testSuccessRateZeroAttempts() {
    WorkcellController controller;
    QCOMPARE(controller.stats().successRate(), 0.0);
}

void WorkcellControllerTest::testSuccessRateAllSuccess() {
    WorkcellController controller;
    controller.startTask();
    for (int i = 0; i < 100; ++i)
        controller.recordPickResult(true);
    QCOMPARE(controller.stats().successRate(), 1.0);
}

void WorkcellControllerTest::testSuccessRateAllFailure() {
    WorkcellController controller(1000);
    controller.startTask();
    for (int i = 0; i < 50; ++i)
        controller.recordPickResult(false);
    QCOMPARE(controller.stats().successRate(), 0.0);
}

void WorkcellControllerTest::testResetSessionFromIdle() {
    WorkcellController controller;
    // Reset from Idle should stay Idle (not transition to Idle from Stopped)
    controller.resetSession();
    QCOMPARE(controller.state(), WorkcellController::State::Idle);
}

void WorkcellControllerTest::testResetSessionFromRunning() {
    WorkcellController controller;
    controller.startTask();
    controller.recordPickResult(true);
    controller.resetSession();
    // State remains Running (only transitions Stopped -> Idle)
    QCOMPARE(controller.state(), WorkcellController::State::Running);
    QCOMPARE(controller.stats().totalAttempts, 0);
}

void WorkcellControllerTest::testResetSessionFromDegraded() {
    WorkcellController controller(1);
    controller.startTask();
    controller.recordPickResult(false);
    QCOMPARE(controller.state(), WorkcellController::State::Degraded);

    controller.resetSession();
    // Degraded stays Degraded (not Stopped)
    QCOMPARE(controller.state(), WorkcellController::State::Degraded);
    QCOMPARE(controller.consecutiveFailures(), 0);
}

void WorkcellControllerTest::testResetSessionFromFault() {
    WorkcellController controller;
    controller.emergencyStop();
    controller.resetSession();
    // Fault stays Fault (only transitions Stopped -> Idle)
    QCOMPARE(controller.state(), WorkcellController::State::Fault);
}

void WorkcellControllerTest::testResetSessionClearsConsecutiveFailures() {
    WorkcellController controller(5);
    controller.startTask();
    controller.recordPickResult(false);
    controller.recordPickResult(false);
    controller.recordPickResult(false);
    QCOMPARE(controller.consecutiveFailures(), 3);

    controller.stopTask();
    controller.resetSession();
    QCOMPARE(controller.consecutiveFailures(), 0);
}

void WorkcellControllerTest::testRecordResultIgnoredInIdle() {
    WorkcellController controller;
    controller.recordPickResult(true);
    controller.recordPickResult(false);
    QCOMPARE(controller.stats().totalAttempts, 0);
}

void WorkcellControllerTest::testRecordResultIgnoredInStopped() {
    WorkcellController controller;
    controller.startTask();
    controller.stopTask();
    controller.recordPickResult(true);
    QCOMPARE(controller.stats().totalAttempts, 0);
}

void WorkcellControllerTest::testRecordResultIgnoredInFault() {
    WorkcellController controller;
    controller.startTask();
    controller.emergencyStop();
    controller.recordPickResult(true);
    QCOMPARE(controller.stats().totalAttempts, 0);
}

void WorkcellControllerTest::testLargeNumberOfAttempts() {
    WorkcellController controller(100000);
    controller.startTask();
    for (int i = 0; i < 10000; ++i) {
        controller.recordPickResult(i % 3 != 0); // 2/3 success
    }
    auto stats = controller.stats();
    QCOMPARE(stats.totalAttempts, 10000);
    QCOMPARE(stats.successCount + stats.failureCount, 10000);
    QVERIFY(stats.successRate() > 0.6);
    QVERIFY(stats.successRate() < 0.7);
}

// === 状态变化信号 ===

void WorkcellControllerTest::testStateChangedSignalOnStart() {
    WorkcellController controller;
    QSignalSpy spy(&controller, &WorkcellController::stateChanged);

    controller.startTask();
    QCOMPARE(spy.count(), 1);

    auto args = spy.first();
    QCOMPARE(args.at(0).value<WorkcellController::State>(), WorkcellController::State::Idle);
    QCOMPARE(args.at(1).value<WorkcellController::State>(), WorkcellController::State::Running);
}

void WorkcellControllerTest::testStateChangedSignalOnDegraded() {
    WorkcellController controller(1);
    controller.startTask();

    QSignalSpy spy(&controller, &WorkcellController::stateChanged);
    controller.recordPickResult(false);

    QCOMPARE(spy.count(), 1);
    auto args = spy.first();
    QCOMPARE(args.at(0).value<WorkcellController::State>(), WorkcellController::State::Running);
    QCOMPARE(args.at(1).value<WorkcellController::State>(), WorkcellController::State::Degraded);
}

void WorkcellControllerTest::testNoSignalOnSameState() {
    WorkcellController controller;
    controller.emergencyStop();

    QSignalSpy spy(&controller, &WorkcellController::stateChanged);
    controller.emergencyStop(); // already Fault
    QCOMPARE(spy.count(), 0); // no signal for same-state transition
}

// === 设备绑定 ===

void WorkcellControllerTest::testDeviceBinding() {
    WorkcellController controller;
    DeviceManager mgr;
    controller.setDeviceManager(&mgr);
    controller.bindCamera("cam-01");
    controller.bindRobot("robot-01");
    QCOMPARE(controller.boundCameraId(), QString("cam-01"));
    QCOMPARE(controller.boundRobotId(), QString("robot-01"));
}

void WorkcellControllerTest::testEmptyDeviceIds() {
    WorkcellController controller;
    QVERIFY(controller.boundCameraId().isEmpty());
    QVERIFY(controller.boundRobotId().isEmpty());
}

// === 工艺配方 ===

void WorkcellControllerTest::testRecipeLoadAndRetrieve() {
    WorkcellController controller;
    QVariantMap recipe;
    recipe["minConfidence"] = 0.75;
    recipe["placeX"] = 300.0;
    recipe["placeY"] = -50.0;
    recipe["sku"] = "part_abc";
    controller.loadRecipe(recipe);

    auto loaded = controller.recipe();
    QCOMPARE(loaded["minConfidence"].toDouble(), 0.75);
    QCOMPARE(loaded["placeX"].toDouble(), 300.0);
    QCOMPARE(loaded["sku"].toString(), QString("part_abc"));
}

void WorkcellControllerTest::testEmptyRecipe() {
    WorkcellController controller;
    QVERIFY(controller.recipe().isEmpty());
}

// === 自动模式 ===

void WorkcellControllerTest::testAutoModeRequiresDeviceManager() {
    WorkcellController controller;
    QString error;
    QVERIFY(!controller.startAutoMode(&error));
    QCOMPARE(error, QString("NoDeviceManager"));
}

void WorkcellControllerTest::testAutoModeRequiresBoundDevices() {
    WorkcellController controller;
    DeviceManager mgr;
    controller.setDeviceManager(&mgr);
    QString error;
    QVERIFY(!controller.startAutoMode(&error));
    QCOMPARE(error, QString("DevicesNotBound"));
}

void WorkcellControllerTest::testAutoModeRequiresCameraAndRobot() {
    WorkcellController controller;
    DeviceManager mgr;
    controller.setDeviceManager(&mgr);
    controller.bindCamera("cam-01"); // only camera, no robot
    QString error;
    QVERIFY(!controller.startAutoMode(&error));
    QCOMPARE(error, QString("DevicesNotBound"));
}

void WorkcellControllerTest::testAutoModeStartFromIdle() {
    WorkcellController controller;
    DeviceManager mgr;
    controller.setDeviceManager(&mgr);
    controller.bindCamera("cam-01");
    controller.bindRobot("robot-01");

    // Should auto-start task then enable auto mode
    QVERIFY(controller.startAutoMode());
    QVERIFY(controller.isAutoMode());
    QCOMPARE(controller.state(), WorkcellController::State::Running);
}

void WorkcellControllerTest::testAutoModeStopViaStopTask() {
    WorkcellController controller;
    DeviceManager mgr;
    controller.setDeviceManager(&mgr);
    controller.bindCamera("cam-01");
    controller.bindRobot("robot-01");
    controller.startAutoMode();

    controller.stopTask();
    QVERIFY(!controller.isAutoMode());
    QCOMPARE(controller.state(), WorkcellController::State::Stopped);
}

void WorkcellControllerTest::testAutoModeStopViaEmergencyStop() {
    WorkcellController controller;
    DeviceManager mgr;
    controller.setDeviceManager(&mgr);
    controller.bindCamera("cam-01");
    controller.bindRobot("robot-01");
    controller.startAutoMode();

    controller.emergencyStop();
    QVERIFY(!controller.isAutoMode());
    QCOMPARE(controller.state(), WorkcellController::State::Fault);
}

void WorkcellControllerTest::testAutoModeFlagAfterStop() {
    WorkcellController controller;
    DeviceManager mgr;
    controller.setDeviceManager(&mgr);
    controller.bindCamera("cam-01");
    controller.bindRobot("robot-01");

    controller.startAutoMode();
    QVERIFY(controller.isAutoMode());

    controller.stopAutoMode();
    QVERIFY(!controller.isAutoMode());
    // Task should still be running even after auto mode stopped
    QCOMPARE(controller.state(), WorkcellController::State::Running);
}

QTEST_MAIN(WorkcellControllerTest)
#include "test_workcellcontroller.moc"
