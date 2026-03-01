#include <QtTest>
#include <QSignalSpy>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include "server/workcellserver.h"
#include "workcellcontroller.h"

class WorkcellServerTest : public QObject {
    Q_OBJECT
private:
    // Helper: connect a TCP client, wait for event loop to process
    QTcpSocket* connectClient(quint16 port) {
        auto* sock = new QTcpSocket(this);
        sock->connectToHost("127.0.0.1", port);
        sock->waitForConnected(2000);
        QTest::qWait(100); // let server process the connection
        return sock;
    }

    // Helper: read all available data, pumping event loop
    QByteArray readAll(QTcpSocket* sock, int waitMs = 200) {
        QTest::qWait(waitMs);
        return sock->readAll();
    }

    // Helper: send JSON command and read response
    QJsonObject sendCommand(QTcpSocket* sock, const QString& cmd,
                            const QJsonObject& params = {}, int id = 1) {
        QJsonObject req;
        req["cmd"] = cmd;
        req["params"] = params;
        req["id"] = id;
        QByteArray data = QJsonDocument(req).toJson(QJsonDocument::Compact);
        data.append('\n');
        sock->write(data);
        sock->flush();

        // Wait for response with event loop processing
        QTest::qWait(200);
        QByteArray all = sock->readAll();

        // Find the line matching our request id
        QList<QByteArray> lines = all.split('\n');
        for (int i = lines.size() - 1; i >= 0; --i) {
            QByteArray line = lines[i].trimmed();
            if (line.isEmpty()) continue;
            QJsonDocument doc = QJsonDocument::fromJson(line);
            QJsonObject obj = doc.object();
            if (obj.contains("id") && obj["id"].toInt() == id)
                return obj;
        }
        // Fallback: return last non-empty parsed line
        for (int i = lines.size() - 1; i >= 0; --i) {
            QByteArray line = lines[i].trimmed();
            if (!line.isEmpty())
                return QJsonDocument::fromJson(line).object();
        }
        return {};
    }

    // Helper: drain initial status push
    void drainInitialStatus(QTcpSocket* sock) {
        QTest::qWait(200);
        sock->readAll(); // discard
    }

private slots:
    // === Server Lifecycle ===
    void testStartStop();
    void testStartOnRandomPort();
    void testDoubleStart();
    void testStopWithoutStart();
    void testIsListening();

    // === Client Connection ===
    void testClientConnects();
    void testMultipleClients();
    void testClientDisconnect();
    void testClientCountTracking();

    // === Signals ===
    void testStartedSignal();
    void testStoppedSignal();
    void testClientConnectedSignal();
    void testClientDisconnectedSignal();

    // === Commands ===
    void testStatusCommand();
    void testEmergencyStopCommand();
    void testLoadRecipeCommand();
    void testLoadRecipeEmptyName();
    void testSetAutoModeOff();
    void testResetCommand();
    void testUnknownCommand();

    // === Protocol ===
    void testInvalidJson();
    void testMissingCmdField();
    void testNoControllerConfigured();

    // === Status Broadcast ===
    void testStatusBroadcastInterval();
    void testInitialStatusOnConnect();

    // === Edge Cases ===
    void testServerPortAccessible();
    void testCommandProcessedSignal();
};

void WorkcellServerTest::testStartStop() {
    WorkcellServer server;
    QVERIFY(server.start(0));
    QVERIFY(server.isListening());
    server.stop();
    QVERIFY(!server.isListening());
}

void WorkcellServerTest::testStartOnRandomPort() {
    WorkcellServer server;
    QVERIFY(server.start(0));
    QVERIFY(server.serverPort() > 0);
    server.stop();
}

void WorkcellServerTest::testDoubleStart() {
    WorkcellServer server;
    QVERIFY(server.start(0));
    quint16 port = server.serverPort();
    QVERIFY(server.start(0));
    QCOMPARE(server.serverPort(), port);
    server.stop();
}

void WorkcellServerTest::testStopWithoutStart() {
    WorkcellServer server;
    server.stop(); // should not crash
}

void WorkcellServerTest::testIsListening() {
    WorkcellServer server;
    QVERIFY(!server.isListening());
    server.start(0);
    QVERIFY(server.isListening());
    server.stop();
    QVERIFY(!server.isListening());
}

void WorkcellServerTest::testClientConnects() {
    WorkcellServer server;
    WorkcellController controller;
    server.setController(&controller);
    server.start(0);

    auto* sock = connectClient(server.serverPort());
    QCOMPARE(server.clientCount(), 1);

    sock->disconnectFromHost();
    QTest::qWait(100);
    server.stop();
}

