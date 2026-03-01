#include <QtTest/QtTest>

#include "orchestration/recoverymanager.h"
#include "workcellcontroller.h"

class RecoveryManagerTest : public QObject {
    Q_OBJECT

private slots:
    // === 注册与查询 ===
    void testRegisterDefaults();
    void testRegisterCustomStrategy();
    void testRemoveStrategy();
    void testOverwriteStrategy();

    // === 重试策略 ===
    void testRetryStrategy();
    void testRetryExhaustion();
    void testRetryCountResetAfterExhaustion();
    void testRetryMaxOne();
    void testRetryMaxFive();

    // === 紧急停止策略 ===
    void testEStopStrategy();
    void testEStopImmediatelyHalts();

    // === 降速策略 ===
    void testDegradeSpeedStrategy();

    // === 跳过策略 ===
    void testSkipObjectResetsRetryCount();

    // === 告警策略（不阻断） ===
    void testAlertContinuesSequence();
    void testAlertOnlyStrategy();

    // === 未知错误 ===
    void testUnknownErrorCode();
    void testEmptyErrorCode();

    // === 空指针/无效输入 ===
    void testNullController();
    void testHandleErrorWithNoStrategies();

    // === 复合策略序列 ===
    void testRetryThenSkip();
    void testAlertThenEStop();
    void testDegradeThenRetryThenEStop();

    // === 多种错误并行（状态隔离） ===
    void testMultipleErrorCodesIndependent();

    // === 信号验证 ===
    void testRecoveryAttemptedSignal();
    void testAlertTriggeredSignal();
    void testSignalCountsMatchActions();

    // === 默认策略详细验证 ===
    void testDefaultGraspFailedFullSequence();
    void testDefaultCollisionDetectedFullSequence();
    void testDefaultRobotCommLostFullSequence();
    void testDefaultVisionTimeoutFullSequence();
    void testDefaultNoObjectStrategy();
};

// === 注册与查询 ===

void RecoveryManagerTest::testRegisterDefaults() {
    RecoveryManager mgr;
    mgr.registerDefaults();
    QVERIFY(mgr.hasStrategy("GraspFailed"));
    QVERIFY(mgr.hasStrategy("CollisionDetected"));
    QVERIFY(mgr.hasStrategy("VisionTimeout"));
    QVERIFY(mgr.hasStrategy("NoObject"));
    QVERIFY(mgr.hasStrategy("RobotCommLost"));
    QVERIFY(!mgr.hasStrategy("UnknownError"));
}

void RecoveryManagerTest::testRegisterCustomStrategy() {
    RecoveryManager mgr;
    RecoveryStrategy s;
    s.errorCode = "CustomError";
    s.actionSequence = {RecoveryStrategy::Retry, RecoveryStrategy::Alert};
    s.maxRetries = 5;
    mgr.registerStrategy(s);
    QVERIFY(mgr.hasStrategy("CustomError"));
}

void RecoveryManagerTest::testRemoveStrategy() {
    RecoveryManager mgr;
    mgr.registerDefaults();
    QVERIFY(mgr.hasStrategy("GraspFailed"));
    mgr.removeStrategy("GraspFailed");
    QVERIFY(!mgr.hasStrategy("GraspFailed"));
}

void RecoveryManagerTest::testOverwriteStrategy() {
    RecoveryManager mgr;
    RecoveryStrategy s1; s1.errorCode = "Test"; s1.actionSequence = {RecoveryStrategy::Retry}; s1.maxRetries = 1;
    mgr.registerStrategy(s1);

    RecoveryStrategy s2; s2.errorCode = "Test"; s2.actionSequence = {RecoveryStrategy::EStop}; s2.maxRetries = 0;
    mgr.registerStrategy(s2);

    WorkcellController ctrl; ctrl.startTask();
    bool recovered = mgr.handleError("Test", &ctrl);
    QVERIFY(!recovered); // EStop
    QCOMPARE(ctrl.state(), WorkcellController::State::Fault);
}

