#ifndef TRAJECTORYPLANNER_H
#define TRAJECTORYPLANNER_H

#include <QVector>
#include "../device/irobotdriver.h"
#include "graspplanner.h"

struct Waypoint {
    CartesianPose pose;
    double speed = 200.0;       // mm/s
    double blendRadius = 5.0;   // mm, smooth transition radius
};

struct TrajectoryPlan {
    QVector<Waypoint> waypoints;
    double estimatedDuration = 0.0; // seconds
    bool valid = false;
};

class TrajectoryPlanner {
public:
    struct Config {
        double maxSpeed = 500.0;         // mm/s
        double maxAcceleration = 1000.0; // mm/s^2
        double safeHeight = 200.0;       // mm, safe transit height (z)
        double approachSpeed = 100.0;    // mm/s, slow speed near object
        double placeSpeed = 150.0;       // mm/s
    };

    void setConfig(const Config& config);
    Config config() const;

    // Plan a complete pick-and-place trajectory
    TrajectoryPlan planPickPlace(const CartesianPose& currentPose,
                                 const GraspCandidate& grasp,
                                 const CartesianPose& placeTarget);

    // Execute a trajectory plan on a robot driver
    static bool execute(IRobotDriver* robot, const TrajectoryPlan& plan);

private:
    static double poseDistance(const CartesianPose& a, const CartesianPose& b);

    Config m_config;
};

#endif // TRAJECTORYPLANNER_H
