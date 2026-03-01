#include "simrobotdriver.h"

SimRobotDriver::SimRobotDriver(const QString& id)
    : m_id(id) {
}

bool SimRobotDriver::connect(const QVariantMap& config) {
    Q_UNUSED(config);
    if (m_status != DeviceStatus::Disconnected)
        return false;
    // Initialize to home position
    m_currentPose = {0, 0, 500, 0, 0, 0};
    m_currentJoints = {0, 0, 0, 0, 0, 0};
    m_status = DeviceStatus::Ready;
    return true;
}

void SimRobotDriver::disconnect() {
    m_status = DeviceStatus::Disconnected;
    m_inMotion = false;
}

IDeviceDriver::DeviceStatus SimRobotDriver::status() const {
    return m_status;
}

QString SimRobotDriver::deviceId() const {
    return m_id;
}

bool SimRobotDriver::moveTo(const CartesianPose& target, double speed) {
    Q_UNUSED(speed);
    if (m_status != DeviceStatus::Ready)
        return false;

    if (m_failNextMove) {
        m_failNextMove = false;
        return false;
    }

    // Instantly move to target (simulation)
    m_currentPose = target;
    return true;
}

bool SimRobotDriver::moveJoint(const JointPosition& target, double speed) {
    Q_UNUSED(speed);
    if (m_status != DeviceStatus::Ready)
        return false;

    if (m_failNextMove) {
        m_failNextMove = false;
        return false;
    }

    m_currentJoints = target;
    return true;
}

CartesianPose SimRobotDriver::currentPose() const {
    return m_currentPose;
}

JointPosition SimRobotDriver::currentJoints() const {
    return m_currentJoints;
}

bool SimRobotDriver::isInMotion() const {
    return m_inMotion;
}

bool SimRobotDriver::stopMotion() {
    m_inMotion = false;
    return true;
}

void SimRobotDriver::setDigitalOutput(int pin, bool value) {
    m_digitalOutputs[pin] = value;
}

bool SimRobotDriver::digitalOutput(int pin) const {
    return m_digitalOutputs.value(pin, false);
}

void SimRobotDriver::setFailNextMove(bool fail) {
    m_failNextMove = fail;
}