// === 重试策略 ===

void RecoveryManagerTest::testRetryStrategy() {
    RecoveryManager mgr;
    mgr.registerDefaults();

    WorkcellController ctrl; ctrl.startTask();
    QSignalSpy spy(&mgr, &RecoveryManager::recoveryAttempted);

    QVERIFY(mgr.handleError("GraspFailed", &ctrl));
    QVERIFY(spy.count() > 0);
    QCOMPARE(ctrl.state(), WorkcellController::State::Running);
}

void RecoveryManagerTest::testRetryExhaustion() {
    RecoveryManager mgr;
    RecoveryStrategy s;
    s.errorCode = "TestError";
    s.actionSequence = {RecoveryStrategy::Retry, RecoveryStrategy::SkipObject};
    s.maxRetries = 2;
    mgr.registerStrategy(s);

    WorkcellController ctrl; ctrl.startTask();
    QVERIFY(mgr.handleError("TestError", &ctrl)); // retry 1
    QVERIFY(mgr.handleError("TestError", &ctrl)); // retry 2
    QVERIFY(mgr.handleError("TestError", &ctrl)); // retries exhausted -> SkipObject
}

void RecoveryManagerTest::testRetryCountResetAfterExhaustion() {
    RecoveryManager mgr;
    RecoveryStrategy s;
    s.errorCode = "Test";
    s.actionSequence = {RecoveryStrategy::Retry, RecoveryStrategy::SkipObject};
    s.maxRetries = 1;
    mgr.registerStrategy(s);

    WorkcellController ctrl; ctrl.startTask();
    QVERIFY(mgr.handleError("Test", &ctrl)); // retry
    QVERIFY(mgr.handleError("Test", &ctrl)); // exhausted -> skip (resets count)
    // Now retry count should be reset, so another call should retry again
    QVERIFY(mgr.handleError("Test", &ctrl)); // retry again
}

void RecoveryManagerTest::testRetryMaxOne() {
    RecoveryManager mgr;
    RecoveryStrategy s;
    s.errorCode = "Test";
    s.actionSequence = {RecoveryStrategy::Retry};
    s.maxRetries = 1;
    mgr.registerStrategy(s);

    WorkcellController ctrl; ctrl.startTask();
    QVERIFY(mgr.handleError("Test", &ctrl)); // first retry succeeds
    QVERIFY(!mgr.handleError("Test", &ctrl)); // no more retries, no fallback
}

void RecoveryManagerTest::testRetryMaxFive() {
    RecoveryManager mgr;
    RecoveryStrategy s;
    s.errorCode = "Test";
    s.actionSequence = {RecoveryStrategy::Retry};
    s.maxRetries = 5;
    mgr.registerStrategy(s);

    WorkcellController ctrl; ctrl.startTask();
    for (int i = 0; i < 5; ++i)
        QVERIFY(mgr.handleError("Test", &ctrl));
    QVERIFY(!mgr.handleError("Test", &ctrl)); // 6th call exhausted
}

// === 紧急停止 ===

void RecoveryManagerTest::testEStopStrategy() {
    RecoveryManager mgr;
    RecoveryStrategy s; s.errorCode = "Critical"; s.actionSequence = {RecoveryStrategy::EStop}; s.maxRetries = 1;
    mgr.registerStrategy(s);

    WorkcellController ctrl; ctrl.startTask();
    QVERIFY(!mgr.handleError("Critical", &ctrl));
    QCOMPARE(ctrl.state(), WorkcellController::State::Fault);
}

void RecoveryManagerTest::testEStopImmediatelyHalts() {
    RecoveryManager mgr;
    // Sequence: Retry, EStop - but if retry succeeds, should not reach EStop
    RecoveryStrategy s; s.errorCode = "Test"; s.actionSequence = {RecoveryStrategy::Retry, RecoveryStrategy::EStop}; s.maxRetries = 1;
    mgr.registerStrategy(s);

    WorkcellController ctrl; ctrl.startTask();
    QVERIFY(mgr.handleError("Test", &ctrl)); // retry
    QCOMPARE(ctrl.state(), WorkcellController::State::Running); // not estop

    // Second call: retry exhausted, falls to EStop
    QVERIFY(!mgr.handleError("Test", &ctrl));
    QCOMPARE(ctrl.state(), WorkcellController::State::Fault);
}

