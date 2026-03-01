#include "alertnotificationservice.h"
#include "mobileclient.h"

AlertNotificationService::AlertNotificationService(MobileClient* client, QObject* parent)
    : QObject(parent)
    , m_client(client) {
    QObject::connect(m_client, &MobileClient::responseReceived,
                     this, &AlertNotificationService::onResponseReceived);
}

QVector<AlertNotificationService::Alert> AlertNotificationService::allAlerts() const {
    return m_alerts;
}

QVector<AlertNotificationService::Alert> AlertNotificationService::unacknowledgedAlerts() const {
    QVector<Alert> result;
    for (const auto& a : m_alerts) {
        if (!a.acknowledged)
            result.append(a);
    }
    return result;
}

QVector<AlertNotificationService::Alert> AlertNotificationService::alertsBySeverity(Severity severity) const {
    QVector<Alert> result;
    for (const auto& a : m_alerts) {
        if (a.severity == severity)
            result.append(a);
    }
    return result;
}

AlertNotificationService::Alert AlertNotificationService::alertById(int id) const {
    for (const auto& a : m_alerts) {
        if (a.id == id)
            return a;
    }
    return Alert{};
}

int AlertNotificationService::alertCount() const {
    return m_alerts.size();
}

int AlertNotificationService::unacknowledgedCount() const {
    int count = 0;
    for (const auto& a : m_alerts) {
        if (!a.acknowledged)
            ++count;
    }
    return count;
}

void AlertNotificationService::acknowledgeAlert(int alertId) {
    for (auto& a : m_alerts) {
        if (a.id == alertId && !a.acknowledged) {
            a.acknowledged = true;
            emit alertAcknowledged(alertId);
            return;
        }
    }
}

void AlertNotificationService::acknowledgeAll() {
    bool any = false;
    for (auto& a : m_alerts) {
        if (!a.acknowledged) {
            a.acknowledged = true;
            any = true;
        }
    }
    if (any)
        emit allAlertsAcknowledged();
}

void AlertNotificationService::clearHistory() {
    m_alerts.clear();
    emit alertsCleared();
}

void AlertNotificationService::setMaxAlertHistory(int max) {
    m_maxHistory = max;
    // Trim if needed
    while (m_alerts.size() > m_maxHistory) {
        m_alerts.removeFirst();
    }
}

int AlertNotificationService::maxAlertHistory() const {
    return m_maxHistory;
}

void AlertNotificationService::addAlert(Severity severity, const QString& source,
                                         const QString& message) {
    Alert alert;
    alert.id = m_nextAlertId++;
    alert.severity = severity;
    alert.source = source;
    alert.message = message;
    alert.timestamp = QDateTime::currentDateTime();
    alert.acknowledged = false;

    m_alerts.append(alert);

    // Trim oldest if over limit
    while (m_alerts.size() > m_maxHistory) {
        m_alerts.removeFirst();
    }

    emit alertReceived(alert.id, severity, message);
}

void AlertNotificationService::processServerAlert(const QVariantMap& alertData) {
    QString severityStr = alertData.value(QStringLiteral("severity")).toString();
    Severity sev = Severity::Info;
    if (severityStr == QStringLiteral("warning")) sev = Severity::Warning;
    else if (severityStr == QStringLiteral("error")) sev = Severity::Error;
    else if (severityStr == QStringLiteral("critical")) sev = Severity::Critical;

    QString source = alertData.value(QStringLiteral("source")).toString();
    QString message = alertData.value(QStringLiteral("message")).toString();

    addAlert(sev, source, message);
}

void AlertNotificationService::onResponseReceived(int requestId, const QVariantMap& data) {
    Q_UNUSED(requestId);

    // Check if this response contains an alert
    if (data.contains(QStringLiteral("alertType")) || data.contains(QStringLiteral("severity"))) {
        processServerAlert(data);
    }
}
