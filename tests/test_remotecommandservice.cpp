#include <QtTest>
#include <QSignalSpy>
#include "mobile/mobileclient.h"
#include "mobile/remotecommandservice.h"

class RemoteCommandServiceTest : public QObject {
    Q_OBJECT
private slots:
    // === Standard Commands ===
    void testStartCycle();
    void testStopCycle();
    void testEmergencyStop();
    void testLoadRecipe();
    void testSetAutoMode();
    void testSetSpeed();
    void testGenericCommand();

    // === Command When Disconnected ===
    void testCommandWhenDisconnected();
    void testStartCycleWhenDisconnected();
    void testEmergencyStopWhenDisconnected();

    // === Command Tracking ===
    void testCommandRecordExists();
    void testCommandRecordPending();
    void testPendingCommandCount();
    void testPendingCommandIds();
    void testNonExistentRecord();

    // === Response Processing ===
    void testAcknowledgeResponse();
    void testRejectedResponse();
    void testTimeoutResponse();
    void testUnknownStatusResponse();
    void testResponseForUnknownId();

    // === Signals ===
    void testCommandSentSignal();
    void testCommandAcknowledgedSignal();
    void testCommandRejectedSignal();
    void testCommandTimeoutSignal();
    void testCommandErrorSignal();

    // === Timeout Config ===
    void testDefaultTimeout();
    void testSetTimeout();

    // === Multiple Commands ===
    void testMultipleCommandsTracked();
    void testAckOneOfMultiple();
    void testMixedResponses();

    // === Integration with MobileClient ===
    void testResponseViaClientSignal();
    void testLoadRecipeParams();
    void testSetSpeedParams();
};

void RemoteCommandServiceTest::testStartCycle() {
    MobileClient client;
    client.setSimulationMode(true);
    client.connectToServer("host", 8080);
    RemoteCommandService svc(&client);

    int id = svc.startCycle();
    QVERIFY(id > 0);
    auto rec = svc.commandRecord(id);
    QCOMPARE(rec.command, QString("startCycle"));
}

void RemoteCommandServiceTest::testStopCycle() {
    MobileClient client;
    client.setSimulationMode(true);
    client.connectToServer("host", 8080);
    RemoteCommandService svc(&client);

    int id = svc.stopCycle();
    QVERIFY(id > 0);
    QCOMPARE(svc.commandRecord(id).command, QString("stopCycle"));
}

void RemoteCommandServiceTest::testEmergencyStop() {
    MobileClient client;
    client.setSimulationMode(true);
    client.connectToServer("host", 8080);
    RemoteCommandService svc(&client);

    int id = svc.emergencyStop();
    QVERIFY(id > 0);
    QCOMPARE(svc.commandRecord(id).command, QString("emergencyStop"));
}

void RemoteCommandServiceTest::testLoadRecipe() {
    MobileClient client;
    client.setSimulationMode(true);
    client.connectToServer("host", 8080);
    RemoteCommandService svc(&client);

    int id = svc.loadRecipe("bolt_m6");
    QVERIFY(id > 0);
    QCOMPARE(svc.commandRecord(id).command, QString("loadRecipe"));
}

void RemoteCommandServiceTest::testSetAutoMode() {
    MobileClient client;
    client.setSimulationMode(true);
    client.connectToServer("host", 8080);
    RemoteCommandService svc(&client);

    int id = svc.setAutoMode(true);
    QVERIFY(id > 0);
    QCOMPARE(svc.commandRecord(id).command, QString("setAutoMode"));
}

void RemoteCommandServiceTest::testSetSpeed() {
    MobileClient client;
    client.setSimulationMode(true);
    client.connectToServer("host", 8080);
    RemoteCommandService svc(&client);

    int id = svc.setSpeed(75.0);
    QVERIFY(id > 0);
    QCOMPARE(svc.commandRecord(id).command, QString("setSpeed"));
}

