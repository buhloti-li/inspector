#include "remotecommandservice.h"
#include "mobileclient.h"

RemoteCommandService::RemoteCommandService(MobileClient* client, QObject* parent)
    : QObject(parent)
    , m_client(client) {
    QObject::connect(m_client, &MobileClient::responseReceived,
                     this, &RemoteCommandService::onResponseReceived);
}

int RemoteCommandService::startCycle() {
    return sendCommand(QStringLiteral("startCycle"));
}

int RemoteCommandService::stopCycle() {
    return sendCommand(QStringLiteral("stopCycle"));
}

int RemoteCommandService::emergencyStop() {
    return sendCommand(QStringLiteral("emergencyStop"));
}

int RemoteCommandService::loadRecipe(const QString& recipeName) {
    QVariantMap params;
    params[QStringLiteral("recipe")] = recipeName;
    return sendCommand(QStringLiteral("loadRecipe"), params);
}

int RemoteCommandService::setAutoMode(bool enabled) {
    QVariantMap params;
    params[QStringLiteral("enabled")] = enabled;
    return sendCommand(QStringLiteral("setAutoMode"), params);
}

int RemoteCommandService::setSpeed(double speedPercent) {
    QVariantMap params;
    params[QStringLiteral("speed")] = speedPercent;
    return sendCommand(QStringLiteral("setSpeed"), params);
}

int RemoteCommandService::sendCommand(const QString& command, const QVariantMap& params) {
    int id = m_client->sendCommand(command, params);
    if (id < 0)
        return id;

    CommandRecord record;
    record.requestId = id;
    record.command = command;
    record.params = params;
    record.status = CommandStatus::Pending;
    m_commands[id] = record;

    emit commandSent(id, command);
    return id;
}

RemoteCommandService::CommandRecord RemoteCommandService::commandRecord(int requestId) const {
    return m_commands.value(requestId);
}

int RemoteCommandService::pendingCommandCount() const {
    int count = 0;
    for (auto it = m_commands.constBegin(); it != m_commands.constEnd(); ++it) {
        if (it.value().status == CommandStatus::Pending)
            ++count;
    }
    return count;
}

QList<int> RemoteCommandService::pendingCommandIds() const {
    QList<int> ids;
    for (auto it = m_commands.constBegin(); it != m_commands.constEnd(); ++it) {
        if (it.value().status == CommandStatus::Pending)
            ids.append(it.key());
    }
    return ids;
}

void RemoteCommandService::setCommandTimeoutMs(int ms) {
    m_commandTimeoutMs = ms;
}

int RemoteCommandService::commandTimeoutMs() const {
    return m_commandTimeoutMs;
}

void RemoteCommandService::processResponse(int requestId, const QVariantMap& response) {
    onResponseReceived(requestId, response);
}

void RemoteCommandService::onResponseReceived(int requestId, const QVariantMap& data) {
    if (!m_commands.contains(requestId))
        return;

    auto& record = m_commands[requestId];
    QString status = data.value(QStringLiteral("status")).toString();

    if (status == QStringLiteral("ok") || status == QStringLiteral("ack")) {
        record.status = CommandStatus::Acknowledged;
        record.message = data.value(QStringLiteral("message")).toString();
        emit commandAcknowledged(requestId, record.message);
    } else if (status == QStringLiteral("rejected") || status == QStringLiteral("error")) {
        record.status = CommandStatus::Rejected;
        record.message = data.value(QStringLiteral("reason"),
                                     data.value(QStringLiteral("message"))).toString();
        emit commandRejected(requestId, record.message);
    } else if (status == QStringLiteral("timeout")) {
        record.status = CommandStatus::Timeout;
        emit commandTimeout(requestId);
    } else {
        record.status = CommandStatus::Error;
        record.message = QStringLiteral("Unknown response status: ") + status;
        emit commandError(requestId, record.message);
    }
}
