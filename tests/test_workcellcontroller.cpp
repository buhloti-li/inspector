#include <QtTest/QtTest>

#include "workcellcontroller.h"

class WorkcellControllerTest : public QObject {
    Q_OBJECT

private slots:
    void testStartStopLifecycle();
    void testRetryToDegraded();
    void testStatisticsAndReset();
    void testFaultAndRecovery();
};

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

QTEST_MAIN(WorkcellControllerTest)
#include "test_workcellcontroller.moc"
