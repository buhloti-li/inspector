#include "workcellserver.h"
#include "workcellcontroller.h"
#include <QJsonArray>

// ============================================================
// ClientSession
// ============================================================

ClientSession::ClientSession(QTcpSocket* socket, QObject* parent)
    : QObject(parent)
    , m_socket(socket) {
    connect(m_socket, &QTcpSocket::readyRead, this, &ClientSession::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &ClientSession::onDisconnected);
}

void ClientSession::sendJson(const QJsonObject& obj) {
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState)
        return;
    QByteArray data = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    data.append('\n');
    m_socket->write(data);
    m_socket->flush();
}

void ClientSession::sendError(const QString& error) {
    QJsonObject resp;
    resp["status"] = QStringLiteral("error");
    resp["message"] = error;
    sendJson(resp);
}

void ClientSession::sendStatusUpdate(const QJsonObject& status) {
    QJsonObject msg;
    msg["event"] = QStringLiteral("statusUpdate");
    msg["data"] = status;
    sendJson(msg);
}

QString ClientSession::peerAddress() const {
    if (!m_socket) return {};
    return m_socket->peerAddress().toString() + ":" + QString::number(m_socket->peerPort());
}

bool ClientSession::isConnected() const {
    return m_socket && m_socket->state() == QAbstractSocket::ConnectedState;
}

void ClientSession::onReadyRead() {
    m_buffer.append(m_socket->readAll());

    // Process newline-delimited JSON messages
    while (true) {
        int idx = m_buffer.indexOf('\n');
        if (idx < 0)
            break;
        QByteArray line = m_buffer.left(idx).trimmed();
        m_buffer.remove(0, idx + 1);
        if (!line.isEmpty())
            processMessage(line);
    }

    // Prevent buffer overflow from malicious clients
    if (m_buffer.size() > 1024 * 1024) {
        sendError(QStringLiteral("Message too large"));
        m_buffer.clear();
    }
}

void ClientSession::onDisconnected() {
    emit disconnected();
}

void ClientSession::processMessage(const QByteArray& message) {
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(message, &err);
    if (err.error != QJsonParseError::NoError) {
        sendError(QStringLiteral("Invalid JSON: ") + err.errorString());
        return;
    }

    QJsonObject obj = doc.object();
    QString cmd = obj.value("cmd").toString();
    QJsonObject params = obj.value("params").toObject();
    int requestId = obj.value("id").toInt(-1);

    if (cmd.isEmpty()) {
        sendError(QStringLiteral("Missing 'cmd' field"));
        return;
    }

    emit commandReceived(cmd, params, requestId);
}

// ============================================================
// WorkcellServer
// ============================================================

WorkcellServer::WorkcellServer(QObject* parent)
    : QObject(parent)
    , m_server(new QTcpServer(this)) {
    connect(m_server, &QTcpServer::newConnection, this, &WorkcellServer::onNewConnection);
    connect(&m_broadcastTimer, &QTimer::timeout, this, &WorkcellServer::broadcastStatus);
}

bool WorkcellServer::start(quint16 port) {
    if (m_server->isListening())
        return true;

    if (!m_server->listen(QHostAddress::Any, port)) {
        emit serverError(m_server->errorString());
        return false;
    }

    m_broadcastTimer.start(1000);
    emit started(m_server->serverPort());
    return true;
}

void WorkcellServer::stop() {
    m_broadcastTimer.stop();

    for (auto* session : m_clients) {
        session->deleteLater();
    }
    m_clients.clear();

    if (m_server->isListening())
        m_server->close();

    emit stopped();
}

bool WorkcellServer::isListening() const {
    return m_server->isListening();
}

quint16 WorkcellServer::serverPort() const {
    return m_server->serverPort();
}

int WorkcellServer::clientCount() const {
    return m_clients.size();
}

void WorkcellServer::setController(WorkcellController* controller) {
    m_controller = controller;
}

void WorkcellServer::setStatusBroadcastInterval(int ms) {
    if (m_broadcastTimer.isActive())
        m_broadcastTimer.start(ms);
    else
        m_broadcastTimer.setInterval(ms);
}

void WorkcellServer::onNewConnection() {
    while (m_server->hasPendingConnections()) {
        QTcpSocket* socket = m_server->nextPendingConnection();
        auto* session = new ClientSession(socket, this);

        connect(session, &ClientSession::commandReceived,
                this, [this, session](const QString& cmd, const QJsonObject& params, int id) {
            handleCommand(session, cmd, params, id);
        });
        connect(session, &ClientSession::disconnected,
                this, &WorkcellServer::onClientDisconnected);

        m_clients.append(session);
        emit clientConnected(session->peerAddress());

        // Send initial status
        session->sendStatusUpdate(buildStatusSnapshot());
    }
}

