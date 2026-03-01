#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QMap>

class MobileClient;

/// Service for sending remote commands to the workcell controller.
/// Tracks command acknowledgment and provides timeout handling.
class RemoteCommandService : public QObject {
    Q_OBJECT
public:
    enum class CommandStatus {
        Pending,
        Acknowledged,
        Rejected,
        Timeout,
        Error
    };
    Q_ENUM(CommandStatus)

    struct CommandRecord {
        int requestId = -1;
        QString command;
        QVariantMap params;
        CommandStatus status = CommandStatus::Pending;
        QString message;
    };

    explicit RemoteCommandService(MobileClient* client, QObject* parent = nullptr);

    // Standard commands
    int startCycle();
    int stopCycle();
    int emergencyStop();
    int loadRecipe(const QString& recipeName);
    int setAutoMode(bool enabled);
    int setSpeed(double speedPercent);

    // Generic command
    int sendCommand(const QString& command, const QVariantMap& params = {});

    // Command tracking
    CommandRecord commandRecord(int requestId) const;
    int pendingCommandCount() const;
    QList<int> pendingCommandIds() const;

    // Timeout
    void setCommandTimeoutMs(int ms);
    int commandTimeoutMs() const;

    // Process response from server
    void processResponse(int requestId, const QVariantMap& response);

signals:
    void commandSent(int requestId, const QString& command);
    void commandAcknowledged(int requestId, const QString& message);
    void commandRejected(int requestId, const QString& reason);
    void commandTimeout(int requestId);
    void commandError(int requestId, const QString& error);

private slots:
    void onResponseReceived(int requestId, const QVariantMap& data);

private:
    void checkTimeouts();

    MobileClient* m_client;
    QMap<int, CommandRecord> m_commands;
    int m_commandTimeoutMs = 5000;
};
