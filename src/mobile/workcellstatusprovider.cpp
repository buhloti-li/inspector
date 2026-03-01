#include "workcellstatusprovider.h"
#include "mobileclient.h"

WorkcellStatusProvider::WorkcellStatusProvider(MobileClient* client, QObject* parent)
    : QObject(parent)
    , m_client(client) {
    QObject::connect(m_client, &MobileClient::statusUpdated,
                     this, &WorkcellStatusProvider::onStatusUpdated);
    QObject::connect(&m_pollTimer, &QTimer::timeout,
                     this, &WorkcellStatusProvider::onPollTimer);
}

WorkcellStatusProvider::WorkcellState WorkcellStatusProvider::workcellState() const {
    return m_state;
}

int WorkcellStatusProvider::cycleCount() const {
    return m_cycleCount;
}

int WorkcellStatusProvider::successCount() const {
    return m_successCount;
}

int WorkcellStatusProvider::failCount() const {
    return m_failCount;
}

double WorkcellStatusProvider::successRate() const {
    if (m_cycleCount == 0) return 0.0;
    return static_cast<double>(m_successCount) / m_cycleCount;
}

QString WorkcellStatusProvider::currentRecipe() const {
    return m_currentRecipe;
}

bool WorkcellStatusProvider::isAutoMode() const {
    return m_autoMode;
}

void WorkcellStatusProvider::startPolling(int intervalMs) {
    m_pollTimer.start(intervalMs);
}

void WorkcellStatusProvider::stopPolling() {
    m_pollTimer.stop();
}

bool WorkcellStatusProvider::isPolling() const {
    return m_pollTimer.isActive();
}

int WorkcellStatusProvider::pollIntervalMs() const {
    return m_pollTimer.interval();
}

void WorkcellStatusProvider::requestUpdate() {
    if (m_client)
        m_client->requestStatus();
}

void WorkcellStatusProvider::updateFromStatus(const QVariantMap& status) {
    onStatusUpdated(status);
}

void WorkcellStatusProvider::onPollTimer() {
    requestUpdate();
}

void WorkcellStatusProvider::onStatusUpdated(const QVariantMap& status) {
    // Parse state
    QString stateStr = status.value(QStringLiteral("state")).toString();
    WorkcellState newState = WorkcellState::Unknown;
    if (stateStr == QStringLiteral("Idle")) newState = WorkcellState::Idle;
    else if (stateStr == QStringLiteral("Running")) newState = WorkcellState::Running;
    else if (stateStr == QStringLiteral("Stopped")) newState = WorkcellState::Stopped;
    else if (stateStr == QStringLiteral("Fault")) newState = WorkcellState::Fault;
    else if (stateStr == QStringLiteral("Degraded")) newState = WorkcellState::Degraded;

    if (newState != m_state) {
        m_state = newState;
        emit stateChanged(m_state);
    }

    // Parse cycle counts
    int newCycleCount = status.value(QStringLiteral("cycleCount"), m_cycleCount).toInt();
    int newSuccess = status.value(QStringLiteral("successCount"), m_successCount).toInt();
    int newFail = status.value(QStringLiteral("failCount"), m_failCount).toInt();

    if (newCycleCount != m_cycleCount || newSuccess != m_successCount || newFail != m_failCount) {
        m_cycleCount = newCycleCount;
        m_successCount = newSuccess;
        m_failCount = newFail;
        emit cycleCountChanged(m_cycleCount, m_successCount, m_failCount);
    }

    // Parse recipe
    QString recipe = status.value(QStringLiteral("recipe"), m_currentRecipe).toString();
    if (recipe != m_currentRecipe) {
        m_currentRecipe = recipe;
        emit recipeChanged(m_currentRecipe);
    }

    // Parse auto mode
    bool autoMode = status.value(QStringLiteral("autoMode"), m_autoMode).toBool();
    if (autoMode != m_autoMode) {
        m_autoMode = autoMode;
        emit autoModeChanged(m_autoMode);
    }

    emit updateReceived();
}
