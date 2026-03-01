#include <QtTest/QtTest>

#include "orchestration/recoverymanager.h"
#include "workcellcontroller.h"

class RecoveryManagerTest : public QObject {
    Q_OBJECT

private slots:
    void testRegisterDefaults();
    void testRetryStrategy();
    void testEStopStrategy();
    void testUnknownErrorCode();
    void testRetryExhaustion();
};

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

void RecoveryManagerTest::testRetryStrategy() {
    RecoveryManager mgr;
    mgr.registerDefaults();

    WorkcellController controller;
    controller.startTask();

    QSignalSpy recoverySpy(&mgr, &RecoveryManager::recoveryAttempted);

    // First attempt at GraspFailed should succeed (retry available)
    bool recovered = mgr.handleError("GraspFailed", &controller);
    QVERIFY(recovered);
    QVERIFY(recoverySpy.count() > 0);

    // Controller should still be running
    QCOMPARE(controller.state(), WorkcellController::State::Running);
}

void RecoveryManagerTest::testEStopStrategy() {
    RecoveryManager mgr;

    RecoveryStrategy strategy;
    strategy.errorCode = "CriticalFault";
    strategy.actionSequence = {RecoveryStrategy::EStop};
    strategy.maxRetries = 1;
    mgr.registerStrategy(strategy);

    WorkcellController controller;
    controller.startTask();
    QCOMPARE(controller.state(), WorkcellController::State::Running);

    bool recovered = mgr.handleError("CriticalFault", &controller);
    QVERIFY(!recovered); // EStop means recovery failed
    QCOMPARE(controller.state(), WorkcellController::State::Fault);
}

void RecoveryManagerTest::testUnknownErrorCode() {
    RecoveryManager mgr;
    mgr.registerDefaults();

    WorkcellController controller;
    controller.startTask();

    QSignalSpy alertSpy(&mgr, &RecoveryManager::alertTriggered);

    bool recovered = mgr.handleError("SomeRandomError", &controller);
    QVERIFY(!recovered);
    QCOMPARE(alertSpy.count(), 1);
}

void RecoveryManagerTest::testRetryExhaustion() {
    RecoveryManager mgr;

    RecoveryStrategy strategy;
    strategy.errorCode = "TestError";
    strategy.actionSequence = {RecoveryStrategy::Retry, RecoveryStrategy::SkipObject};
    strategy.maxRetries = 2;
    mgr.registerStrategy(strategy);

    WorkcellController controller;
    controller.startTask();

    // First two calls: retry succeeds
    QVERIFY(mgr.handleError("TestError", &controller));
    QVERIFY(mgr.handleError("TestError", &controller));

    // Third call: retries exhausted, falls through to SkipObject
    QVERIFY(mgr.handleError("TestError", &controller));
}

QTEST_MAIN(RecoveryManagerTest)
#include "test_recoverymanager.moc"
