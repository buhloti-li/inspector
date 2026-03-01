#include <QtTest/QtTest>

#include "planning/graspplanner.h"

class GraspPlannerTest : public QObject {
    Q_OBJECT

private slots:
    // === 正常规划 ===
    void testPlanGeneratesCandidates();
    void testCandidatesSortedByScore();
    void testApproachAboveGrasp();
    void testRetreatAboveGrasp();

    // === 配置参数 ===
    void testMinScoreFilter();
    void testMaxCandidatesLimit();
    void testMaxCandidatesOne();
    void testMaxCandidatesZero();
    void testDefaultConfig();
    void testConfigRoundTrip();

    // === 输入边界 ===
    void testZeroConfidenceTarget();
    void testPerfectConfidenceTarget();
    void testVeryLowConfidence();
    void testTargetAtOrigin();
    void testTargetAtExtremePosition();
    void testTargetWithRotation();
    void testTargetNegativeCoordinates();

    // === 碰撞检测 ===
    void testCollisionCheckNoObstacles();
    void testCollisionCheckWithObstacle();
    void testCollisionCheckEmptyCloud();
    void testCollisionCheckFarObstacle();

    // === 抓取距离参数 ===
    void testCustomApproachDistance();
    void testCustomRetreatDistance();

    // === 多目标/一致性 ===
    void testPlanForDifferentTargets();
    void testConsistentPlanningForSameTarget();
};

void GraspPlannerTest::testPlanGeneratesCandidates() {
    GraspPlanner planner;
    DetectionResult target;
    target.objectClass = "test_part";
    target.confidence = 0.85f;
    target.pose6D = {100.0, 50.0, 400.0, 0, 0, 0};

    auto candidates = planner.plan(target);
    QVERIFY(!candidates.isEmpty());
    for (const auto& c : candidates) {
        QVERIFY(c.approachPose.z < c.graspPose.z);
        QVERIFY(c.score > 0.0);
    }
}

void GraspPlannerTest::testCandidatesSortedByScore() {
    GraspPlanner planner;
    GraspPlanner::Config config; config.minScore = 0.0; planner.setConfig(config);

    DetectionResult target; target.confidence = 0.8f; target.pose6D = {0, 0, 400, 0, 0, 0};
    auto candidates = planner.plan(target);
    QVERIFY(candidates.size() >= 2);
    for (int i = 1; i < candidates.size(); ++i)
        QVERIFY(candidates[i - 1].score >= candidates[i].score);
}

void GraspPlannerTest::testApproachAboveGrasp() {
    GraspPlanner planner;
    DetectionResult target; target.confidence = 0.9f; target.pose6D = {0, 0, 400, 0, 0, 0};
    for (const auto& c : planner.plan(target))
        QVERIFY(c.approachPose.z < c.graspPose.z);
}

void GraspPlannerTest::testRetreatAboveGrasp() {
    GraspPlanner planner;
    DetectionResult target; target.confidence = 0.9f; target.pose6D = {0, 0, 400, 0, 0, 0};
    for (const auto& c : planner.plan(target))
        QVERIFY(c.retreatPose.z < c.graspPose.z);
}

void GraspPlannerTest::testMinScoreFilter() {
    GraspPlanner planner;
    GraspPlanner::Config config; config.minScore = 0.8; planner.setConfig(config);
    DetectionResult target; target.confidence = 0.5f; target.pose6D = {0, 0, 400, 0, 0, 0};
    for (const auto& c : planner.plan(target))
        QVERIFY(c.score >= 0.8);
}

void GraspPlannerTest::testMaxCandidatesLimit() {
    GraspPlanner planner;
    GraspPlanner::Config config; config.maxCandidates = 2; config.minScore = 0.0; planner.setConfig(config);
    DetectionResult target; target.confidence = 0.9f; target.pose6D = {0, 0, 400, 0, 0, 0};
    QVERIFY(planner.plan(target).size() <= 2);
}

void GraspPlannerTest::testMaxCandidatesOne() {
    GraspPlanner planner;
    GraspPlanner::Config config; config.maxCandidates = 1; config.minScore = 0.0; planner.setConfig(config);
    DetectionResult target; target.confidence = 0.9f; target.pose6D = {0, 0, 400, 0, 0, 0};
    QCOMPARE(planner.plan(target).size(), 1);
}

