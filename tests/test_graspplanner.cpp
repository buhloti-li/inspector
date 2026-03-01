#include <QtTest/QtTest>

#include "planning/graspplanner.h"

class GraspPlannerTest : public QObject {
    Q_OBJECT

private slots:
    void testPlanGeneratesCandidates();
    void testMinScoreFilter();
    void testMaxCandidatesLimit();
    void testCandidatesSortedByScore();
};

void GraspPlannerTest::testPlanGeneratesCandidates() {
    GraspPlanner planner;

    DetectionResult target;
    target.objectClass = "test_part";
    target.confidence = 0.85f;
    target.pose6D = {100.0, 50.0, 400.0, 0, 0, 0};

    auto candidates = planner.plan(target);
    QVERIFY(!candidates.isEmpty());

    // Verify approach is above grasp
    for (const auto& c : candidates) {
        QVERIFY(c.approachPose.z < c.graspPose.z); // lower z = higher in camera frame
        QVERIFY(c.score > 0.0);
    }
}

void GraspPlannerTest::testMinScoreFilter() {
    GraspPlanner planner;
    GraspPlanner::Config config;
    config.minScore = 0.8;
    planner.setConfig(config);

    DetectionResult target;
    target.confidence = 0.5f; // Low confidence → low scores
    target.pose6D = {0, 0, 400, 0, 0, 0};

    auto candidates = planner.plan(target);
    for (const auto& c : candidates) {
        QVERIFY(c.score >= 0.8);
    }
}

void GraspPlannerTest::testMaxCandidatesLimit() {
    GraspPlanner planner;
    GraspPlanner::Config config;
    config.maxCandidates = 2;
    config.minScore = 0.0;
    planner.setConfig(config);

    DetectionResult target;
    target.confidence = 0.9f;
    target.pose6D = {0, 0, 400, 0, 0, 0};

    auto candidates = planner.plan(target);
    QVERIFY(candidates.size() <= 2);
}

void GraspPlannerTest::testCandidatesSortedByScore() {
    GraspPlanner planner;
    GraspPlanner::Config config;
    config.minScore = 0.0;
    planner.setConfig(config);

    DetectionResult target;
    target.confidence = 0.8f;
    target.pose6D = {0, 0, 400, 0, 0, 0};

    auto candidates = planner.plan(target);
    QVERIFY(candidates.size() >= 2);
    for (int i = 1; i < candidates.size(); ++i) {
        QVERIFY(candidates[i - 1].score >= candidates[i].score);
    }
}

QTEST_MAIN(GraspPlannerTest)
#include "test_graspplanner.moc"
