#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QTimer>

/// Network communication client for mobile app.
/// Connects to the industrial workcell server via TCP/WebSocket.
/// In simulation mode, works in-process without actual network.
class MobileClient : public QObject {
    Q_OBJECT
public:
    enum class ConnectionState {
        Disconnected,
        Connecting,
        Connected,
        Reconnecting,
        Error
    };
    Q_ENUM(ConnectionState)

    explicit MobileClient(QObject* parent = nullptr);

    // Connection management
    bool connectToServer(const QString& host, int port);
    void disconnectFromServer();
    ConnectionState connectionState() const;
    QString lastError() const;

    // Server info
    QString serverHost() const;
    int serverPort() const;
    int reconnectAttempts() const;
    void setMaxReconnectAttempts(int max);
    int maxReconnectAttempts() const;
    void setReconnectIntervalMs(int ms);
    int reconnectIntervalMs() const;

    // Commands (returns request ID, -1 on failure)
    int sendCommand(const QString& command, const QVariantMap& params = {});
    int requestStatus();

    // Simulation mode (for testing without server)
    void setSimulationMode(bool enabled);
    bool isSimulationMode() const;
    void injectSimResponse(int requestId, const QVariantMap& response);
    void simulateDisconnect();
    void simulateReconnect();

signals:
    void connectionStateChanged(MobileClient::ConnectionState state);
    void connected();
    void disconnected();
    void errorOccurred(const QString& error);
    void responseReceived(int requestId, const QVariantMap& data);
    void statusUpdated(const QVariantMap& status);
    void reconnectAttemptStarted(int attempt);

private slots:
    void onReconnectTimer();

private:
    void setState(ConnectionState state);

    ConnectionState m_state = ConnectionState::Disconnected;
    QString m_host;
    int m_port = 0;
    int m_nextRequestId = 1;
    int m_reconnectAttempts = 0;
    int m_maxReconnectAttempts = 5;
    int m_reconnectIntervalMs = 2000;
    bool m_simulationMode = false;
    QString m_lastError;
    QTimer m_reconnectTimer;
};
