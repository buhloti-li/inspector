#include <QtTest>
#include <QSignalSpy>
#include "mobile/mobileclient.h"

class MobileClientTest : public QObject {
    Q_OBJECT
private slots:
    // === Connection ===
    void testInitialState();
    void testConnectSuccess();
    void testConnectAlreadyConnected();
    void testConnectInvalidHost();
    void testConnectInvalidPort();
    void testConnectEmptyHost();
    void testConnectPortZero();
    void testConnectPortNegative();
    void testConnectPortOverflow();
    void testDisconnect();
    void testDisconnectWhenAlreadyDisconnected();
    void testServerInfoAfterConnect();

    // === Simulation Mode ===
    void testSimulationModeDefault();
    void testSimulationModeConnect();
    void testSimulateDisconnect();
    void testSimulateReconnect();
    void testSimulateDisconnectWhenAlreadyDisconnected();
    void testInjectSimResponse();
    void testInjectSimResponseWithState();
    void testInjectSimResponseNotInSimMode();

    // === Commands ===
    void testSendCommandConnected();
    void testSendCommandDisconnected();
    void testSendCommandEmptyCommand();
    void testSendCommandIncrementingIds();
    void testRequestStatus();

    // === Reconnection ===
    void testReconnectAttemptsDefault();
    void testSetMaxReconnectAttempts();
    void testSetReconnectInterval();
    void testReconnectTimerTriggersAttempt();
    void testMaxReconnectAttemptsExceeded();

    // === Signals ===
    void testConnectedSignal();
    void testDisconnectedSignal();
    void testConnectionStateChangedSignal();
    void testErrorOccurredSignal();
    void testResponseReceivedSignal();
    void testStatusUpdatedSignal();
    void testReconnectAttemptSignal();

    // === Edge Cases ===
    void testConnectDisconnectCycle();
    void testMultipleConnections();
    void testSendMultipleCommands();
};

void MobileClientTest::testInitialState() {
    MobileClient client;
    QCOMPARE(client.connectionState(), MobileClient::ConnectionState::Disconnected);
    QVERIFY(client.serverHost().isEmpty());
    QCOMPARE(client.serverPort(), 0);
    QCOMPARE(client.reconnectAttempts(), 0);
    QCOMPARE(client.maxReconnectAttempts(), 5);
    QVERIFY(!client.isSimulationMode());
}

void MobileClientTest::testConnectSuccess() {
    MobileClient client;
    bool ok = client.connectToServer("192.168.1.100", 8080);
    QVERIFY(ok);
    QCOMPARE(client.connectionState(), MobileClient::ConnectionState::Connected);
}

void MobileClientTest::testConnectAlreadyConnected() {
    MobileClient client;
    client.connectToServer("192.168.1.100", 8080);
    bool ok = client.connectToServer("192.168.1.200", 9090);
    QVERIFY(ok); // returns true when already connected
    QCOMPARE(client.connectionState(), MobileClient::ConnectionState::Connected);
}

void MobileClientTest::testConnectInvalidHost() {
    MobileClient client;
    bool ok = client.connectToServer("", 8080);
    QVERIFY(!ok);
    QCOMPARE(client.connectionState(), MobileClient::ConnectionState::Error);
    QVERIFY(!client.lastError().isEmpty());
}

void MobileClientTest::testConnectInvalidPort() {
    MobileClient client;
    bool ok = client.connectToServer("host", 99999);
    QVERIFY(!ok);
    QCOMPARE(client.connectionState(), MobileClient::ConnectionState::Error);
}

void MobileClientTest::testConnectEmptyHost() {
    MobileClient client;
    QSignalSpy errorSpy(&client, &MobileClient::errorOccurred);
    bool ok = client.connectToServer("", 1234);
    QVERIFY(!ok);
    QCOMPARE(errorSpy.count(), 1);
    QVERIFY(errorSpy.first().first().toString().contains("Invalid"));
}

void MobileClientTest::testConnectPortZero() {
    MobileClient client;
    bool ok = client.connectToServer("host", 0);
    QVERIFY(!ok);
    QCOMPARE(client.connectionState(), MobileClient::ConnectionState::Error);
}

void MobileClientTest::testConnectPortNegative() {
    MobileClient client;
    bool ok = client.connectToServer("host", -1);
    QVERIFY(!ok);
}

void MobileClientTest::testConnectPortOverflow() {
    MobileClient client;
    bool ok = client.connectToServer("host", 70000);
    QVERIFY(!ok);
}

void MobileClientTest::testDisconnect() {
    MobileClient client;
    client.connectToServer("host", 8080);
    client.disconnectFromServer();
    QCOMPARE(client.connectionState(), MobileClient::ConnectionState::Disconnected);
}

void MobileClientTest::testDisconnectWhenAlreadyDisconnected() {
    MobileClient client;
    QSignalSpy spy(&client, &MobileClient::disconnected);
    client.disconnectFromServer();
    QCOMPARE(spy.count(), 0); // no signal when already disconnected
}

