#include <QtTest>
#include <QSignalSpy>
#include "mobile/mobileclient.h"
#include "mobile/alertnotificationservice.h"

class AlertNotificationServiceTest : public QObject {
    Q_OBJECT
private slots:
    // === Initial State ===
    void testInitialState();

    // === Add Alerts ===
    void testAddInfoAlert();
    void testAddWarningAlert();
    void testAddErrorAlert();
    void testAddCriticalAlert();
    void testAlertHasTimestamp();
    void testAlertIdsIncrement();
    void testAlertNotAcknowledgedByDefault();

    // === Query Alerts ===
    void testAllAlerts();
    void testUnacknowledgedAlerts();
    void testAlertsBySeverity();
    void testAlertById();
    void testAlertByIdNotFound();
    void testAlertCount();
    void testUnacknowledgedCount();

    // === Acknowledge ===
    void testAcknowledgeAlert();
    void testAcknowledgeAlreadyAcked();
    void testAcknowledgeNonExistent();
    void testAcknowledgeAll();
    void testAcknowledgeAllWhenNoneUnacked();

    // === Clear ===
    void testClearHistory();
    void testClearEmptyHistory();

    // === Max History ===
    void testSetMaxHistory();
    void testMaxHistoryTrimsOldest();
    void testReduceMaxHistoryTrims();

    // === Server Alert Processing ===
    void testProcessServerAlertInfo();
    void testProcessServerAlertWarning();
    void testProcessServerAlertError();
    void testProcessServerAlertCritical();
    void testProcessServerAlertUnknownSeverity();

    // === Signals ===
    void testAlertReceivedSignal();
    void testAlertAcknowledgedSignal();
    void testAllAlertsAcknowledgedSignal();
    void testAlertsClearedSignal();

    // === Client Integration ===
    void testAlertViaClientResponse();
    void testNonAlertResponseIgnored();

    // === Edge Cases ===
    void testManyAlerts();
    void testMixedSeverities();
    void testAcknowledgeSomeNotAll();
};

void AlertNotificationServiceTest::testInitialState() {
    MobileClient client;
    AlertNotificationService svc(&client);
    QCOMPARE(svc.alertCount(), 0);
    QCOMPARE(svc.unacknowledgedCount(), 0);
    QVERIFY(svc.allAlerts().isEmpty());
    QCOMPARE(svc.maxAlertHistory(), 100);
}

void AlertNotificationServiceTest::testAddInfoAlert() {
    MobileClient client;
    AlertNotificationService svc(&client);
    svc.addAlert(AlertNotificationService::Severity::Info, "System", "All good");
    QCOMPARE(svc.alertCount(), 1);
    QCOMPARE(svc.allAlerts().first().severity, AlertNotificationService::Severity::Info);
    QCOMPARE(svc.allAlerts().first().source, QString("System"));
    QCOMPARE(svc.allAlerts().first().message, QString("All good"));
}

void AlertNotificationServiceTest::testAddWarningAlert() {
    MobileClient client;
    AlertNotificationService svc(&client);
    svc.addAlert(AlertNotificationService::Severity::Warning, "Vision", "Low confidence");
    QCOMPARE(svc.allAlerts().first().severity, AlertNotificationService::Severity::Warning);
}

void AlertNotificationServiceTest::testAddErrorAlert() {
    MobileClient client;
    AlertNotificationService svc(&client);
    svc.addAlert(AlertNotificationService::Severity::Error, "Robot", "Joint limit exceeded");
    QCOMPARE(svc.allAlerts().first().severity, AlertNotificationService::Severity::Error);
}

void AlertNotificationServiceTest::testAddCriticalAlert() {
    MobileClient client;
    AlertNotificationService svc(&client);
    svc.addAlert(AlertNotificationService::Severity::Critical, "Safety", "E-Stop triggered");
    QCOMPARE(svc.allAlerts().first().severity, AlertNotificationService::Severity::Critical);
}