void RemoteCommandServiceTest::testGenericCommand() {
    MobileClient client;
    client.setSimulationMode(true);
    client.connectToServer("host", 8080);
    RemoteCommandService svc(&client);

    QVariantMap params;
    params["key"] = "value";
    int id = svc.sendCommand("customCmd", params);
    QVERIFY(id > 0);
    QCOMPARE(svc.commandRecord(id).command, QString("customCmd"));
}

void RemoteCommandServiceTest::testCommandWhenDisconnected() {
    MobileClient client;
    RemoteCommandService svc(&client);

    int id = svc.sendCommand("test");
    QCOMPARE(id, -1);
}

void RemoteCommandServiceTest::testStartCycleWhenDisconnected() {
    MobileClient client;
    RemoteCommandService svc(&client);

    int id = svc.startCycle();
    QCOMPARE(id, -1);
}

void RemoteCommandServiceTest::testEmergencyStopWhenDisconnected() {
    MobileClient client;
    RemoteCommandService svc(&client);

    int id = svc.emergencyStop();
    QCOMPARE(id, -1);
}

void RemoteCommandServiceTest::testCommandRecordExists() {
    MobileClient client;
    client.setSimulationMode(true);
    client.connectToServer("host", 8080);
    RemoteCommandService svc(&client);

    int id = svc.startCycle();
    auto rec = svc.commandRecord(id);
    QCOMPARE(rec.requestId, id);
    QCOMPARE(rec.status, RemoteCommandService::CommandStatus::Pending);
}

void RemoteCommandServiceTest::testCommandRecordPending() {
    MobileClient client;
    client.setSimulationMode(true);
    client.connectToServer("host", 8080);
    RemoteCommandService svc(&client);

    int id = svc.startCycle();
    QCOMPARE(svc.commandRecord(id).status, RemoteCommandService::CommandStatus::Pending);
}

void RemoteCommandServiceTest::testPendingCommandCount() {
    MobileClient client;
    client.setSimulationMode(true);
    client.connectToServer("host", 8080);
    RemoteCommandService svc(&client);

    svc.startCycle();
    svc.stopCycle();
    svc.emergencyStop();
    QCOMPARE(svc.pendingCommandCount(), 3);
}

void RemoteCommandServiceTest::testPendingCommandIds() {
    MobileClient client;
    client.setSimulationMode(true);
    client.connectToServer("host", 8080);
    RemoteCommandService svc(&client);

    int id1 = svc.startCycle();
    int id2 = svc.stopCycle();
    auto ids = svc.pendingCommandIds();
    QVERIFY(ids.contains(id1));
    QVERIFY(ids.contains(id2));
}

void RemoteCommandServiceTest::testNonExistentRecord() {
    MobileClient client;
    RemoteCommandService svc(&client);
    auto rec = svc.commandRecord(9999);
    QCOMPARE(rec.requestId, -1); // default
}

void RemoteCommandServiceTest::testAcknowledgeResponse() {
    MobileClient client;
    client.setSimulationMode(true);
    client.connectToServer("host", 8080);
    RemoteCommandService svc(&client);

    int id = svc.startCycle();
    QVariantMap resp;
    resp["status"] = "ok";
    resp["message"] = "Cycle started";
    svc.processResponse(id, resp);

    QCOMPARE(svc.commandRecord(id).status, RemoteCommandService::CommandStatus::Acknowledged);
    QCOMPARE(svc.pendingCommandCount(), 0);
}

void RemoteCommandServiceTest::testRejectedResponse() {
    MobileClient client;
    client.setSimulationMode(true);
    client.connectToServer("host", 8080);
    RemoteCommandService svc(&client);

    int id = svc.startCycle();
    QVariantMap resp;
    resp["status"] = "rejected";
    resp["reason"] = "Not in Idle state";
    svc.processResponse(id, resp);

    QCOMPARE(svc.commandRecord(id).status, RemoteCommandService::CommandStatus::Rejected);
    QCOMPARE(svc.commandRecord(id).message, QString("Not in Idle state"));
}

