#include "workcellcontroller.h"
#include "device/devicemanager.h"
#include "device/icameradriver.h"
#include "device/irobotdriver.h"
#include "vision/visionpipeline.h"
#include "planning/graspplanner.h"
#include "planning/trajectoryplanner.h"

#include <QElapsedTimer>

WorkcellController::WorkcellController(int maxRetry, QObject* parent)
    : QObject(parent),
      m_maxRetry(maxRetry),
      m_consecutiveFailures(0),
      m_state(State::Idle) {
}

bool WorkcellController::startTask(QString* error) {
    if (m_state == State::Running) {
        if (error)
            *error = QStringLiteral("AlreadyRunning");
        return false;
    }

    if (m_state == State::Fault) {
        if (error)
            *error = QStringLiteral("CannotStartInFault");
        return false;
    }

    setState(State::Running);
    return true;
}

bool WorkcellController::stopTask(QString* error) {
    if (m_state != State::Running && m_state != State::Degraded) {
        if (error)
            *error = QStringLiteral("NotRunning");
        return false;
    }

    m_autoMode = false;
    setState(State::Stopped);
    return true;
}

void WorkcellController::recordPickResult(bool success) {
    if (m_state != State::Running && m_state != State::Degraded)
        return;

    ++m_stats.totalAttempts;
    if (success) {
        ++m_stats.successCount;
        m_consecutiveFailures = 0;
        if (m_state == State::Degraded) {
            setState(State::Running);
        }
    } else {
        ++m_stats.failureCount;
        ++m_consecutiveFailures;
        if (m_consecutiveFailures >= m_maxRetry) {
            setState(State::Degraded);
        }
    }
}

void WorkcellController::emergencyStop() {
    m_autoMode = false;
    setState(State::Fault);
}

bool WorkcellController::recoverFromFault() {
    if (m_state != State::Fault)
        return false;
    m_consecutiveFailures = 0;
    setState(State::Idle);
    return true;
}

void WorkcellController::resetSession() {
    m_stats = Stats{};
    m_consecutiveFailures = 0;
    m_cycleIndex = 0;
    if (m_state == State::Stopped) {
        setState(State::Idle);
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

// === Device binding ===

void WorkcellController::setDeviceManager(DeviceManager* dm) {
    m_deviceManager = dm;
}

void WorkcellController::bindCamera(const QString& cameraId) {
    m_cameraId = cameraId;
}

void WorkcellController::bindRobot(const QString& robotId) {
    m_robotId = robotId;
}

QString WorkcellController::boundCameraId() const {
    return m_cameraId;
}

QString WorkcellController::boundRobotId() const {
    return m_robotId;
}

// === Recipe ===

void WorkcellController::loadRecipe(const QVariantMap& recipe) {
    m_recipe = recipe;
}

QVariantMap WorkcellController::recipe() const {
    return m_recipe;
}

// === Auto-cycle mode ===

bool WorkcellController::startAutoMode(QString* error) {
    if (m_state != State::Running && m_state != State::Degraded) {
        if (!startTask(error))
            return false;
    }

    if (!m_deviceManager) {
        if (error)
            *error = QStringLiteral("NoDeviceManager");
        return false;
    }

    if (m_cameraId.isEmpty() || m_robotId.isEmpty()) {
        if (error)
            *error = QStringLiteral("DevicesNotBound");
        return false;
    }

    m_autoMode = true;
    return true;
}

void WorkcellController::stopAutoMode() {
    m_autoMode = false;
}

bool WorkcellController::isAutoMode() const {
    return m_autoMode;
}

// === Internal ===

void WorkcellController::setState(State newState) {
    if (m_state == newState)
        return;
    State old = m_state;
    m_state = newState;
    emit stateChanged(old, newState);
}

void WorkcellController::executeSingleCycle() {
    if (!m_deviceManager || !m_autoMode)
        return;

    ICameraDriver* cam = m_deviceManager->camera(m_cameraId);
    IRobotDriver* robot = m_deviceManager->robot(m_robotId);
    if (!cam || !robot) {
        emit alertRaised(QStringLiteral("DeviceUnavailable"),
                         QStringLiteral("Bound devices not found in DeviceManager"));
        return;
    }

    QElapsedTimer timer;
    timer.start();

    // Step 1: Vision
    VisionPipeline vision;
    vision.setCameraDriver(cam);
    float minConf = m_recipe.value(QStringLiteral("minConfidence"), 0.5).toFloat();
    vision.setMinConfidence(minConf);

    auto detections = vision.execute();
    if (detections.isEmpty()) {
        emit alertRaised(QStringLiteral("NoObject"),
                         QStringLiteral("No objects detected in scene"));
        recordPickResult(false);
        emit pickAttemptFinished(false, QStringLiteral("NoObject"));
        return;
    }

    // Step 2: Grasp planning
    GraspPlanner graspPlanner;
    auto candidates = graspPlanner.plan(detections.first());
    if (candidates.isEmpty()) {
        recordPickResult(false);
        emit pickAttemptFinished(false, QStringLiteral("NoGraspCandidate"));
        return;
    }

    // Step 3: Trajectory planning
    TrajectoryPlanner trajPlanner;
    CartesianPose placeTarget;
    placeTarget.x = m_recipe.value(QStringLiteral("placeX"), 300.0).toDouble();
    placeTarget.y = m_recipe.value(QStringLiteral("placeY"), 0.0).toDouble();
    placeTarget.z = m_recipe.value(QStringLiteral("placeZ"), 400.0).toDouble();

    auto plan = trajPlanner.planPickPlace(robot->currentPose(), candidates.first(), placeTarget);
    if (!plan.valid) {
        recordPickResult(false);
        emit pickAttemptFinished(false, QStringLiteral("PlanningFailed"));
        return;
    }

    // Step 4: Execute
    bool moveOk = TrajectoryPlanner::execute(robot, plan);
    recordPickResult(moveOk);
    emit pickAttemptFinished(moveOk, moveOk ? QString() : QStringLiteral("ExecutionFailed"));

    ++m_cycleIndex;
    emit cycleCompleted(m_cycleIndex, timer.elapsed());
}
