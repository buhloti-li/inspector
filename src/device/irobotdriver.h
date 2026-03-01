#ifndef IROBOTDRIVER_H
#define IROBOTDRIVER_H

#include "idevicedriver.h"
#include <array>

struct JointPosition {
    std::array<double, 6> joints = {};  // 6-axis joint angles (rad)
};

struct CartesianPose {
    double x = 0, y = 0, z = 0;     // mm
    double rx = 0, ry = 0, rz = 0;  // rad (RPY)
};

class IRobotDriver : public IDeviceDriver {
public:
    QString deviceType() const override { return QStringLiteral("Robot"); }

    virtual bool moveTo(const CartesianPose& target, double speed) = 0;
    virtual bool moveJoint(const JointPosition& target, double speed) = 0;
    virtual CartesianPose currentPose() const = 0;
    virtual JointPosition currentJoints() const = 0;
    virtual bool isInMotion() const = 0;
    virtual bool stopMotion() = 0;
    virtual void setDigitalOutput(int pin, bool value) = 0;
};

#endif // IROBOTDRIVER_H
