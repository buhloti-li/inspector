#include "recoverymanager.h"
#include "../workcellcontroller.h"

RecoveryManager::RecoveryManager(QObject* parent)
    : QObject(parent) {
}

void RecoveryManager::registerStrategy(const RecoveryStrategy& strategy) {
    m_strategies.insert(strategy.errorCode, strategy);
}

void RecoveryManager::removeStrategy(const QString& errorCode) {
    m_strategies.remove(errorCode);
}

bool RecoveryManager::hasStrategy(const QString& errorCode) const {
    return m_strategies.contains(errorCode);
}

bool RecoveryManager::handleError(const QString& errorCode, WorkcellController* controller) {
    if (!controller)
        return false;

    auto it = m_strategies.find(errorCode);
    if (it == m_strategies.end()) {
        emit alertTriggered(errorCode, QStringLiteral("No recovery strategy for: ") + errorCode);
        return false;
    }

    const RecoveryStrategy& strategy = it.value();
    int& retryCount = m_retryCounts[errorCode];

    for (const auto& action : strategy.actionSequence) {
        switch (action) {
        case RecoveryStrategy::Retry:
            if (retryCount < strategy.maxRetries) {
                ++retryCount;
                emit recoveryAttempted(errorCode, actionName(action), true);
                return true; // Caller should retry the operation
            }
            emit recoveryAttempted(errorCode, actionName(action), false);
            break;

        case RecoveryStrategy::SkipObject:
            retryCount = 0;
            emit recoveryAttempted(errorCode, actionName(action), true);
            return true; // Caller should skip to next object

        case RecoveryStrategy::DegradeSpeed:
            emit recoveryAttempted(errorCode, actionName(action), true);
            return true;

        case RecoveryStrategy::Alert:
            emit alertTriggered(errorCode, QStringLiteral("Recovery alert: ") + errorCode);
            emit recoveryAttempted(errorCode, actionName(action), true);
            break; // Alert doesn't stop the sequence; continue to next action

        case RecoveryStrategy::EStop:
            controller->emergencyStop();
            retryCount = 0;
            emit recoveryAttempted(errorCode, actionName(action), true);
            return false; // E-stop means recovery failed, system halted
        }
    }

    // All actions exhausted without resolution
    retryCount = 0;
    return false;
}

void RecoveryManager::registerDefaults() {
    registerStrategy({
        QStringLiteral("GraspFailed"),
        {RecoveryStrategy::Retry, RecoveryStrategy::SkipObject, RecoveryStrategy::Alert},
        3
    });

    registerStrategy({
        QStringLiteral("CollisionDetected"),
        {RecoveryStrategy::DegradeSpeed, RecoveryStrategy::Retry, RecoveryStrategy::EStop},
        2
    });

    registerStrategy({
        QStringLiteral("VisionTimeout"),
        {RecoveryStrategy::Retry, RecoveryStrategy::Alert},
        2
    });

    registerStrategy({
        QStringLiteral("NoObject"),
        {RecoveryStrategy::Alert},
        1
    });

    registerStrategy({
        QStringLiteral("RobotCommLost"),
        {RecoveryStrategy::Retry, RecoveryStrategy::Alert, RecoveryStrategy::EStop},
        5
    });
}

QString RecoveryManager::actionName(RecoveryStrategy::Action action) {
    switch (action) {
    case RecoveryStrategy::Retry:         return QStringLiteral("Retry");
    case RecoveryStrategy::SkipObject:    return QStringLiteral("SkipObject");
    case RecoveryStrategy::DegradeSpeed:  return QStringLiteral("DegradeSpeed");
    case RecoveryStrategy::Alert:         return QStringLiteral("Alert");
    case RecoveryStrategy::EStop:         return QStringLiteral("EStop");
    }
    return QStringLiteral("Unknown");
}
