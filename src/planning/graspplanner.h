#ifndef GRASPPLANNER_H
#define GRASPPLANNER_H

#include <QVector>
#include "../device/icameradriver.h"
#include "../device/irobotdriver.h"
#include "../vision/visionpipeline.h"

struct GraspCandidate {
    CartesianPose approachPose;   // Pre-grasp approach
    CartesianPose graspPose;      // Actual grasp pose
    CartesianPose retreatPose;    // Post-grasp retreat
    double score = 0.0;           // Grasp quality [0, 1]
};

class GraspPlanner {
public:
    struct Config {
        double minScore = 0.4;
        int maxCandidates = 5;
        double approachDistance = 80.0;   // mm above grasp point
        double retreatDistance = 100.0;   // mm above after grasp
        bool enableCollisionCheck = true;
    };

    void setConfig(const Config& config);
    Config config() const;

    // Generate ranked grasp candidates for a detected object
    QVector<GraspCandidate> plan(const DetectionResult& target);

    // Simple collision check placeholder
    static bool checkCollision(const GraspCandidate& candidate,
                               const PointCloud& sceneCloud,
                               double safetyMargin = 20.0);

private:
    Config m_config;
};

#endif // GRASPPLANNER_H
