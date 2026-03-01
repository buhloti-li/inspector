#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QDateTime>
#include <QVariantMap>

class MobileClient;

/// Manages alert notifications from the workcell for mobile display.
/// Supports severity levels, acknowledgment, and alert history.
class AlertNotificationService : public QObject {
    Q_OBJECT
public:
    enum class Severity {
        Info,
        Warning,
        Error,
        Critical
    };
    Q_ENUM(Severity)

    struct Alert {
        int id = 0;
        Severity severity = Severity::Info;
        QString source;
        QString message;
        QDateTime timestamp;
        bool acknowledged = false;
    };

    explicit AlertNotificationService(MobileClient* client, QObject* parent = nullptr);

    // Alert access
    QVector<Alert> allAlerts() const;
    QVector<Alert> unacknowledgedAlerts() const;
    QVector<Alert> alertsBySeverity(Severity severity) const;
    Alert alertById(int id) const;
    int alertCount() const;
    int unacknowledgedCount() const;

    // Alert management
    void acknowledgeAlert(int alertId);
    void acknowledgeAll();
    void clearHistory();

    // Max history
    void setMaxAlertHistory(int max);
    int maxAlertHistory() const;

    // Add alert (from server or for testing)
    void addAlert(Severity severity, const QString& source, const QString& message);
    void processServerAlert(const QVariantMap& alertData);

signals:
    void alertReceived(int alertId, AlertNotificationService::Severity severity,
                       const QString& message);
    void alertAcknowledged(int alertId);
    void allAlertsAcknowledged();
    void alertsCleared();

private slots:
    void onResponseReceived(int requestId, const QVariantMap& data);

private:
    MobileClient* m_client;
    QVector<Alert> m_alerts;
    int m_nextAlertId = 1;
    int m_maxHistory = 100;
};