void AlertNotificationServiceTest::testAlertHasTimestamp() {
    MobileClient client;
    AlertNotificationService svc(&client);
    QDateTime before = QDateTime::currentDateTime();
    svc.addAlert(AlertNotificationService::Severity::Info, "Test", "msg");
    QDateTime after = QDateTime::currentDateTime();

    auto ts = svc.allAlerts().first().timestamp;
    QVERIFY(ts >= before);
    QVERIFY(ts <= after);
}

void AlertNotificationServiceTest::testAlertIdsIncrement() {
    MobileClient client;
    AlertNotificationService svc(&client);
    svc.addAlert(AlertNotificationService::Severity::Info, "A", "1");
    svc.addAlert(AlertNotificationService::Severity::Info, "B", "2");
    svc.addAlert(AlertNotificationService::Severity::Info, "C", "3");

    auto alerts = svc.allAlerts();
    QVERIFY(alerts[0].id < alerts[1].id);
    QVERIFY(alerts[1].id < alerts[2].id);
}

void AlertNotificationServiceTest::testAlertNotAcknowledgedByDefault() {
    MobileClient client;
    AlertNotificationService svc(&client);
    svc.addAlert(AlertNotificationService::Severity::Error, "R", "err");
    QVERIFY(!svc.allAlerts().first().acknowledged);
}

void AlertNotificationServiceTest::testAllAlerts() {
    MobileClient client;
    AlertNotificationService svc(&client);
    svc.addAlert(AlertNotificationService::Severity::Info, "A", "1");
    svc.addAlert(AlertNotificationService::Severity::Warning, "B", "2");
    QCOMPARE(svc.allAlerts().size(), 2);
}

void AlertNotificationServiceTest::testUnacknowledgedAlerts() {
    MobileClient client;
    AlertNotificationService svc(&client);
    svc.addAlert(AlertNotificationService::Severity::Info, "A", "1");
    svc.addAlert(AlertNotificationService::Severity::Warning, "B", "2");

    int id = svc.allAlerts().first().id;
    svc.acknowledgeAlert(id);

    QCOMPARE(svc.unacknowledgedAlerts().size(), 1);
    QCOMPARE(svc.unacknowledgedAlerts().first().message, QString("2"));
}

void AlertNotificationServiceTest::testAlertsBySeverity() {
    MobileClient client;
    AlertNotificationService svc(&client);
    svc.addAlert(AlertNotificationService::Severity::Info, "A", "1");
    svc.addAlert(AlertNotificationService::Severity::Error, "B", "2");
    svc.addAlert(AlertNotificationService::Severity::Info, "C", "3");
    svc.addAlert(AlertNotificationService::Severity::Error, "D", "4");

    QCOMPARE(svc.alertsBySeverity(AlertNotificationService::Severity::Info).size(), 2);
    QCOMPARE(svc.alertsBySeverity(AlertNotificationService::Severity::Error).size(), 2);
    QCOMPARE(svc.alertsBySeverity(AlertNotificationService::Severity::Warning).size(), 0);
}

void AlertNotificationServiceTest::testAlertById() {
    MobileClient client;
    AlertNotificationService svc(&client);
    svc.addAlert(AlertNotificationService::Severity::Info, "Src", "Found me");
    int id = svc.allAlerts().first().id;
    auto alert = svc.alertById(id);
    QCOMPARE(alert.id, id);
    QCOMPARE(alert.message, QString("Found me"));
}

void AlertNotificationServiceTest::testAlertByIdNotFound() {
    MobileClient client;
    AlertNotificationService svc(&client);
    auto alert = svc.alertById(999);
    QCOMPARE(alert.id, 0); // default
}

void AlertNotificationServiceTest::testAlertCount() {
    MobileClient client;
    AlertNotificationService svc(&client);
    QCOMPARE(svc.alertCount(), 0);
    svc.addAlert(AlertNotificationService::Severity::Info, "A", "1");
    QCOMPARE(svc.alertCount(), 1);
    svc.addAlert(AlertNotificationService::Severity::Info, "B", "2");
    QCOMPARE(svc.alertCount(), 2);
}

