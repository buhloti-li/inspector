#pragma once

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVector>
#include <QMap>
#include <QTimer>

class WorkcellController;

/// A single connected mobile client session.
class ClientSession : public QObject {
    Q_OBJECT
public:
    explicit ClientSession(QTcpSocket* socket, QObject* parent = nullptr);

    void sendJson(const QJsonObject& obj);
    void sendError(const QString& error);
    void sendStatusUpdate(const QJsonObject& status);
    QString peerAddress() const;
    bool isConnected() const;

signals:
    void commandReceived(const QString& command, const QJsonObject& params, int requestId);
    void disconnected();

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    void processMessage(const QByteArray& message);
    QTcpSocket* m_socket;
    QByteArray m_buffer;
};

/// TCP server exposing the WorkcellController to mobile clients.
/// Protocol: newline-delimited JSON over TCP.
///
/// Request format:  {"cmd":"startCycle","params":{},"id":1}
/// Response format: {"id":1,"status":"ok","data":{...}}
/// Push format:     {"event":"statusUpdate","data":{...}}
class WorkcellServer : public QObject {
    Q_OBJECT
public:
    explicit WorkcellServer(QObject* parent = nullptr);

    bool start(quint16 port = 9600);
    void stop();
    bool isListening() const;
    quint16 serverPort() const;
    int clientCount() const;

    void setController(WorkcellController* controller);
    void setStatusBroadcastInterval(int ms);

signals:
    void clientConnected(const QString& address);
    void clientDisconnected(const QString& address);
    void commandProcessed(const QString& command, int requestId);
    void serverError(const QString& error);
    void started(quint16 port);
    void stopped();

private slots:
    void onNewConnection();
    void onClientCommand(const QString& command, const QJsonObject& params, int requestId);
    void onClientDisconnected();
    void broadcastStatus();

private:
    QJsonObject buildStatusSnapshot() const;
    void handleCommand(ClientSession* session, const QString& cmd,
                       const QJsonObject& params, int requestId);

    QTcpServer* m_server = nullptr;
    QVector<ClientSession*> m_clients;
    WorkcellController* m_controller = nullptr;
    QTimer m_broadcastTimer;
};