void GraspPlannerTest::testMaxCandidatesZero() {
    GraspPlanner planner;
    GraspPlanner::Config config; config.maxCandidates = 0; config.minScore = 0.0; planner.setConfig(config);
    DetectionResult target; target.confidence = 0.9f; target.pose6D = {0, 0, 400, 0, 0, 0};
    QVERIFY(planner.plan(target).isEmpty());
}

void GraspPlannerTest::testDefaultConfig() {
    GraspPlanner planner;
    auto c = planner.config();
    QCOMPARE(c.minScore, 0.4);
    QCOMPARE(c.maxCandidates, 5);
    QCOMPARE(c.approachDistance, 80.0);
    QCOMPARE(c.retreatDistance, 100.0);
    QVERIFY(c.enableCollisionCheck);
}

void GraspPlannerTest::testConfigRoundTrip() {
    GraspPlanner planner;
    GraspPlanner::Config config; config.minScore = 0.75; config.maxCandidates = 3;
    config.approachDistance = 120.0; config.retreatDistance = 150.0; config.enableCollisionCheck = false;
    planner.setConfig(config);
    auto r = planner.config();
    QCOMPARE(r.minScore, 0.75); QCOMPARE(r.maxCandidates, 3);
    QCOMPARE(r.approachDistance, 120.0); QCOMPARE(r.retreatDistance, 150.0);
    QVERIFY(!r.enableCollisionCheck);
}

void GraspPlannerTest::testZeroConfidenceTarget() {
    GraspPlanner planner;
    GraspPlanner::Config config; config.minScore = 0.0; planner.setConfig(config);
    DetectionResult target; target.confidence = 0.0f; target.pose6D = {0, 0, 400, 0, 0, 0};
    for (const auto& c : planner.plan(target))
        QCOMPARE(c.score, 0.0);
}

void GraspPlannerTest::testPerfectConfidenceTarget() {
    GraspPlanner planner;
    GraspPlanner::Config config; config.minScore = 0.0; planner.setConfig(config);
    DetectionResult target; target.confidence = 1.0f; target.pose6D = {0, 0, 400, 0, 0, 0};
    auto candidates = planner.plan(target);
    QVERIFY(!candidates.isEmpty());
    QCOMPARE(candidates.first().score, 0.9);
}

void GraspPlannerTest::testVeryLowConfidence() {
    GraspPlanner planner;
    DetectionResult target; target.confidence = 0.01f; target.pose6D = {0, 0, 400, 0, 0, 0};
    QVERIFY(planner.plan(target).isEmpty()); // all below default minScore=0.4
}

void GraspPlannerTest::testTargetAtOrigin() {
    GraspPlanner planner;
    GraspPlanner::Config config; config.minScore = 0.0; planner.setConfig(config);
    DetectionResult target; target.confidence = 0.8f; target.pose6D = {0, 0, 0, 0, 0, 0};
    auto c = planner.plan(target);
    QVERIFY(!c.isEmpty());
    QCOMPARE(c.first().graspPose.x, 0.0);
}

void GraspPlannerTest::testTargetAtExtremePosition() {
    GraspPlanner planner;
    GraspPlanner::Config config; config.minScore = 0.0; planner.setConfig(config);
    DetectionResult target; target.confidence = 0.8f; target.pose6D = {5000, -3000, 900, 0, 0, 0};
    QVERIFY(!planner.plan(target).isEmpty());
}

void GraspPlannerTest::testTargetWithRotation() {
    GraspPlanner planner;
    GraspPlanner::Config config; config.minScore = 0.0; planner.setConfig(config);
    DetectionResult target; target.confidence = 0.8f; target.pose6D = {100, 100, 400, 1.57, 0.5, -0.3};
    QVERIFY(!planner.plan(target).isEmpty());
}

void GraspPlannerTest::testTargetNegativeCoordinates() {
    GraspPlanner planner;
    GraspPlanner::Config config; config.minScore = 0.0; planner.setConfig(config);
    DetectionResult target; target.confidence = 0.8f; target.pose6D = {-500, -300, -100, 0, 0, 0};
    auto c = planner.plan(target);
    QVERIFY(!c.isEmpty());
    QCOMPARE(c.first().graspPose.x, -500.0);
}

