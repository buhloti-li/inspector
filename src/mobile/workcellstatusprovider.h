#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QTimer>

class MobileClient;

/// Provides real-time workcell status updates for mobile UI.
/// Polls the server at configurable intervals and emits signals on changes.
class WorkcellStatusProvider : public QObject {
    Q_OBJECT
public:
    enum class WorkcellState {
        Unknown,
        Idle,
        Running,
        Stopped,
        Fault,
        Degraded
    };
    Q_ENUM(WorkcellState)

    explicit WorkcellStatusProvider(MobileClient* client, QObject* parent = nullptr);

    // Current state
    WorkcellState workcellState() const;
    int cycleCount() const;
    int successCount() const;
    int failCount() const;
    double successRate() const;
    QString currentRecipe() const;
    bool isAutoMode() const;

    // Polling
    void startPolling(int intervalMs = 1000);
    void stopPolling();
    bool isPolling() const;
    int pollIntervalMs() const;

    // Manual update
    void requestUpdate();
    void updateFromStatus(const QVariantMap& status);

signals:
    void stateChanged(WorkcellStatusProvider::WorkcellState newState);
    void cycleCountChanged(int total, int success, int fail);
    void recipeChanged(const QString& recipe);
    void autoModeChanged(bool autoMode);
    void updateReceived();

private slots:
    void onPollTimer();
    void onStatusUpdated(const QVariantMap& status);

private:
    MobileClient* m_client;
    QTimer m_pollTimer;
    WorkcellState m_state = WorkcellState::Unknown;
    int m_cycleCount = 0;
    int m_successCount = 0;
    int m_failCount = 0;
    QString m_currentRecipe;
    bool m_autoMode = false;
};