void RemoteCommandServiceTest::testTimeoutResponse() {
    MobileClient client;
    client.setSimulationMode(true);
    client.connectToServer("host", 8080);
    RemoteCommandService svc(&client);

    int id = svc.startCycle();
    QVariantMap resp;
    resp["status"] = "timeout";
    svc.processResponse(id, resp);

    QCOMPARE(svc.commandRecord(id).status, RemoteCommandService::CommandStatus::Timeout);
}

void RemoteCommandServiceTest::testUnknownStatusResponse() {
    MobileClient client;
    client.setSimulationMode(true);
    client.connectToServer("host", 8080);
    RemoteCommandService svc(&client);

    int id = svc.startCycle();
    QVariantMap resp;
    resp["status"] = "something_unexpected";
    svc.processResponse(id, resp);

    QCOMPARE(svc.commandRecord(id).status, RemoteCommandService::CommandStatus::Error);
}

void RemoteCommandServiceTest::testResponseForUnknownId() {
    MobileClient client;
    client.setSimulationMode(true);
    client.connectToServer("host", 8080);
    RemoteCommandService svc(&client);

    QSignalSpy ackSpy(&svc, &RemoteCommandService::commandAcknowledged);
    QVariantMap resp;
    resp["status"] = "ok";
    svc.processResponse(999, resp); // unknown id
    QCOMPARE(ackSpy.count(), 0); // no signal for unknown id
}

void RemoteCommandServiceTest::testCommandSentSignal() {
    MobileClient client;
    client.setSimulationMode(true);
    client.connectToServer("host", 8080);
    RemoteCommandService svc(&client);

    QSignalSpy spy(&svc, &RemoteCommandService::commandSent);
    int id = svc.startCycle();
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().at(0).toInt(), id);
    QCOMPARE(spy.first().at(1).toString(), QString("startCycle"));
}

void RemoteCommandServiceTest::testCommandAcknowledgedSignal() {
    MobileClient client;
    client.setSimulationMode(true);
    client.connectToServer("host", 8080);
    RemoteCommandService svc(&client);

    QSignalSpy spy(&svc, &RemoteCommandService::commandAcknowledged);
    int id = svc.startCycle();

    QVariantMap resp;
    resp["status"] = "ack";
    resp["message"] = "Done";
    svc.processResponse(id, resp);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().at(0).toInt(), id);
    QCOMPARE(spy.first().at(1).toString(), QString("Done"));
}

void RemoteCommandServiceTest::testCommandRejectedSignal() {
    MobileClient client;
    client.setSimulationMode(true);
    client.connectToServer("host", 8080);
    RemoteCommandService svc(&client);

    QSignalSpy spy(&svc, &RemoteCommandService::commandRejected);
    int id = svc.startCycle();

    QVariantMap resp;
    resp["status"] = "error";
    resp["message"] = "Controller busy";
    svc.processResponse(id, resp);
    QCOMPARE(spy.count(), 1);
}

void RemoteCommandServiceTest::testCommandTimeoutSignal() {
    MobileClient client;
    client.setSimulationMode(true);
    client.connectToServer("host", 8080);
    RemoteCommandService svc(&client);

    QSignalSpy spy(&svc, &RemoteCommandService::commandTimeout);
    int id = svc.startCycle();

    QVariantMap resp;
    resp["status"] = "timeout";
    svc.processResponse(id, resp);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().at(0).toInt(), id);
}

void RemoteCommandServiceTest::testCommandErrorSignal() {
    MobileClient client;
    client.setSimulationMode(true);
    client.connectToServer("host", 8080);
    RemoteCommandService svc(&client);

    QSignalSpy spy(&svc, &RemoteCommandService::commandError);
    int id = svc.startCycle();

    QVariantMap resp;
    resp["status"] = "xyz";
    svc.processResponse(id, resp);
    QCOMPARE(spy.count(), 1);
}

