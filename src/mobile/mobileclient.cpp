#include "mobileclient.h"

MobileClient::MobileClient(QObject* parent)
    : QObject(parent) {
    m_reconnectTimer.setSingleShot(true);
    QObject::connect(&m_reconnectTimer, &QTimer::timeout, this, &MobileClient::onReconnectTimer);
}

bool MobileClient::connectToServer(const QString& host, int port) {
    if (host.isEmpty() || port <= 0 || port > 65535) {
        m_lastError = QStringLiteral("Invalid host or port");
        setState(ConnectionState::Error);
        emit errorOccurred(m_lastError);
        return false;
    }

    if (m_state == ConnectionState::Connected) {
        return true; // already connected
    }

    m_host = host;
    m_port = port;
    m_reconnectAttempts = 0;

    setState(ConnectionState::Connecting);

    if (m_simulationMode) {
        setState(ConnectionState::Connected);
        emit connected();
        return true;
    }

    // In production, this would initiate a TCP/WebSocket connection.
    // For now, simulate success.
    setState(ConnectionState::Connected);
    emit connected();
    return true;
}

void MobileClient::disconnectFromServer() {
    m_reconnectTimer.stop();
    m_reconnectAttempts = 0;

    if (m_state == ConnectionState::Disconnected)
        return;

    setState(ConnectionState::Disconnected);
    emit disconnected();
}

MobileClient::ConnectionState MobileClient::connectionState() const {
    return m_state;
}

QString MobileClient::lastError() const {
    return m_lastError;
}

QString MobileClient::serverHost() const {
    return m_host;
}

int MobileClient::serverPort() const {
    return m_port;
}

int MobileClient::reconnectAttempts() const {
    return m_reconnectAttempts;
}

void MobileClient::setMaxReconnectAttempts(int max) {
    m_maxReconnectAttempts = max;
}

int MobileClient::maxReconnectAttempts() const {
    return m_maxReconnectAttempts;
}

void MobileClient::setReconnectIntervalMs(int ms) {
    m_reconnectIntervalMs = ms;
}

int MobileClient::reconnectIntervalMs() const {
    return m_reconnectIntervalMs;
}

int MobileClient::sendCommand(const QString& command, const QVariantMap& params) {
    if (m_state != ConnectionState::Connected) {
        m_lastError = QStringLiteral("Not connected");
        emit errorOccurred(m_lastError);
        return -1;
    }

    if (command.isEmpty()) {
        m_lastError = QStringLiteral("Empty command");
        emit errorOccurred(m_lastError);
        return -1;
    }

    int id = m_nextRequestId++;

    if (m_simulationMode) {
        // In simulation, response is injected externally via injectSimResponse()
        Q_UNUSED(params);
        return id;
    }

    // In production, serialize and send over network
    Q_UNUSED(params);
    return id;
}

int MobileClient::requestStatus() {
    return sendCommand(QStringLiteral("status"));
}

void MobileClient::setSimulationMode(bool enabled) {
    m_simulationMode = enabled;
}

bool MobileClient::isSimulationMode() const {
    return m_simulationMode;
}

void MobileClient::injectSimResponse(int requestId, const QVariantMap& response) {
    if (!m_simulationMode)
        return;

    emit responseReceived(requestId, response);

    // If it contains "state" key, treat as status update
    if (response.contains(QStringLiteral("state"))) {
        emit statusUpdated(response);
    }
}

void MobileClient::simulateDisconnect() {
    if (!m_simulationMode || m_state == ConnectionState::Disconnected)
        return;

    m_lastError = QStringLiteral("Connection lost");
    setState(ConnectionState::Reconnecting);
    emit errorOccurred(m_lastError);
    m_reconnectAttempts = 0;
    m_reconnectTimer.start(m_reconnectIntervalMs);
}

void MobileClient::simulateReconnect() {
    if (!m_simulationMode)
        return;

    m_reconnectTimer.stop();
    m_reconnectAttempts = 0;
    setState(ConnectionState::Connected);
    emit connected();
}

void MobileClient::onReconnectTimer() {
    if (m_state != ConnectionState::Reconnecting)
        return;

    m_reconnectAttempts++;
    emit reconnectAttemptStarted(m_reconnectAttempts);

    if (m_simulationMode) {
        // In simulation, reconnect always fails until simulateReconnect() is called
        if (m_reconnectAttempts >= m_maxReconnectAttempts) {
            m_lastError = QStringLiteral("Max reconnect attempts exceeded");
            setState(ConnectionState::Error);
            emit errorOccurred(m_lastError);
            return;
        }
        // Schedule next attempt with exponential backoff
        int interval = m_reconnectIntervalMs * (1 << qMin(m_reconnectAttempts, 4));
        m_reconnectTimer.start(interval);
        return;
    }

    // In production, attempt real reconnection here
    if (m_reconnectAttempts >= m_maxReconnectAttempts) {
        m_lastError = QStringLiteral("Max reconnect attempts exceeded");
        setState(ConnectionState::Error);
        emit errorOccurred(m_lastError);
        return;
    }

    int interval = m_reconnectIntervalMs * (1 << qMin(m_reconnectAttempts, 4));
    m_reconnectTimer.start(interval);
}

void MobileClient::setState(ConnectionState state) {
    if (m_state == state)
        return;
    m_state = state;
    emit connectionStateChanged(state);
}
