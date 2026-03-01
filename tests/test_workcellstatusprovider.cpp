#include <QtTest>
#include <QSignalSpy>
#include "mobile/mobileclient.h"
#include "mobile/workcellstatusprovider.h"

class WorkcellStatusProviderTest : public QObject {
    Q_OBJECT
private slots:
    // === Initial State ===
    void testInitialState();

    // === State Updates ===
    void testStateUpdateIdle();
    void testStateUpdateRunning();
    void testStateUpdateStopped();
    void testStateUpdateFault();
    void testStateUpdateDegraded();
    void testStateUpdateUnknown();
    void testStateChangeEmitsSignal();
    void testSameStateNoSignal();

    // === Cycle Counts ===
    void testCycleCountUpdate();
    void testSuccessRate();
    void testSuccessRateZeroCycles();
    void testCycleCountChangeEmitsSignal();
    void testSameCycleCountNoSignal();

    // === Recipe ===
    void testRecipeUpdate();
    void testRecipeChangeEmitsSignal();
    void testSameRecipeNoSignal();

    // === Auto Mode ===
    void testAutoModeUpdate();
    void testAutoModeChangeEmitsSignal();
    void testSameAutoModeNoSignal();

    // === Polling ===
    void testPollingStartStop();
    void testPollInterval();
    void testPollingNotActiveByDefault();
    void testPollingRequestsUpdate();

    // === Manual Update ===
    void testManualRequestUpdate();
    void testUpdateFromStatus();

    // === Combined Status ===
    void testFullStatusUpdate();
    void testPartialStatusUpdate();
    void testMultipleSequentialUpdates();

    // === Edge Cases ===
    void testEmptyStatusMap();
    void testInvalidStateString();
    void testNegativeCycleCounts();
};

void WorkcellStatusProviderTest::testInitialState() {
    MobileClient client;
    WorkcellStatusProvider provider(&client);
    QCOMPARE(provider.workcellState(), WorkcellStatusProvider::WorkcellState::Unknown);
    QCOMPARE(provider.cycleCount(), 0);
    QCOMPARE(provider.successCount(), 0);
    QCOMPARE(provider.failCount(), 0);
    QVERIFY(provider.currentRecipe().isEmpty());
    QVERIFY(!provider.isAutoMode());
    QVERIFY(!provider.isPolling());
}

void WorkcellStatusProviderTest::testStateUpdateIdle() {
    MobileClient client;
    WorkcellStatusProvider provider(&client);
    QVariantMap status;
    status["state"] = "Idle";
    provider.updateFromStatus(status);
    QCOMPARE(provider.workcellState(), WorkcellStatusProvider::WorkcellState::Idle);
}

void WorkcellStatusProviderTest::testStateUpdateRunning() {
    MobileClient client;
    WorkcellStatusProvider provider(&client);
    QVariantMap status;
    status["state"] = "Running";
    provider.updateFromStatus(status);
    QCOMPARE(provider.workcellState(), WorkcellStatusProvider::WorkcellState::Running);
}

void WorkcellStatusProviderTest::testStateUpdateStopped() {
    MobileClient client;
    WorkcellStatusProvider provider(&client);
    QVariantMap status;
    status["state"] = "Stopped";
    provider.updateFromStatus(status);
    QCOMPARE(provider.workcellState(), WorkcellStatusProvider::WorkcellState::Stopped);
}

void WorkcellStatusProviderTest::testStateUpdateFault() {
    MobileClient client;
    WorkcellStatusProvider provider(&client);
    QVariantMap status;
    status["state"] = "Fault";
    provider.updateFromStatus(status);
    QCOMPARE(provider.workcellState(), WorkcellStatusProvider::WorkcellState::Fault);
}

void WorkcellStatusProviderTest::testStateUpdateDegraded() {
    MobileClient client;
    WorkcellStatusProvider provider(&client);
    QVariantMap status;
    status["state"] = "Degraded";
    provider.updateFromStatus(status);
    QCOMPARE(provider.workcellState(), WorkcellStatusProvider::WorkcellState::Degraded);
}