void MobileClientTest::testServerInfoAfterConnect() {
    MobileClient client;
    client.connectToServer("10.0.0.1", 9999);
    QCOMPARE(client.serverHost(), QString("10.0.0.1"));
    QCOMPARE(client.serverPort(), 9999);
}

void MobileClientTest::testSimulationModeDefault() {
    MobileClient client;
    QVERIFY(!client.isSimulationMode());
}

void MobileClientTest::testSimulationModeConnect() {
    MobileClient client;
    client.setSimulationMode(true);
    QVERIFY(client.isSimulationMode());
    bool ok = client.connectToServer("sim-host", 5555);
    QVERIFY(ok);
    QCOMPARE(client.connectionState(), MobileClient::ConnectionState::Connected);
}

void MobileClientTest::testSimulateDisconnect() {
    MobileClient client;
    client.setSimulationMode(true);
    client.connectToServer("host", 8080);

    QSignalSpy errorSpy(&client, &MobileClient::errorOccurred);
    client.simulateDisconnect();
    QCOMPARE(client.connectionState(), MobileClient::ConnectionState::Reconnecting);
    QCOMPARE(errorSpy.count(), 1);
    QVERIFY(errorSpy.first().first().toString().contains("Connection lost"));
}

void MobileClientTest::testSimulateReconnect() {
    MobileClient client;
    client.setSimulationMode(true);
    client.connectToServer("host", 8080);
    client.simulateDisconnect();

    QSignalSpy connSpy(&client, &MobileClient::connected);
    client.simulateReconnect();
    QCOMPARE(client.connectionState(), MobileClient::ConnectionState::Connected);
    QCOMPARE(connSpy.count(), 1);
}

void MobileClientTest::testSimulateDisconnectWhenAlreadyDisconnected() {
    MobileClient client;
    client.setSimulationMode(true);
    // Not connected
    QSignalSpy errorSpy(&client, &MobileClient::errorOccurred);
    client.simulateDisconnect();
    QCOMPARE(errorSpy.count(), 0); // no-op
}

void MobileClientTest::testInjectSimResponse() {
    MobileClient client;
    client.setSimulationMode(true);
    client.connectToServer("host", 8080);

    QSignalSpy respSpy(&client, &MobileClient::responseReceived);
    QVariantMap resp;
    resp["result"] = "success";
    client.injectSimResponse(42, resp);
    QCOMPARE(respSpy.count(), 1);
    QCOMPARE(respSpy.first().at(0).toInt(), 42);
}

void MobileClientTest::testInjectSimResponseWithState() {
    MobileClient client;
    client.setSimulationMode(true);
    client.connectToServer("host", 8080);

    QSignalSpy statusSpy(&client, &MobileClient::statusUpdated);
    QVariantMap resp;
    resp["state"] = "Running";
    client.injectSimResponse(1, resp);
    QCOMPARE(statusSpy.count(), 1);
    QCOMPARE(statusSpy.first().first().toMap()["state"].toString(), QString("Running"));
}

void MobileClientTest::testInjectSimResponseNotInSimMode() {
    MobileClient client;
    // Not in sim mode
    QSignalSpy respSpy(&client, &MobileClient::responseReceived);
    client.injectSimResponse(1, {});
    QCOMPARE(respSpy.count(), 0); // ignored
}

void MobileClientTest::testSendCommandConnected() {
    MobileClient client;
    client.connectToServer("host", 8080);
    int id = client.sendCommand("test_cmd");
    QVERIFY(id > 0);
}

void MobileClientTest::testSendCommandDisconnected() {
    MobileClient client;
    QSignalSpy errorSpy(&client, &MobileClient::errorOccurred);
    int id = client.sendCommand("test_cmd");
    QCOMPARE(id, -1);
    QCOMPARE(errorSpy.count(), 1);
}

void MobileClientTest::testSendCommandEmptyCommand() {
    MobileClient client;
    client.connectToServer("host", 8080);
    int id = client.sendCommand("");
    QCOMPARE(id, -1);
}

void MobileClientTest::testSendCommandIncrementingIds() {
    MobileClient client;
    client.connectToServer("host", 8080);
    int id1 = client.sendCommand("cmd1");
    int id2 = client.sendCommand("cmd2");
    int id3 = client.sendCommand("cmd3");
    QVERIFY(id1 > 0);
    QVERIFY(id2 > id1);
    QVERIFY(id3 > id2);
}

void MobileClientTest::testRequestStatus() {
    MobileClient client;
    client.connectToServer("host", 8080);
    int id = client.requestStatus();
    QVERIFY(id > 0);
}

void MobileClientTest::testReconnectAttemptsDefault() {
    MobileClient client;
    QCOMPARE(client.maxReconnectAttempts(), 5);
    QCOMPARE(client.reconnectIntervalMs(), 2000);
}

void MobileClientTest::testSetMaxReconnectAttempts() {
    MobileClient client;
    client.setMaxReconnectAttempts(10);
    QCOMPARE(client.maxReconnectAttempts(), 10);
}

void MobileClientTest::testSetReconnectInterval() {
    MobileClient client;
    client.setReconnectIntervalMs(500);
    QCOMPARE(client.reconnectIntervalMs(), 500);
}