void AlertNotificationServiceTest::testUnacknowledgedCount() {
    MobileClient client;
    AlertNotificationService svc(&client);
    svc.addAlert(AlertNotificationService::Severity::Info, "A", "1");
    svc.addAlert(AlertNotificationService::Severity::Error, "B", "2");
    QCOMPARE(svc.unacknowledgedCount(), 2);

    svc.acknowledgeAlert(svc.allAlerts().first().id);
    QCOMPARE(svc.unacknowledgedCount(), 1);
}

void AlertNotificationServiceTest::testAcknowledgeAlert() {
    MobileClient client;
    AlertNotificationService svc(&client);
    svc.addAlert(AlertNotificationService::Severity::Error, "R", "err");
    int id = svc.allAlerts().first().id;
    svc.acknowledgeAlert(id);
    QVERIFY(svc.alertById(id).acknowledged);
}

void AlertNotificationServiceTest::testAcknowledgeAlreadyAcked() {
    MobileClient client;
    AlertNotificationService svc(&client);
    svc.addAlert(AlertNotificationService::Severity::Info, "A", "1");
    int id = svc.allAlerts().first().id;
    svc.acknowledgeAlert(id);

    QSignalSpy spy(&svc, &AlertNotificationService::alertAcknowledged);
    svc.acknowledgeAlert(id); // already acked
    QCOMPARE(spy.count(), 0); // no signal
}

void AlertNotificationServiceTest::testAcknowledgeNonExistent() {
    MobileClient client;
    AlertNotificationService svc(&client);
    QSignalSpy spy(&svc, &AlertNotificationService::alertAcknowledged);
    svc.acknowledgeAlert(999);
    QCOMPARE(spy.count(), 0);
}

void AlertNotificationServiceTest::testAcknowledgeAll() {
    MobileClient client;
    AlertNotificationService svc(&client);
    svc.addAlert(AlertNotificationService::Severity::Info, "A", "1");
    svc.addAlert(AlertNotificationService::Severity::Error, "B", "2");
    svc.addAlert(AlertNotificationService::Severity::Warning, "C", "3");

    svc.acknowledgeAll();
    QCOMPARE(svc.unacknowledgedCount(), 0);
    QCOMPARE(svc.alertCount(), 3); // still in history
}

void AlertNotificationServiceTest::testAcknowledgeAllWhenNoneUnacked() {
    MobileClient client;
    AlertNotificationService svc(&client);
    svc.addAlert(AlertNotificationService::Severity::Info, "A", "1");
    svc.acknowledgeAll();

    QSignalSpy spy(&svc, &AlertNotificationService::allAlertsAcknowledged);
    svc.acknowledgeAll(); // all already acked
    QCOMPARE(spy.count(), 0);
}

void AlertNotificationServiceTest::testClearHistory() {
    MobileClient client;
    AlertNotificationService svc(&client);
    svc.addAlert(AlertNotificationService::Severity::Info, "A", "1");
    svc.addAlert(AlertNotificationService::Severity::Info, "B", "2");
    svc.clearHistory();
    QCOMPARE(svc.alertCount(), 0);
}

void AlertNotificationServiceTest::testClearEmptyHistory() {
    MobileClient client;
    AlertNotificationService svc(&client);
    QSignalSpy spy(&svc, &AlertNotificationService::alertsCleared);
    svc.clearHistory();
    QCOMPARE(spy.count(), 1); // signal still emitted
}

void AlertNotificationServiceTest::testSetMaxHistory() {
    MobileClient client;
    AlertNotificationService svc(&client);
    svc.setMaxAlertHistory(50);
    QCOMPARE(svc.maxAlertHistory(), 50);
}

