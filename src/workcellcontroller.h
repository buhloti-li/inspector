#ifndef WORKCELLCONTROLLER_H
#define WORKCELLCONTROLLER_H

#include <QString>

class WorkcellController {
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

    explicit WorkcellController(int maxRetry = 3);

    bool startTask(QString* error = nullptr);
    bool stopTask(QString* error = nullptr);

    void recordPickResult(bool success);
    void emergencyStop();
    bool recoverFromFault();

    void resetSession();

    State state() const;
    int consecutiveFailures() const;
    Stats stats() const;

private:
    int m_maxRetry;
    int m_consecutiveFailures;
    State m_state;
    Stats m_stats;
};

#endif