void MobileClientTest::testReconnectTimerTriggersAttempt() {
    MobileClient client;
    client.setSimulationMode(true);
    client.setReconnectIntervalMs(50); // fast for test
    client.connectToServer("host", 8080);

    QSignalSpy attemptSpy(&client, &MobileClient::reconnectAttemptStarted);
    client.simulateDisconnect();

    // Wait for first reconnect attempt
    QTest::qWait(100);
    QVERIFY(attemptSpy.count() >= 1);
    QCOMPARE(attemptSpy.first().first().toInt(), 1);
}

void MobileClientTest::testMaxReconnectAttemptsExceeded() {
    MobileClient client;
    client.setSimulationMode(true);
    client.setMaxReconnectAttempts(1);
    client.setReconnectIntervalMs(10);
    client.connectToServer("host", 8080);

    QSignalSpy errorSpy(&client, &MobileClient::errorOccurred);
    QSignalSpy stateSpy(&client, &MobileClient::connectionStateChanged);
    client.simulateDisconnect();

    // Wait for reconnect attempts to exhaust
    QTest::qWait(100);
    QCOMPARE(client.connectionState(), MobileClient::ConnectionState::Error);

    // Check that error contains max attempts message
    bool foundMaxError = false;
    for (const auto& args : errorSpy) {
        if (args.first().toString().contains("Max reconnect"))
            foundMaxError = true;
    }
    QVERIFY(foundMaxError);
}

void MobileClientTest::testConnectedSignal() {
    MobileClient client;
    QSignalSpy spy(&client, &MobileClient::connected);
    client.connectToServer("host", 8080);
    QCOMPARE(spy.count(), 1);
}

void MobileClientTest::testDisconnectedSignal() {
    MobileClient client;
    client.connectToServer("host", 8080);
    QSignalSpy spy(&client, &MobileClient::disconnected);
    client.disconnectFromServer();
    QCOMPARE(spy.count(), 1);
}

void MobileClientTest::testConnectionStateChangedSignal() {
    MobileClient client;
    QSignalSpy spy(&client, &MobileClient::connectionStateChanged);
    client.connectToServer("host", 8080);
    // Should get Connecting then Connected
    QCOMPARE(spy.count(), 2);
}

void MobileClientTest::testErrorOccurredSignal() {
    MobileClient client;
    QSignalSpy spy(&client, &MobileClient::errorOccurred);
    client.connectToServer("", 8080);
    QCOMPARE(spy.count(), 1);
}

void MobileClientTest::testResponseReceivedSignal() {
    MobileClient client;
    client.setSimulationMode(true);
    client.connectToServer("host", 8080);

    QSignalSpy spy(&client, &MobileClient::responseReceived);
    QVariantMap resp;
    resp["data"] = 123;
    client.injectSimResponse(5, resp);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().at(0).toInt(), 5);
}

void MobileClientTest::testStatusUpdatedSignal() {
    MobileClient client;
    client.setSimulationMode(true);
    client.connectToServer("host", 8080);

    QSignalSpy spy(&client, &MobileClient::statusUpdated);
    QVariantMap status;
    status["state"] = "Idle";
    client.injectSimResponse(1, status);
    QCOMPARE(spy.count(), 1);
}

void MobileClientTest::testReconnectAttemptSignal() {
    MobileClient client;
    client.setSimulationMode(true);
    client.setReconnectIntervalMs(10);
    client.connectToServer("host", 8080);

    QSignalSpy spy(&client, &MobileClient::reconnectAttemptStarted);
    client.simulateDisconnect();
    QTest::qWait(50);
    QVERIFY(spy.count() >= 1);
}

void MobileClientTest::testConnectDisconnectCycle() {
    MobileClient client;
    for (int i = 0; i < 5; ++i) {
        QVERIFY(client.connectToServer("host", 8080));
        QCOMPARE(client.connectionState(), MobileClient::ConnectionState::Connected);
        client.disconnectFromServer();
        QCOMPARE(client.connectionState(), MobileClient::ConnectionState::Disconnected);
    }
}

void MobileClientTest::testMultipleConnections() {
    MobileClient client;
    client.connectToServer("host1", 1111);
    QCOMPARE(client.serverHost(), QString("host1"));
    client.disconnectFromServer();
    client.connectToServer("host2", 2222);
    QCOMPARE(client.serverHost(), QString("host2"));
    QCOMPARE(client.serverPort(), 2222);
}

void MobileClientTest::testSendMultipleCommands() {
    MobileClient client;
    client.setSimulationMode(true);
    client.connectToServer("host", 8080);

    QVector<int> ids;
    for (int i = 0; i < 10; ++i) {
        int id = client.sendCommand(QString("cmd_%1").arg(i));
        QVERIFY(id > 0);
        ids.append(id);
    }

    // All IDs should be unique
    QSet<int> uniqueIds(ids.begin(), ids.end());
    QCOMPARE(uniqueIds.size(), 10);
}

QTEST_MAIN(MobileClientTest)
#include "test_mobileclient.moc"