// === 降速 ===

void RecoveryManagerTest::testDegradeSpeedStrategy() {
    RecoveryManager mgr;
    RecoveryStrategy s; s.errorCode = "Collision"; s.actionSequence = {RecoveryStrategy::DegradeSpeed}; s.maxRetries = 1;
    mgr.registerStrategy(s);

    WorkcellController ctrl; ctrl.startTask();
    QVERIFY(mgr.handleError("Collision", &ctrl));
    QCOMPARE(ctrl.state(), WorkcellController::State::Running); // still running
}

// === 跳过 ===

void RecoveryManagerTest::testSkipObjectResetsRetryCount() {
    RecoveryManager mgr;
    RecoveryStrategy s; s.errorCode = "Test";
    s.actionSequence = {RecoveryStrategy::Retry, RecoveryStrategy::SkipObject};
    s.maxRetries = 2;
    mgr.registerStrategy(s);

    WorkcellController ctrl; ctrl.startTask();
    mgr.handleError("Test", &ctrl);
    mgr.handleError("Test", &ctrl);
    // Third call: retries exhausted -> skip (resets)
    QVERIFY(mgr.handleError("Test", &ctrl));
}

// === 告警 ===

void RecoveryManagerTest::testAlertContinuesSequence() {
    RecoveryManager mgr;
    RecoveryStrategy s; s.errorCode = "Test";
    s.actionSequence = {RecoveryStrategy::Alert, RecoveryStrategy::Retry};
    s.maxRetries = 1;
    mgr.registerStrategy(s);

    WorkcellController ctrl; ctrl.startTask();
    QSignalSpy alertSpy(&mgr, &RecoveryManager::alertTriggered);

    QVERIFY(mgr.handleError("Test", &ctrl));
    QCOMPARE(alertSpy.count(), 1); // Alert emitted before retry
}

void RecoveryManagerTest::testAlertOnlyStrategy() {
    RecoveryManager mgr;
    RecoveryStrategy s; s.errorCode = "Info";
    s.actionSequence = {RecoveryStrategy::Alert};
    s.maxRetries = 1;
    mgr.registerStrategy(s);

    WorkcellController ctrl; ctrl.startTask();
    QSignalSpy alertSpy(&mgr, &RecoveryManager::alertTriggered);

    // Alert alone doesn't return true (no resolution action)
    bool result = mgr.handleError("Info", &ctrl);
    QVERIFY(!result);
    QCOMPARE(alertSpy.count(), 1);
}

// === 未知/空错误 ===

void RecoveryManagerTest::testUnknownErrorCode() {
    RecoveryManager mgr;
    mgr.registerDefaults();

    WorkcellController ctrl; ctrl.startTask();
    QSignalSpy alertSpy(&mgr, &RecoveryManager::alertTriggered);

    QVERIFY(!mgr.handleError("SomeRandomError", &ctrl));
    QCOMPARE(alertSpy.count(), 1);
}

void RecoveryManagerTest::testEmptyErrorCode() {
    RecoveryManager mgr;
    WorkcellController ctrl; ctrl.startTask();
    QVERIFY(!mgr.handleError("", &ctrl));
}

// === 空指针 ===

void RecoveryManagerTest::testNullController() {
    RecoveryManager mgr;
    mgr.registerDefaults();
    QVERIFY(!mgr.handleError("GraspFailed", nullptr));
}

void RecoveryManagerTest::testHandleErrorWithNoStrategies() {
    RecoveryManager mgr; // no strategies registered
    WorkcellController ctrl; ctrl.startTask();
    QVERIFY(!mgr.handleError("AnyError", &ctrl));
}