void GraspPlannerTest::testCollisionCheckNoObstacles() {
    GraspCandidate c; c.approachPose = {100, 100, 300, 0, 0, 0}; c.graspPose = {100, 100, 400, 0, 0, 0};
    PointCloud empty;
    QVERIFY(!GraspPlanner::checkCollision(c, empty));
}

void GraspPlannerTest::testCollisionCheckWithObstacle() {
    GraspCandidate c; c.approachPose = {100, 100, 300, 0, 0, 0}; c.graspPose = {100, 100, 400, 0, 0, 0};
    PointCloud cloud;
    for (int i = 0; i < 10; ++i) { cloud.points.push_back(100.0f); cloud.points.push_back(100.0f); cloud.points.push_back(350.0f); }
    QVERIFY(GraspPlanner::checkCollision(c, cloud, 20.0));
}

void GraspPlannerTest::testCollisionCheckEmptyCloud() {
    GraspCandidate c; c.approachPose = {0, 0, 0, 0, 0, 0}; c.graspPose = {0, 0, 100, 0, 0, 0};
    QVERIFY(!GraspPlanner::checkCollision(c, PointCloud{}));
}

void GraspPlannerTest::testCollisionCheckFarObstacle() {
    GraspCandidate c; c.approachPose = {100, 100, 300, 0, 0, 0}; c.graspPose = {100, 100, 400, 0, 0, 0};
    PointCloud cloud;
    cloud.points.push_back(999.0f); cloud.points.push_back(999.0f); cloud.points.push_back(350.0f);
    QVERIFY(!GraspPlanner::checkCollision(c, cloud, 20.0));
}

void GraspPlannerTest::testCustomApproachDistance() {
    GraspPlanner planner;
    GraspPlanner::Config config; config.approachDistance = 200.0; config.minScore = 0.0; planner.setConfig(config);
    DetectionResult target; target.confidence = 0.8f; target.pose6D = {0, 0, 500, 0, 0, 0};
    auto c = planner.plan(target);
    QVERIFY(!c.isEmpty());
    QCOMPARE(c.first().graspPose.z - c.first().approachPose.z, 200.0);
}

void GraspPlannerTest::testCustomRetreatDistance() {
    GraspPlanner planner;
    GraspPlanner::Config config; config.retreatDistance = 250.0; config.minScore = 0.0; planner.setConfig(config);
    DetectionResult target; target.confidence = 0.8f; target.pose6D = {0, 0, 500, 0, 0, 0};
    auto c = planner.plan(target);
    QVERIFY(!c.isEmpty());
    QCOMPARE(c.first().graspPose.z - c.first().retreatPose.z, 250.0);
}

void GraspPlannerTest::testPlanForDifferentTargets() {
    GraspPlanner planner;
    GraspPlanner::Config config; config.minScore = 0.0; planner.setConfig(config);
    DetectionResult t1; t1.confidence = 0.9f; t1.pose6D = {100, 100, 400, 0, 0, 0};
    DetectionResult t2; t2.confidence = 0.5f; t2.pose6D = {-200, 300, 500, 0, 0, 0};
    QVERIFY(planner.plan(t1).first().score > planner.plan(t2).first().score);
}

void GraspPlannerTest::testConsistentPlanningForSameTarget() {
    GraspPlanner planner;
    GraspPlanner::Config config; config.minScore = 0.0; planner.setConfig(config);
    DetectionResult target; target.confidence = 0.8f; target.pose6D = {100, 200, 400, 0, 0, 0};
    auto c1 = planner.plan(target);
    auto c2 = planner.plan(target);
    QCOMPARE(c1.size(), c2.size());
    for (int i = 0; i < c1.size(); ++i) {
        QCOMPARE(c1[i].score, c2[i].score);
        QCOMPARE(c1[i].graspPose.x, c2[i].graspPose.x);
    }
}

QTEST_MAIN(GraspPlannerTest)
#include "test_graspplanner.moc"