void WorkcellServerTest::testMultipleClients() {
    WorkcellServer server;
    WorkcellController controller;
    server.setController(&controller);
    server.start(0);

    auto* c1 = connectClient(server.serverPort());
    auto* c2 = connectClient(server.serverPort());
    auto* c3 = connectClient(server.serverPort());
    QCOMPARE(server.clientCount(), 3);

    c1->disconnectFromHost();
    c2->disconnectFromHost();
    c3->disconnectFromHost();
    QTest::qWait(100);
    server.stop();
}

void WorkcellServerTest::testClientDisconnect() {
    WorkcellServer server;
    WorkcellController controller;
    server.setController(&controller);
    server.start(0);

    auto* sock = connectClient(server.serverPort());
    QCOMPARE(server.clientCount(), 1);

    sock->disconnectFromHost();
    QTest::qWait(200);
    QCOMPARE(server.clientCount(), 0);

    server.stop();
}

void WorkcellServerTest::testClientCountTracking() {
    WorkcellServer server;
    server.start(0);
    QCOMPARE(server.clientCount(), 0);

    auto* c1 = connectClient(server.serverPort());
    QCOMPARE(server.clientCount(), 1);

    auto* c2 = connectClient(server.serverPort());
    QCOMPARE(server.clientCount(), 2);

    c1->disconnectFromHost();
    QTest::qWait(200);
    QCOMPARE(server.clientCount(), 1);

    c2->disconnectFromHost();
    QTest::qWait(200);
    QCOMPARE(server.clientCount(), 0);

    server.stop();
}

void WorkcellServerTest::testStartedSignal() {
    WorkcellServer server;
    QSignalSpy spy(&server, &WorkcellServer::started);
    server.start(0);
    QCOMPARE(spy.count(), 1);
    QVERIFY(spy.first().first().toUInt() > 0);
    server.stop();
}

void WorkcellServerTest::testStoppedSignal() {
    WorkcellServer server;
    server.start(0);
    QSignalSpy spy(&server, &WorkcellServer::stopped);
    server.stop();
    QCOMPARE(spy.count(), 1);
}

void WorkcellServerTest::testClientConnectedSignal() {
    WorkcellServer server;
    server.start(0);
    QSignalSpy spy(&server, &WorkcellServer::clientConnected);

    auto* sock = connectClient(server.serverPort());
    QCOMPARE(spy.count(), 1);
    QVERIFY(!spy.first().first().toString().isEmpty());

    sock->disconnectFromHost();
    QTest::qWait(100);
    server.stop();
}

void WorkcellServerTest::testClientDisconnectedSignal() {
    WorkcellServer server;
    server.start(0);

    auto* sock = connectClient(server.serverPort());
    QSignalSpy spy(&server, &WorkcellServer::clientDisconnected);
    sock->disconnectFromHost();
    QTest::qWait(200);
    QCOMPARE(spy.count(), 1);

    server.stop();
}

void WorkcellServerTest::testStatusCommand() {
    WorkcellServer server;
    WorkcellController controller;
    server.setController(&controller);
    server.start(0);

    auto* sock = connectClient(server.serverPort());
    drainInitialStatus(sock);

    auto resp = sendCommand(sock, "status");
    QCOMPARE(resp["status"].toString(), QString("ok"));
    QJsonObject data = resp["data"].toObject();
    QCOMPARE(data["state"].toString(), QString("Idle"));

    sock->disconnectFromHost();
    QTest::qWait(100);
    server.stop();
}

void WorkcellServerTest::testEmergencyStopCommand() {
    WorkcellServer server;
    WorkcellController controller;
    server.setController(&controller);
    server.start(0);

    auto* sock = connectClient(server.serverPort());
    drainInitialStatus(sock);

    auto resp = sendCommand(sock, "emergencyStop");
    QCOMPARE(resp["status"].toString(), QString("ok"));
    QVERIFY(resp["message"].toString().contains("Emergency"));

    sock->disconnectFromHost();
    QTest::qWait(100);
    server.stop();
}

void WorkcellServerTest::testLoadRecipeCommand() {
    WorkcellServer server;
    WorkcellController controller;
    server.setController(&controller);
    server.start(0);

    auto* sock = connectClient(server.serverPort());
    drainInitialStatus(sock);

    QJsonObject params;
    params["recipe"] = "bolt_m8";
    auto resp = sendCommand(sock, "loadRecipe", params);
    QCOMPARE(resp["status"].toString(), QString("ok"));
    QVERIFY(resp["message"].toString().contains("bolt_m8"));

    sock->disconnectFromHost();
    QTest::qWait(100);
    server.stop();
}

void WorkcellServerTest::testLoadRecipeEmptyName() {
    WorkcellServer server;
    WorkcellController controller;
    server.setController(&controller);
    server.start(0);

    auto* sock = connectClient(server.serverPort());
    drainInitialStatus(sock);

    auto resp = sendCommand(sock, "loadRecipe", {});
    QCOMPARE(resp["status"].toString(), QString("error"));

    sock->disconnectFromHost();
    QTest::qWait(100);
    server.stop();
}

