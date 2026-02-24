#include "workcellcontroller.h"

WorkcellController::WorkcellController(int maxRetry)
    : m_maxRetry(maxRetry),
      m_consecutiveFailures(0),
      m_state(State::Idle) {
}

bool WorkcellController::startTask(QString* error) {
    if (m_state == State::Running) {
        if (error) {
            *error = "AlreadyRunning";
        }
        return false;
    }

    if (m_state == State::Fault) {
        if (error) {
            *error = "CannotStartInFault";
        }
        return false;
    }

    m_state = State::Running;
    return true;
}

bool WorkcellController::stopTask(QString* error) {
    if (m_state != State::Running && m_state != State::Degraded) {
        if (error) {
            *error = "NotRunning";
        }
        return false;
    }

    m_state = State::Stopped;
    return true;
}

void WorkcellController::recordPickResult(bool success) {
    if (m_state != State::Running && m_state != State::Degraded) {
        return;
    }

    ++m_stats.totalAttempts;
    if (success) {
        ++m_stats.successCount;
        m_consecutiveFailures = 0;
        if (m_state == State::Degraded) {
            m_state = State::Running;
        }
    } else {
        ++m_stats.failureCount;
        ++m_consecutiveFailures;
        if (m_consecutiveFailures >= m_maxRetry) {
            m_state = State::Degraded;
        }
    }
}

void WorkcellController::emergencyStop() {
    m_state = State::Fault;
}

bool WorkcellController::recoverFromFault() {
    if (m_state != State::Fault) {
        return false;
    }
    m_state = State::Idle;
    m_consecutiveFailures = 0;
    return true;
}

void WorkcellController::resetSession() {
    m_stats = Stats{};
    m_consecutiveFailures = 0;
    if (m_state == State::Stopped) {
        m_state = State::Idle;
    }
}

WorkcellController::State WorkcellController::state() const {
    return m_state;
}

int WorkcellController::consecutiveFailures() const {
    return m_consecutiveFailures;
}

WorkcellController::Stats WorkcellController::stats() const {
    return m_stats;
}