void WorkcellStatusProviderTest::testStateUpdateUnknown() {
    MobileClient client;
    WorkcellStatusProvider provider(&client);
    QVariantMap status;
    status["state"] = "SomethingInvalid";
    provider.updateFromStatus(status);
    QCOMPARE(provider.workcellState(), WorkcellStatusProvider::WorkcellState::Unknown);
}

void WorkcellStatusProviderTest::testStateChangeEmitsSignal() {
    MobileClient client;
    WorkcellStatusProvider provider(&client);
    QSignalSpy spy(&provider, &WorkcellStatusProvider::stateChanged);

    QVariantMap status;
    status["state"] = "Running";
    provider.updateFromStatus(status);
    QCOMPARE(spy.count(), 1);
}

void WorkcellStatusProviderTest::testSameStateNoSignal() {
    MobileClient client;
    WorkcellStatusProvider provider(&client);

    QVariantMap status;
    status["state"] = "Running";
    provider.updateFromStatus(status);

    QSignalSpy spy(&provider, &WorkcellStatusProvider::stateChanged);
    provider.updateFromStatus(status);
    QCOMPARE(spy.count(), 0); // same state, no signal
}

void WorkcellStatusProviderTest::testCycleCountUpdate() {
    MobileClient client;
    WorkcellStatusProvider provider(&client);
    QVariantMap status;
    status["state"] = "Running";
    status["cycleCount"] = 100;
    status["successCount"] = 95;
    status["failCount"] = 5;
    provider.updateFromStatus(status);

    QCOMPARE(provider.cycleCount(), 100);
    QCOMPARE(provider.successCount(), 95);
    QCOMPARE(provider.failCount(), 5);
}

void WorkcellStatusProviderTest::testSuccessRate() {
    MobileClient client;
    WorkcellStatusProvider provider(&client);
    QVariantMap status;
    status["state"] = "Running";
    status["cycleCount"] = 10;
    status["successCount"] = 7;
    status["failCount"] = 3;
    provider.updateFromStatus(status);

    QCOMPARE(provider.successRate(), 0.7);
}

void WorkcellStatusProviderTest::testSuccessRateZeroCycles() {
    MobileClient client;
    WorkcellStatusProvider provider(&client);
    QCOMPARE(provider.successRate(), 0.0);
}

void WorkcellStatusProviderTest::testCycleCountChangeEmitsSignal() {
    MobileClient client;
    WorkcellStatusProvider provider(&client);
    QSignalSpy spy(&provider, &WorkcellStatusProvider::cycleCountChanged);

    QVariantMap status;
    status["state"] = "Idle";
    status["cycleCount"] = 5;
    status["successCount"] = 4;
    status["failCount"] = 1;
    provider.updateFromStatus(status);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().at(0).toInt(), 5);
    QCOMPARE(spy.first().at(1).toInt(), 4);
    QCOMPARE(spy.first().at(2).toInt(), 1);
}

void WorkcellStatusProviderTest::testSameCycleCountNoSignal() {
    MobileClient client;
    WorkcellStatusProvider provider(&client);

    QVariantMap status;
    status["state"] = "Idle";
    status["cycleCount"] = 5;
    status["successCount"] = 4;
    status["failCount"] = 1;
    provider.updateFromStatus(status);

    QSignalSpy spy(&provider, &WorkcellStatusProvider::cycleCountChanged);
    provider.updateFromStatus(status); // same counts
    QCOMPARE(spy.count(), 0);
}

void WorkcellStatusProviderTest::testRecipeUpdate() {
    MobileClient client;
    WorkcellStatusProvider provider(&client);
    QVariantMap status;
    status["state"] = "Idle";
    status["recipe"] = "bolt_m8_pick";
    provider.updateFromStatus(status);

    QCOMPARE(provider.currentRecipe(), QString("bolt_m8_pick"));
}

