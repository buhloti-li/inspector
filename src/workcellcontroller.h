#ifndef WORKCELLCONTROLLER_H
#define WORKCELLCONTROLLER_H

#include <QObject>
#include <QString>
#include <QVariantMap>

class DeviceManager;
class ICameraDriver;
class IRobotDriver;

class WorkcellController : public QObject {
    Q_OBJECT
public:
    enum class State {
        Idle,
        Running,
        Stopped,
        Fault,
        Degraded
    };

    struct Stats {
        int totalAttempts = 0;
        int successCount = 0;
        int failureCount = 0;

        double successRate() const {
            return totalAttempts == 0 ? 0.0 : static_cast<double>(successCount) / totalAttempts;
        }
    };

    explicit WorkcellController(int maxRetry = 3, QObject* parent = nullptr);

    // === Original API (preserved) ===
    bool startTask(QString* error = nullptr);
    bool stopTask(QString* error = nullptr);

    void recordPickResult(bool success);
    void emergencyStop();
    bool recoverFromFault();

    void resetSession();

    State state() const;
    int consecutiveFailures() const;
    Stats stats() const;

    // === Device binding ===
    void setDeviceManager(DeviceManager* dm);
    void bindCamera(const QString& cameraId);
    void bindRobot(const QString& robotId);

    QString boundCameraId() const;
    QString boundRobotId() const;

    // === Recipe / process configuration ===
    void loadRecipe(const QVariantMap& recipe);
    QVariantMap recipe() const;

    // === Auto-cycle mode ===
    bool startAutoMode(QString* error = nullptr);
    void stopAutoMode();
    bool isAutoMode() const;

signals:
    void stateChanged(WorkcellController::State oldState, WorkcellController::State newState);
    void pickAttemptFinished(bool success, const QString& reason);
    void cycleCompleted(int cycleIndex, double cycleDurationMs);
    void alertRaised(const QString& alertCode, const QString& message);

private:
    void setState(State newState);
    void executeSingleCycle();

    int m_maxRetry;
    int m_consecutiveFailures;
    State m_state;
    Stats m_stats;

    // Device references
    DeviceManager* m_deviceManager = nullptr;
    QString m_cameraId;
    QString m_robotId;

    // Recipe
    QVariantMap m_recipe;

    // Auto-cycle
    bool m_autoMode = false;
    int m_cycleIndex = 0;
};

#endif