void WorkcellServer::onClientCommand(const QString& command,
                                      const QJsonObject& params, int requestId) {
    auto* session = qobject_cast<ClientSession*>(sender());
    if (session)
        handleCommand(session, command, params, requestId);
}

void WorkcellServer::onClientDisconnected() {
    auto* session = qobject_cast<ClientSession*>(sender());
    if (!session) return;

    QString addr = session->peerAddress();
    m_clients.removeAll(session);
    session->deleteLater();
    emit clientDisconnected(addr);
}

void WorkcellServer::broadcastStatus() {
    if (m_clients.isEmpty())
        return;

    QJsonObject status = buildStatusSnapshot();
    for (auto* session : m_clients) {
        session->sendStatusUpdate(status);
    }
}

QJsonObject WorkcellServer::buildStatusSnapshot() const {
    QJsonObject status;

    if (!m_controller) {
        status["state"] = QStringLiteral("Unknown");
        return status;
    }

    // Map controller state
    switch (m_controller->state()) {
        case WorkcellController::State::Idle:
            status["state"] = QStringLiteral("Idle"); break;
        case WorkcellController::State::Running:
            status["state"] = QStringLiteral("Running"); break;
        case WorkcellController::State::Stopped:
            status["state"] = QStringLiteral("Stopped"); break;
        case WorkcellController::State::Fault:
            status["state"] = QStringLiteral("Fault"); break;
        case WorkcellController::State::Degraded:
            status["state"] = QStringLiteral("Degraded"); break;
    }

    auto s = m_controller->stats();
    status["cycleCount"] = s.totalAttempts;
    status["successCount"] = s.successCount;
    status["failCount"] = s.failureCount;
    status["autoMode"] = m_controller->isAutoMode();

    return status;
}

void WorkcellServer::handleCommand(ClientSession* session, const QString& cmd,
                                    const QJsonObject& params, int requestId) {
    QJsonObject resp;
    resp["id"] = requestId;

    if (!m_controller) {
        resp["status"] = QStringLiteral("error");
        resp["message"] = QStringLiteral("No controller configured");
        session->sendJson(resp);
        return;
    }

    if (cmd == "status") {
        resp["status"] = QStringLiteral("ok");
        resp["data"] = buildStatusSnapshot();
    }
    else if (cmd == "startCycle") {
        QString err;
        bool ok = m_controller->startTask(&err);
        resp["status"] = ok ? QStringLiteral("ok") : QStringLiteral("error");
        if (!ok) resp["message"] = err;
        else resp["message"] = QStringLiteral("Cycle started");
    }
    else if (cmd == "stopCycle") {
        QString err;
        bool ok = m_controller->stopTask(&err);
        resp["status"] = ok ? QStringLiteral("ok") : QStringLiteral("error");
        if (!ok) resp["message"] = err;
        else resp["message"] = QStringLiteral("Cycle stopped");
    }
    else if (cmd == "emergencyStop") {
        m_controller->emergencyStop();
        resp["status"] = QStringLiteral("ok");
        resp["message"] = QStringLiteral("Emergency stop activated");
    }
    else if (cmd == "loadRecipe") {
        QString recipe = params.value("recipe").toString();
        if (recipe.isEmpty()) {
            resp["status"] = QStringLiteral("error");
            resp["message"] = QStringLiteral("Missing recipe name");
        } else {
            QVariantMap recipeMap;
            recipeMap["name"] = recipe;
            m_controller->loadRecipe(recipeMap);
            resp["status"] = QStringLiteral("ok");
            resp["message"] = QStringLiteral("Recipe loaded: ") + recipe;
        }
    }
    else if (cmd == "setAutoMode") {
        bool enabled = params.value("enabled").toBool();
        if (enabled) {
            QString err;
            bool ok = m_controller->startAutoMode(&err);
            resp["status"] = ok ? QStringLiteral("ok") : QStringLiteral("error");
            if (!ok) resp["message"] = err;
        } else {
            m_controller->stopAutoMode();
            resp["status"] = QStringLiteral("ok");
        }
    }
    else if (cmd == "reset") {
        m_controller->resetSession();
        resp["status"] = QStringLiteral("ok");
        resp["message"] = QStringLiteral("Controller reset");
    }
    else {
        resp["status"] = QStringLiteral("error");
        resp["message"] = QStringLiteral("Unknown command: ") + cmd;
    }

    session->sendJson(resp);
    emit commandProcessed(cmd, requestId);
}