void AlertNotificationServiceTest::testMaxHistoryTrimsOldest() {
    MobileClient client;
    AlertNotificationService svc(&client);
    svc.setMaxAlertHistory(3);

    svc.addAlert(AlertNotificationService::Severity::Info, "A", "1");
    svc.addAlert(AlertNotificationService::Severity::Info, "B", "2");
    svc.addAlert(AlertNotificationService::Severity::Info, "C", "3");
    svc.addAlert(AlertNotificationService::Severity::Info, "D", "4");

    QCOMPARE(svc.alertCount(), 3);
    // Oldest (message "1") should be trimmed
    auto alerts = svc.allAlerts();
    QCOMPARE(alerts.first().message, QString("2"));
    QCOMPARE(alerts.last().message, QString("4"));
}

void AlertNotificationServiceTest::testReduceMaxHistoryTrims() {
    MobileClient client;
    AlertNotificationService svc(&client);

    for (int i = 0; i < 10; ++i)
        svc.addAlert(AlertNotificationService::Severity::Info, "S", QString::number(i));
    QCOMPARE(svc.alertCount(), 10);

    svc.setMaxAlertHistory(5);
    QCOMPARE(svc.alertCount(), 5);
}

void AlertNotificationServiceTest::testProcessServerAlertInfo() {
    MobileClient client;
    AlertNotificationService svc(&client);
    QVariantMap data;
    data["severity"] = "info";
    data["source"] = "Server";
    data["message"] = "Test info";
    svc.processServerAlert(data);

    QCOMPARE(svc.alertCount(), 1);
    QCOMPARE(svc.allAlerts().first().severity, AlertNotificationService::Severity::Info);
}

void AlertNotificationServiceTest::testProcessServerAlertWarning() {
    MobileClient client;
    AlertNotificationService svc(&client);
    QVariantMap data;
    data["severity"] = "warning";
    data["source"] = "Vision";
    data["message"] = "Low confidence";
    svc.processServerAlert(data);
    QCOMPARE(svc.allAlerts().first().severity, AlertNotificationService::Severity::Warning);
}

void AlertNotificationServiceTest::testProcessServerAlertError() {
    MobileClient client;
    AlertNotificationService svc(&client);
    QVariantMap data;
    data["severity"] = "error";
    data["source"] = "Robot";
    data["message"] = "Joint fault";
    svc.processServerAlert(data);
    QCOMPARE(svc.allAlerts().first().severity, AlertNotificationService::Severity::Error);
}

void AlertNotificationServiceTest::testProcessServerAlertCritical() {
    MobileClient client;
    AlertNotificationService svc(&client);
    QVariantMap data;
    data["severity"] = "critical";
    data["source"] = "Safety";
    data["message"] = "E-Stop";
    svc.processServerAlert(data);
    QCOMPARE(svc.allAlerts().first().severity, AlertNotificationService::Severity::Critical);
}

void AlertNotificationServiceTest::testProcessServerAlertUnknownSeverity() {
    MobileClient client;
    AlertNotificationService svc(&client);
    QVariantMap data;
    data["severity"] = "unknown_level";
    data["source"] = "X";
    data["message"] = "Y";
    svc.processServerAlert(data);
    QCOMPARE(svc.allAlerts().first().severity, AlertNotificationService::Severity::Info); // default
}

void AlertNotificationServiceTest::testAlertReceivedSignal() {
    MobileClient client;
    AlertNotificationService svc(&client);
    QSignalSpy spy(&svc, &AlertNotificationService::alertReceived);
    svc.addAlert(AlertNotificationService::Severity::Error, "Src", "Msg");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().at(2).toString(), QString("Msg"));
}

void AlertNotificationServiceTest::testAlertAcknowledgedSignal() {
    MobileClient client;
    AlertNotificationService svc(&client);
    svc.addAlert(AlertNotificationService::Severity::Info, "A", "1");
    int id = svc.allAlerts().first().id;

    QSignalSpy spy(&svc, &AlertNotificationService::alertAcknowledged);
    svc.acknowledgeAlert(id);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().first().toInt(), id);
}