void WorkcellServerTest::testSetAutoModeOff() {
    WorkcellServer server;
    WorkcellController controller;
    server.setController(&controller);
    server.start(0);

    auto* sock = connectClient(server.serverPort());
    drainInitialStatus(sock);

    QJsonObject params;
    params["enabled"] = false;
    auto resp = sendCommand(sock, "setAutoMode", params);
    QCOMPARE(resp["status"].toString(), QString("ok"));

    sock->disconnectFromHost();
    QTest::qWait(100);
    server.stop();
}

void WorkcellServerTest::testResetCommand() {
    WorkcellServer server;
    WorkcellController controller;
    server.setController(&controller);
    server.start(0);

    auto* sock = connectClient(server.serverPort());
    drainInitialStatus(sock);

    auto resp = sendCommand(sock, "reset");
    QCOMPARE(resp["status"].toString(), QString("ok"));

    sock->disconnectFromHost();
    QTest::qWait(100);
    server.stop();
}

void WorkcellServerTest::testUnknownCommand() {
    WorkcellServer server;
    WorkcellController controller;
    server.setController(&controller);
    server.start(0);

    auto* sock = connectClient(server.serverPort());
    drainInitialStatus(sock);

    auto resp = sendCommand(sock, "nonexistent_command");
    QCOMPARE(resp["status"].toString(), QString("error"));
    QVERIFY(resp["message"].toString().contains("Unknown"));

    sock->disconnectFromHost();
    QTest::qWait(100);
    server.stop();
}

void WorkcellServerTest::testInvalidJson() {
    WorkcellServer server;
    WorkcellController controller;
    server.setController(&controller);
    server.start(0);

    auto* sock = connectClient(server.serverPort());
    drainInitialStatus(sock);

    sock->write("this is not json\n");
    sock->flush();
    QTest::qWait(200);
    QByteArray resp = sock->readAll();
    QVERIFY(resp.contains("Invalid JSON"));

    sock->disconnectFromHost();
    QTest::qWait(100);
    server.stop();
}

void WorkcellServerTest::testMissingCmdField() {
    WorkcellServer server;
    WorkcellController controller;
    server.setController(&controller);
    server.start(0);

    auto* sock = connectClient(server.serverPort());
    drainInitialStatus(sock);

    sock->write("{\"params\":{}}\n");
    sock->flush();
    QTest::qWait(200);
    QByteArray resp = sock->readAll();
    QVERIFY(resp.contains("Missing"));

    sock->disconnectFromHost();
    QTest::qWait(100);
    server.stop();
}

void WorkcellServerTest::testNoControllerConfigured() {
    WorkcellServer server;
    server.start(0);

    auto* sock = connectClient(server.serverPort());
    drainInitialStatus(sock);

    auto resp = sendCommand(sock, "status");
    QCOMPARE(resp["status"].toString(), QString("error"));
    QVERIFY(resp["message"].toString().contains("No controller"));

    sock->disconnectFromHost();
    QTest::qWait(100);
    server.stop();
}

void WorkcellServerTest::testStatusBroadcastInterval() {
    WorkcellServer server;
    server.setStatusBroadcastInterval(100);
    server.start(0);
    server.setStatusBroadcastInterval(50);
    server.stop();
}

void WorkcellServerTest::testInitialStatusOnConnect() {
    WorkcellServer server;
    WorkcellController controller;
    server.setController(&controller);
    server.start(0);

    auto* sock = connectClient(server.serverPort());
    // connectClient already waits 100ms; data should be available
    QByteArray data = sock->readAll();
    QVERIFY(!data.isEmpty());
    QJsonDocument doc = QJsonDocument::fromJson(data.split('\n').first());
    QJsonObject obj = doc.object();
    QCOMPARE(obj["event"].toString(), QString("statusUpdate"));

    sock->disconnectFromHost();
    QTest::qWait(100);
    server.stop();
}

void WorkcellServerTest::testServerPortAccessible() {
    WorkcellServer server;
    QCOMPARE(server.serverPort(), quint16(0));
    server.start(0);
    QVERIFY(server.serverPort() > 0);
    server.stop();
}

void WorkcellServerTest::testCommandProcessedSignal() {
    WorkcellServer server;
    WorkcellController controller;
    server.setController(&controller);
    server.start(0);

    auto* sock = connectClient(server.serverPort());
    drainInitialStatus(sock);

    QSignalSpy spy(&server, &WorkcellServer::commandProcessed);
    sendCommand(sock, "status");
    QVERIFY(spy.count() >= 1);
    QCOMPARE(spy.first().at(0).toString(), QString("status"));

    sock->disconnectFromHost();
    QTest::qWait(100);
    server.stop();
}

QTEST_MAIN(WorkcellServerTest)
#include "test_workcellserver.moc"
