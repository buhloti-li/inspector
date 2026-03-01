#ifndef SIMROBOTDRIVER_H
#define SIMROBOTDRIVER_H

#include "../irobotdriver.h"
#include <QMap>

class SimRobotDriver : public IRobotDriver {
public:
    explicit SimRobotDriver(const QString& id = QStringLiteral("sim-robot-01"));

    // IDeviceDriver
    bool connect(const QVariantMap& config) override;
    void disconnect() override;
    DeviceStatus status() const override;
    QString deviceId() const override;

    // IRobotDriver
    bool moveTo(const CartesianPose& target, double speed) override;
    bool moveJoint(const JointPosition& target, double speed) override;
    CartesianPose currentPose() const override;
    JointPosition currentJoints() const override;
    bool isInMotion() const override;
    bool stopMotion() override;
    void setDigitalOutput(int pin, bool value) override;

    // Simulation controls
    bool digitalOutput(int pin) const;
    void setFailNextMove(bool fail);

private:
    QString m_id;
    DeviceStatus m_status = DeviceStatus::Disconnected;
    CartesianPose m_currentPose;
    JointPosition m_currentJoints;
    bool m_inMotion = false;
    bool m_failNextMove = false;
    QMap<int, bool> m_digitalOutputs;
};

#endif // SIMROBOTDRIVER_H