void RemoteCommandServiceTest::testDefaultTimeout() {
    MobileClient client;
    RemoteCommandService svc(&client);
    QCOMPARE(svc.commandTimeoutMs(), 5000);
}

void RemoteCommandServiceTest::testSetTimeout() {
    MobileClient client;
    RemoteCommandService svc(&client);
    svc.setCommandTimeoutMs(10000);
    QCOMPARE(svc.commandTimeoutMs(), 10000);
}

void RemoteCommandServiceTest::testMultipleCommandsTracked() {
    MobileClient client;
    client.setSimulationMode(true);
    client.connectToServer("host", 8080);
    RemoteCommandService svc(&client);

    int id1 = svc.startCycle();
    int id2 = svc.setSpeed(50);
    int id3 = svc.loadRecipe("test");
    QCOMPARE(svc.pendingCommandCount(), 3);
    QVERIFY(id1 != id2);
    QVERIFY(id2 != id3);
}

void RemoteCommandServiceTest::testAckOneOfMultiple() {
    MobileClient client;
    client.setSimulationMode(true);
    client.connectToServer("host", 8080);
    RemoteCommandService svc(&client);

    int id1 = svc.startCycle();
    svc.setSpeed(50);
    svc.loadRecipe("test");

    QVariantMap resp;
    resp["status"] = "ok";
    svc.processResponse(id1, resp);

    QCOMPARE(svc.pendingCommandCount(), 2);
    QCOMPARE(svc.commandRecord(id1).status, RemoteCommandService::CommandStatus::Acknowledged);
}

void RemoteCommandServiceTest::testMixedResponses() {
    MobileClient client;
    client.setSimulationMode(true);
    client.connectToServer("host", 8080);
    RemoteCommandService svc(&client);

    int id1 = svc.startCycle();
    int id2 = svc.setSpeed(50);
    int id3 = svc.loadRecipe("test");

    svc.processResponse(id1, {{"status", "ok"}});
    svc.processResponse(id2, {{"status", "rejected"}, {"reason", "Speed limit"}});
    svc.processResponse(id3, {{"status", "timeout"}});

    QCOMPARE(svc.commandRecord(id1).status, RemoteCommandService::CommandStatus::Acknowledged);
    QCOMPARE(svc.commandRecord(id2).status, RemoteCommandService::CommandStatus::Rejected);
    QCOMPARE(svc.commandRecord(id3).status, RemoteCommandService::CommandStatus::Timeout);
    QCOMPARE(svc.pendingCommandCount(), 0);
}

void RemoteCommandServiceTest::testResponseViaClientSignal() {
    MobileClient client;
    client.setSimulationMode(true);
    client.connectToServer("host", 8080);
    RemoteCommandService svc(&client);

    int id = svc.startCycle();
    QSignalSpy spy(&svc, &RemoteCommandService::commandAcknowledged);

    // Inject response through client (simulates server response arriving)
    QVariantMap resp;
    resp["status"] = "ok";
    resp["message"] = "Started";
    client.injectSimResponse(id, resp);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(svc.commandRecord(id).status, RemoteCommandService::CommandStatus::Acknowledged);
}

void RemoteCommandServiceTest::testLoadRecipeParams() {
    MobileClient client;
    client.setSimulationMode(true);
    client.connectToServer("host", 8080);
    RemoteCommandService svc(&client);

    int id = svc.loadRecipe("my_recipe");
    auto rec = svc.commandRecord(id);
    QCOMPARE(rec.params["recipe"].toString(), QString("my_recipe"));
}

void RemoteCommandServiceTest::testSetSpeedParams() {
    MobileClient client;
    client.setSimulationMode(true);
    client.connectToServer("host", 8080);
    RemoteCommandService svc(&client);

    int id = svc.setSpeed(85.5);
    auto rec = svc.commandRecord(id);
    QCOMPARE(rec.params["speed"].toDouble(), 85.5);
}

QTEST_MAIN(RemoteCommandServiceTest)
#include "test_remotecommandservice.moc"