// === 复合策略 ===

void RecoveryManagerTest::testRetryThenSkip() {
    RecoveryManager mgr;
    RecoveryStrategy s; s.errorCode = "E1";
    s.actionSequence = {RecoveryStrategy::Retry, RecoveryStrategy::SkipObject};
    s.maxRetries = 2;
    mgr.registerStrategy(s);

    WorkcellController ctrl; ctrl.startTask();
    QVERIFY(mgr.handleError("E1", &ctrl)); // retry 1
    QVERIFY(mgr.handleError("E1", &ctrl)); // retry 2
    QVERIFY(mgr.handleError("E1", &ctrl)); // skip
}

void RecoveryManagerTest::testAlertThenEStop() {
    RecoveryManager mgr;
    RecoveryStrategy s; s.errorCode = "E2";
    s.actionSequence = {RecoveryStrategy::Alert, RecoveryStrategy::EStop};
    s.maxRetries = 1;
    mgr.registerStrategy(s);

    WorkcellController ctrl; ctrl.startTask();
    QSignalSpy alertSpy(&mgr, &RecoveryManager::alertTriggered);

    QVERIFY(!mgr.handleError("E2", &ctrl)); // Alert -> EStop
    QCOMPARE(ctrl.state(), WorkcellController::State::Fault);
    QCOMPARE(alertSpy.count(), 1);
}

void RecoveryManagerTest::testDegradeThenRetryThenEStop() {
    RecoveryManager mgr;
    RecoveryStrategy s; s.errorCode = "E3";
    s.actionSequence = {RecoveryStrategy::DegradeSpeed, RecoveryStrategy::Retry, RecoveryStrategy::EStop};
    s.maxRetries = 1;
    mgr.registerStrategy(s);

    WorkcellController ctrl; ctrl.startTask();
    // First call: DegradeSpeed returns immediately
    QVERIFY(mgr.handleError("E3", &ctrl));
    QCOMPARE(ctrl.state(), WorkcellController::State::Running);
}

// === 多种错误独立 ===

void RecoveryManagerTest::testMultipleErrorCodesIndependent() {
    RecoveryManager mgr;
    RecoveryStrategy s1; s1.errorCode = "ErrorA"; s1.actionSequence = {RecoveryStrategy::Retry}; s1.maxRetries = 2;
    RecoveryStrategy s2; s2.errorCode = "ErrorB"; s2.actionSequence = {RecoveryStrategy::Retry}; s2.maxRetries = 2;
    mgr.registerStrategy(s1);
    mgr.registerStrategy(s2);

    WorkcellController ctrl; ctrl.startTask();

    // ErrorA: 2 retries
    QVERIFY(mgr.handleError("ErrorA", &ctrl));
    QVERIFY(mgr.handleError("ErrorA", &ctrl));
    QVERIFY(!mgr.handleError("ErrorA", &ctrl)); // exhausted

    // ErrorB should still have full retries
    QVERIFY(mgr.handleError("ErrorB", &ctrl));
    QVERIFY(mgr.handleError("ErrorB", &ctrl));
    QVERIFY(!mgr.handleError("ErrorB", &ctrl)); // now exhausted
}

// === 信号 ===

void RecoveryManagerTest::testRecoveryAttemptedSignal() {
    RecoveryManager mgr;
    RecoveryStrategy s; s.errorCode = "T"; s.actionSequence = {RecoveryStrategy::Retry}; s.maxRetries = 1;
    mgr.registerStrategy(s);

    WorkcellController ctrl; ctrl.startTask();
    QSignalSpy spy(&mgr, &RecoveryManager::recoveryAttempted);
    mgr.handleError("T", &ctrl);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().at(0).toString(), QString("T"));
    QCOMPARE(spy.first().at(1).toString(), QString("Retry"));
    QCOMPARE(spy.first().at(2).toBool(), true);
}