void WorkcellStatusProviderTest::testRecipeChangeEmitsSignal() {
    MobileClient client;
    WorkcellStatusProvider provider(&client);
    QSignalSpy spy(&provider, &WorkcellStatusProvider::recipeChanged);

    QVariantMap status;
    status["state"] = "Idle";
    status["recipe"] = "part_a";
    provider.updateFromStatus(status);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().first().toString(), QString("part_a"));
}

void WorkcellStatusProviderTest::testSameRecipeNoSignal() {
    MobileClient client;
    WorkcellStatusProvider provider(&client);

    QVariantMap status;
    status["state"] = "Idle";
    status["recipe"] = "part_a";
    provider.updateFromStatus(status);

    QSignalSpy spy(&provider, &WorkcellStatusProvider::recipeChanged);
    provider.updateFromStatus(status);
    QCOMPARE(spy.count(), 0);
}

void WorkcellStatusProviderTest::testAutoModeUpdate() {
    MobileClient client;
    WorkcellStatusProvider provider(&client);
    QVariantMap status;
    status["state"] = "Running";
    status["autoMode"] = true;
    provider.updateFromStatus(status);

    QVERIFY(provider.isAutoMode());
}

void WorkcellStatusProviderTest::testAutoModeChangeEmitsSignal() {
    MobileClient client;
    WorkcellStatusProvider provider(&client);
    QSignalSpy spy(&provider, &WorkcellStatusProvider::autoModeChanged);

    QVariantMap status;
    status["state"] = "Running";
    status["autoMode"] = true;
    provider.updateFromStatus(status);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().first().toBool(), true);
}

void WorkcellStatusProviderTest::testSameAutoModeNoSignal() {
    MobileClient client;
    WorkcellStatusProvider provider(&client);

    QVariantMap status;
    status["state"] = "Running";
    status["autoMode"] = true;
    provider.updateFromStatus(status);

    QSignalSpy spy(&provider, &WorkcellStatusProvider::autoModeChanged);
    provider.updateFromStatus(status);
    QCOMPARE(spy.count(), 0);
}

void WorkcellStatusProviderTest::testPollingStartStop() {
    MobileClient client;
    client.setSimulationMode(true);
    client.connectToServer("host", 8080);
    WorkcellStatusProvider provider(&client);

    provider.startPolling(500);
    QVERIFY(provider.isPolling());
    QCOMPARE(provider.pollIntervalMs(), 500);

    provider.stopPolling();
    QVERIFY(!provider.isPolling());
}

void WorkcellStatusProviderTest::testPollInterval() {
    MobileClient client;
    WorkcellStatusProvider provider(&client);
    provider.startPolling(2000);
    QCOMPARE(provider.pollIntervalMs(), 2000);
    provider.stopPolling();
}

void WorkcellStatusProviderTest::testPollingNotActiveByDefault() {
    MobileClient client;
    WorkcellStatusProvider provider(&client);
    QVERIFY(!provider.isPolling());
}

void WorkcellStatusProviderTest::testPollingRequestsUpdate() {
    MobileClient client;
    client.setSimulationMode(true);
    client.connectToServer("host", 8080);
    WorkcellStatusProvider provider(&client);

    // Poll with very short interval
    provider.startPolling(20);
    QTest::qWait(80);
    provider.stopPolling();
    // Just verify no crash; in real scenario, requestStatus() would be called
}

void WorkcellStatusProviderTest::testManualRequestUpdate() {
    MobileClient client;
    client.setSimulationMode(true);
    client.connectToServer("host", 8080);
    WorkcellStatusProvider provider(&client);
    // Should not crash
    provider.requestUpdate();
}

void WorkcellStatusProviderTest::testUpdateFromStatus() {
    MobileClient client;
    WorkcellStatusProvider provider(&client);
    QSignalSpy updateSpy(&provider, &WorkcellStatusProvider::updateReceived);

    QVariantMap status;
    status["state"] = "Idle";
    provider.updateFromStatus(status);
    QCOMPARE(updateSpy.count(), 1);
}