void AlertNotificationServiceTest::testAllAlertsAcknowledgedSignal() {
    MobileClient client;
    AlertNotificationService svc(&client);
    svc.addAlert(AlertNotificationService::Severity::Info, "A", "1");
    svc.addAlert(AlertNotificationService::Severity::Info, "B", "2");

    QSignalSpy spy(&svc, &AlertNotificationService::allAlertsAcknowledged);
    svc.acknowledgeAll();
    QCOMPARE(spy.count(), 1);
}

void AlertNotificationServiceTest::testAlertsClearedSignal() {
    MobileClient client;
    AlertNotificationService svc(&client);
    svc.addAlert(AlertNotificationService::Severity::Info, "A", "1");

    QSignalSpy spy(&svc, &AlertNotificationService::alertsCleared);
    svc.clearHistory();
    QCOMPARE(spy.count(), 1);
}

void AlertNotificationServiceTest::testAlertViaClientResponse() {
    MobileClient client;
    client.setSimulationMode(true);
    client.connectToServer("host", 8080);
    AlertNotificationService svc(&client);

    QSignalSpy spy(&svc, &AlertNotificationService::alertReceived);

    QVariantMap resp;
    resp["severity"] = "error";
    resp["source"] = "Robot";
    resp["message"] = "Collision detected";
    client.injectSimResponse(1, resp);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(svc.alertCount(), 1);
    QCOMPARE(svc.allAlerts().first().message, QString("Collision detected"));
}

void AlertNotificationServiceTest::testNonAlertResponseIgnored() {
    MobileClient client;
    client.setSimulationMode(true);
    client.connectToServer("host", 8080);
    AlertNotificationService svc(&client);

    QVariantMap resp;
    resp["status"] = "ok";
    resp["data"] = 42;
    client.injectSimResponse(1, resp);

    QCOMPARE(svc.alertCount(), 0); // no alert created
}

void AlertNotificationServiceTest::testManyAlerts() {
    MobileClient client;
    AlertNotificationService svc(&client);
    svc.setMaxAlertHistory(50);

    for (int i = 0; i < 100; ++i) {
        svc.addAlert(AlertNotificationService::Severity::Info, "S", QString::number(i));
    }
    QCOMPARE(svc.alertCount(), 50); // trimmed to max
}

void AlertNotificationServiceTest::testMixedSeverities() {
    MobileClient client;
    AlertNotificationService svc(&client);

    svc.addAlert(AlertNotificationService::Severity::Info, "A", "info");
    svc.addAlert(AlertNotificationService::Severity::Warning, "B", "warn");
    svc.addAlert(AlertNotificationService::Severity::Error, "C", "err");
    svc.addAlert(AlertNotificationService::Severity::Critical, "D", "crit");
    svc.addAlert(AlertNotificationService::Severity::Info, "E", "info2");

    QCOMPARE(svc.alertsBySeverity(AlertNotificationService::Severity::Info).size(), 2);
    QCOMPARE(svc.alertsBySeverity(AlertNotificationService::Severity::Warning).size(), 1);
    QCOMPARE(svc.alertsBySeverity(AlertNotificationService::Severity::Error).size(), 1);
    QCOMPARE(svc.alertsBySeverity(AlertNotificationService::Severity::Critical).size(), 1);
}

void AlertNotificationServiceTest::testAcknowledgeSomeNotAll() {
    MobileClient client;
    AlertNotificationService svc(&client);

    svc.addAlert(AlertNotificationService::Severity::Info, "A", "1");
    svc.addAlert(AlertNotificationService::Severity::Error, "B", "2");
    svc.addAlert(AlertNotificationService::Severity::Warning, "C", "3");

    // Ack only the first
    svc.acknowledgeAlert(svc.allAlerts().at(0).id);
    QCOMPARE(svc.unacknowledgedCount(), 2);
    QCOMPARE(svc.alertCount(), 3);
}

QTEST_MAIN(AlertNotificationServiceTest)
#include "test_alertnotificationservice.moc"
