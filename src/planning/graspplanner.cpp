#include "graspplanner.h"
#include <algorithm>
#include <cmath>

void GraspPlanner::setConfig(const Config& config) {
    m_config = config;
}

GraspPlanner::Config GraspPlanner::config() const {
    return m_config;
}

QVector<GraspCandidate> GraspPlanner::plan(const DetectionResult& target) {
    QVector<GraspCandidate> candidates;

    // Generate primary top-down grasp
    {
        GraspCandidate c;
        c.graspPose = target.pose6D;
        // Top-down approach: same XY, higher Z
        c.approachPose = target.pose6D;
        c.approachPose.z -= m_config.approachDistance;  // Z is camera frame, lower = closer
        c.retreatPose = target.pose6D;
        c.retreatPose.z -= m_config.retreatDistance;
        c.score = target.confidence * 0.9;
        candidates.append(c);
    }

    // Generate angled grasp variant (tilted 15 degrees around Y)
    {
        GraspCandidate c;
        c.graspPose = target.pose6D;
        c.graspPose.ry = 0.26;  // ~15 degrees
        c.approachPose = c.graspPose;
        c.approachPose.z -= m_config.approachDistance;
        c.retreatPose = c.graspPose;
        c.retreatPose.z -= m_config.retreatDistance;
        c.score = target.confidence * 0.75;
        candidates.append(c);
    }

    // Generate angled grasp variant (tilted -15 degrees around Y)
    {
        GraspCandidate c;
        c.graspPose = target.pose6D;
        c.graspPose.ry = -0.26;
        c.approachPose = c.graspPose;
        c.approachPose.z -= m_config.approachDistance;
        c.retreatPose = c.graspPose;
        c.retreatPose.z -= m_config.retreatDistance;
        c.score = target.confidence * 0.7;
        candidates.append(c);
    }

    // Sort by score descending
    std::sort(candidates.begin(), candidates.end(),
              [](const GraspCandidate& a, const GraspCandidate& b) {
                  return a.score > b.score;
              });

    // Limit to maxCandidates
    if (candidates.size() > m_config.maxCandidates)
        candidates.resize(m_config.maxCandidates);

    // Filter by minScore
    candidates.erase(
        std::remove_if(candidates.begin(), candidates.end(),
                       [this](const GraspCandidate& c) { return c.score < m_config.minScore; }),
        candidates.end());

    return candidates;
}

bool GraspPlanner::checkCollision(const GraspCandidate& candidate,
                                   const PointCloud& sceneCloud,
                                   double safetyMargin) {
    // Simplified collision check:
    // Check if any scene points are within safetyMargin of the approach path
    const auto& ap = candidate.approachPose;
    const auto& gp = candidate.graspPose;

    for (int i = 0; i < sceneCloud.pointCount(); ++i) {
        float px = sceneCloud.points[i * 3 + 0];
        float py = sceneCloud.points[i * 3 + 1];
        float pz = sceneCloud.points[i * 3 + 2];

        // Distance from point to approach line segment (simplified XY distance)
        double dx = px - (ap.x + gp.x) / 2.0;
        double dy = py - (ap.y + gp.y) / 2.0;
        double dist = std::sqrt(dx * dx + dy * dy);

        // Check if within approach corridor but not at the grasp target itself
        double dz = std::abs(pz - (ap.z + gp.z) / 2.0);
        if (dist < safetyMargin && dz < std::abs(ap.z - gp.z) / 2.0) {
            return true; // collision detected
        }
    }
    return false;
}