void WorkcellStatusProviderTest::testFullStatusUpdate() {
    MobileClient client;
    WorkcellStatusProvider provider(&client);

    QVariantMap status;
    status["state"] = "Running";
    status["cycleCount"] = 42;
    status["successCount"] = 40;
    status["failCount"] = 2;
    status["recipe"] = "advanced_pick";
    status["autoMode"] = true;
    provider.updateFromStatus(status);

    QCOMPARE(provider.workcellState(), WorkcellStatusProvider::WorkcellState::Running);
    QCOMPARE(provider.cycleCount(), 42);
    QCOMPARE(provider.successCount(), 40);
    QCOMPARE(provider.failCount(), 2);
    QCOMPARE(provider.currentRecipe(), QString("advanced_pick"));
    QVERIFY(provider.isAutoMode());
}

void WorkcellStatusProviderTest::testPartialStatusUpdate() {
    MobileClient client;
    WorkcellStatusProvider provider(&client);

    // First set full state
    QVariantMap status1;
    status1["state"] = "Running";
    status1["cycleCount"] = 10;
    status1["successCount"] = 9;
    status1["failCount"] = 1;
    status1["recipe"] = "recipe_a";
    provider.updateFromStatus(status1);

    // Partial update - only state changes
    QVariantMap status2;
    status2["state"] = "Fault";
    provider.updateFromStatus(status2);

    QCOMPARE(provider.workcellState(), WorkcellStatusProvider::WorkcellState::Fault);
    // Other fields remain
    QCOMPARE(provider.cycleCount(), 10);
    QCOMPARE(provider.currentRecipe(), QString("recipe_a"));
}

void WorkcellStatusProviderTest::testMultipleSequentialUpdates() {
    MobileClient client;
    WorkcellStatusProvider provider(&client);
    QSignalSpy stateSpy(&provider, &WorkcellStatusProvider::stateChanged);

    QStringList states = {"Idle", "Running", "Fault", "Stopped", "Idle"};
    for (const auto& s : states) {
        QVariantMap status;
        status["state"] = s;
        provider.updateFromStatus(status);
    }
    QCOMPARE(stateSpy.count(), 5);
}

void WorkcellStatusProviderTest::testEmptyStatusMap() {
    MobileClient client;
    WorkcellStatusProvider provider(&client);
    QSignalSpy stateSpy(&provider, &WorkcellStatusProvider::stateChanged);
    provider.updateFromStatus({});
    // Empty map → state parsed as Unknown, which is initial → no change
    QCOMPARE(stateSpy.count(), 0);
}

void WorkcellStatusProviderTest::testInvalidStateString() {
    MobileClient client;
    WorkcellStatusProvider provider(&client);

    // Set to Running first
    QVariantMap status;
    status["state"] = "Running";
    provider.updateFromStatus(status);

    QSignalSpy spy(&provider, &WorkcellStatusProvider::stateChanged);
    QVariantMap invalid;
    invalid["state"] = "NonExistentState";
    provider.updateFromStatus(invalid);
    // Should revert to Unknown
    QCOMPARE(provider.workcellState(), WorkcellStatusProvider::WorkcellState::Unknown);
    QCOMPARE(spy.count(), 1);
}

void WorkcellStatusProviderTest::testNegativeCycleCounts() {
    MobileClient client;
    WorkcellStatusProvider provider(&client);
    QVariantMap status;
    status["state"] = "Idle";
    status["cycleCount"] = -1;
    status["successCount"] = -1;
    status["failCount"] = -1;
    provider.updateFromStatus(status);
    // Accepts whatever server sends
    QCOMPARE(provider.cycleCount(), -1);
}

QTEST_MAIN(WorkcellStatusProviderTest)
#include "test_workcellstatusprovider.moc"