void RecoveryManagerTest::testAlertTriggeredSignal() {
    RecoveryManager mgr;
    WorkcellController ctrl; ctrl.startTask();
    QSignalSpy spy(&mgr, &RecoveryManager::alertTriggered);
    mgr.handleError("Unknown", &ctrl);
    QCOMPARE(spy.count(), 1);
    QVERIFY(spy.first().at(1).toString().contains("Unknown"));
}

void RecoveryManagerTest::testSignalCountsMatchActions() {
    RecoveryManager mgr;
    RecoveryStrategy s; s.errorCode = "Multi";
    s.actionSequence = {RecoveryStrategy::Alert, RecoveryStrategy::Retry};
    s.maxRetries = 1;
    mgr.registerStrategy(s);

    WorkcellController ctrl; ctrl.startTask();
    QSignalSpy recoverySpy(&mgr, &RecoveryManager::recoveryAttempted);
    QSignalSpy alertSpy(&mgr, &RecoveryManager::alertTriggered);

    mgr.handleError("Multi", &ctrl);
    QCOMPARE(alertSpy.count(), 1);
    QCOMPARE(recoverySpy.count(), 2); // Alert action + Retry action
}

// === 默认策略详细验证 ===

void RecoveryManagerTest::testDefaultGraspFailedFullSequence() {
    RecoveryManager mgr;
    mgr.registerDefaults();

    WorkcellController ctrl; ctrl.startTask();

    // 3 retries
    QVERIFY(mgr.handleError("GraspFailed", &ctrl));
    QVERIFY(mgr.handleError("GraspFailed", &ctrl));
    QVERIFY(mgr.handleError("GraspFailed", &ctrl));

    // 4th: retries exhausted -> SkipObject
    QVERIFY(mgr.handleError("GraspFailed", &ctrl));

    // Controller should still be running (no EStop)
    QCOMPARE(ctrl.state(), WorkcellController::State::Running);
}

void RecoveryManagerTest::testDefaultCollisionDetectedFullSequence() {
    RecoveryManager mgr;
    mgr.registerDefaults();

    WorkcellController ctrl; ctrl.startTask();

    // First: DegradeSpeed
    QVERIFY(mgr.handleError("CollisionDetected", &ctrl));
    QCOMPARE(ctrl.state(), WorkcellController::State::Running);
}

void RecoveryManagerTest::testDefaultRobotCommLostFullSequence() {
    RecoveryManager mgr;
    mgr.registerDefaults();

    WorkcellController ctrl; ctrl.startTask();

    // 5 retries
    for (int i = 0; i < 5; ++i)
        QVERIFY(mgr.handleError("RobotCommLost", &ctrl));

    // 6th: retries exhausted -> Alert (non-blocking) -> EStop
    QVERIFY(!mgr.handleError("RobotCommLost", &ctrl));
    QCOMPARE(ctrl.state(), WorkcellController::State::Fault);
}

void RecoveryManagerTest::testDefaultVisionTimeoutFullSequence() {
    RecoveryManager mgr;
    mgr.registerDefaults();

    WorkcellController ctrl; ctrl.startTask();

    // 2 retries
    QVERIFY(mgr.handleError("VisionTimeout", &ctrl));
    QVERIFY(mgr.handleError("VisionTimeout", &ctrl));

    // 3rd: retries exhausted -> Alert (non-blocking), then no more actions -> returns false
    QVERIFY(!mgr.handleError("VisionTimeout", &ctrl));
}

void RecoveryManagerTest::testDefaultNoObjectStrategy() {
    RecoveryManager mgr;
    mgr.registerDefaults();

    WorkcellController ctrl; ctrl.startTask();
    QSignalSpy alertSpy(&mgr, &RecoveryManager::alertTriggered);

    // NoObject only has Alert action
    QVERIFY(!mgr.handleError("NoObject", &ctrl));
    QCOMPARE(alertSpy.count(), 1);
    QCOMPARE(ctrl.state(), WorkcellController::State::Running);
}

QTEST_MAIN(RecoveryManagerTest)
#include "test_recoverymanager.moc"
