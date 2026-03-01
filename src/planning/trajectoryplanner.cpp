#include "trajectoryplanner.h"
#include <cmath>

void TrajectoryPlanner::setConfig(const Config& config) {
    m_config = config;
}

TrajectoryPlanner::Config TrajectoryPlanner::config() const {
    return m_config;
}

TrajectoryPlan TrajectoryPlanner::planPickPlace(const CartesianPose& currentPose,
                                                 const GraspCandidate& grasp,
                                                 const CartesianPose& placeTarget) {
    TrajectoryPlan plan;

    // Waypoint 1: Move to safe height above current position
    CartesianPose safeStart = currentPose;
    safeStart.z = m_config.safeHeight;
    plan.waypoints.append({safeStart, m_config.maxSpeed, 10.0});

    // Waypoint 2: Move to above approach position at safe height
    CartesianPose aboveApproach = grasp.approachPose;
    aboveApproach.z = m_config.safeHeight;
    plan.waypoints.append({aboveApproach, m_config.maxSpeed, 10.0});

    // Waypoint 3: Descend to approach position
    plan.waypoints.append({grasp.approachPose, m_config.approachSpeed, 5.0});

    // Waypoint 4: Move to grasp position (slow, precise)
    plan.waypoints.append({grasp.graspPose, m_config.approachSpeed * 0.5, 0.0});

    // Waypoint 5: Close gripper (pause point, speed=0 indicates action point)
    // Represented as same pose but flagged via blendRadius=0
    plan.waypoints.append({grasp.graspPose, 0.0, 0.0});

    // Waypoint 6: Retreat
    plan.waypoints.append({grasp.retreatPose, m_config.approachSpeed, 5.0});

    // Waypoint 7: Safe transit to above place target
    CartesianPose abovePlace = placeTarget;
    abovePlace.z = m_config.safeHeight;
    plan.waypoints.append({abovePlace, m_config.maxSpeed, 10.0});

    // Waypoint 8: Descend to place target
    plan.waypoints.append({placeTarget, m_config.placeSpeed, 0.0});

    // Waypoint 9: Open gripper (action point)
    plan.waypoints.append({placeTarget, 0.0, 0.0});

    // Waypoint 10: Retreat from place
    CartesianPose placeRetreat = placeTarget;
    placeRetreat.z = m_config.safeHeight;
    plan.waypoints.append({placeRetreat, m_config.placeSpeed, 5.0});

    // Estimate total duration
    double totalDist = 0;
    for (int i = 1; i < plan.waypoints.size(); ++i) {
        double d = poseDistance(plan.waypoints[i - 1].pose, plan.waypoints[i].pose);
        double spd = plan.waypoints[i].speed;
        if (spd > 0)
            plan.estimatedDuration += d / spd;
        else
            plan.estimatedDuration += 0.5; // action point duration
        totalDist += d;
    }

    plan.valid = true;
    return plan;
}

bool TrajectoryPlanner::execute(IRobotDriver* robot, const TrajectoryPlan& plan) {
    if (!robot || !plan.valid)
        return false;

    const int gripperPin = 0; // Default gripper DO pin
    bool gripperClosed = false;

    for (int i = 0; i < plan.waypoints.size(); ++i) {
        const auto& wp = plan.waypoints[i];

        if (wp.speed <= 0.0) {
            // Action point: toggle gripper
            gripperClosed = !gripperClosed;
            robot->setDigitalOutput(gripperPin, gripperClosed);
            continue;
        }

        if (!robot->moveTo(wp.pose, wp.speed))
            return false;
    }

    return true;
}

double TrajectoryPlanner::poseDistance(const CartesianPose& a, const CartesianPose& b) {
    double dx = a.x - b.x;
    double dy = a.y - b.y;
    double dz = a.z - b.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}
