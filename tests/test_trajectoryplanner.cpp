#include <QtTest/QtTest>

#include "planning/trajectoryplanner.h"
#include "device/sim/simrobotdriver.h"

class TrajectoryPlannerTest : public QObject {
    Q_OBJECT

private slots:
    // === 正常规划 ===
    void testPlanPickPlaceValid();
    void testPlanHasCorrectWaypointCount();
    void testPlanEstimatedDuration();
    void testPlanContainsActionPoints();

    // === 配置参数 ===
    void testDefaultConfig();
    void testConfigRoundTrip();
    void testCustomSafeHeight();
    void testCustomSpeeds();

    // === 执行 ===
    void testExecuteSuccess();
    void testExecuteWithFailingRobot();
    void testExecuteNullRobot();
    void testExecuteInvalidPlan();
    void testExecuteGripperToggle();

    // === 边界条件 ===
    void testPlanFromSamePosition();
    void testPlanToSamePosition();
    void testPlanWithZeroCoordinates();
    void testPlanWithNegativeCoordinates();
    void testPlanWithLargeDistance();

    // === 机器人运动失败中途 ===
    void testExecuteFailsMidway();
    void testExecuteAfterDisconnect();
};

void TrajectoryPlannerTest::testPlanPickPlaceValid() {
    TrajectoryPlanner planner;

    CartesianPose current = {0, 0, 500, 0, 0, 0};
    GraspCandidate grasp;
    grasp.approachPose = {100, 100, 320, 0, 0, 0};
    grasp.graspPose = {100, 100, 400, 0, 0, 0};
    grasp.retreatPose = {100, 100, 300, 0, 0, 0};

    CartesianPose place = {300, 0, 400, 0, 0, 0};

    auto plan = planner.planPickPlace(current, grasp, place);
    QVERIFY(plan.valid);
    QVERIFY(!plan.waypoints.isEmpty());
    QVERIFY(plan.estimatedDuration > 0);
}

void TrajectoryPlannerTest::testPlanHasCorrectWaypointCount() {
    TrajectoryPlanner planner;
    CartesianPose current = {0, 0, 500, 0, 0, 0};
    GraspCandidate grasp;
    grasp.approachPose = {100, 100, 320, 0, 0, 0};
    grasp.graspPose = {100, 100, 400, 0, 0, 0};
    grasp.retreatPose = {100, 100, 300, 0, 0, 0};
    CartesianPose place = {300, 0, 400, 0, 0, 0};

    auto plan = planner.planPickPlace(current, grasp, place);
    // 10 waypoints: safe start, above approach, approach, grasp, close gripper,
    // retreat, above place, place, open gripper, place retreat
    QCOMPARE(plan.waypoints.size(), 10);
}

void TrajectoryPlannerTest::testPlanEstimatedDuration() {
    TrajectoryPlanner planner;
    CartesianPose current = {0, 0, 500, 0, 0, 0};
    GraspCandidate grasp;
    grasp.approachPose = {100, 100, 320, 0, 0, 0};
    grasp.graspPose = {100, 100, 400, 0, 0, 0};
    grasp.retreatPose = {100, 100, 300, 0, 0, 0};
    CartesianPose place = {300, 0, 400, 0, 0, 0};

    auto plan = planner.planPickPlace(current, grasp, place);
    // Duration should be reasonable (> 1 second for real movement)
    QVERIFY(plan.estimatedDuration > 0.5);
    QVERIFY(plan.estimatedDuration < 60.0); // not unreasonably long
}

void TrajectoryPlannerTest::testPlanContainsActionPoints() {
    TrajectoryPlanner planner;
    CartesianPose current = {0, 0, 500, 0, 0, 0};
    GraspCandidate grasp;
    grasp.approachPose = {100, 100, 320, 0, 0, 0};
    grasp.graspPose = {100, 100, 400, 0, 0, 0};
    grasp.retreatPose = {100, 100, 300, 0, 0, 0};
    CartesianPose place = {300, 0, 400, 0, 0, 0};

    auto plan = planner.planPickPlace(current, grasp, place);
    int actionPoints = 0;
    for (const auto& wp : plan.waypoints) {
        if (wp.speed <= 0.0)
            ++actionPoints;
    }
    QCOMPARE(actionPoints, 2); // close gripper + open gripper
}

void TrajectoryPlannerTest::testDefaultConfig() {
    TrajectoryPlanner planner;
    auto c = planner.config();
    QCOMPARE(c.maxSpeed, 500.0);
    QCOMPARE(c.maxAcceleration, 1000.0);
    QCOMPARE(c.safeHeight, 200.0);
    QCOMPARE(c.approachSpeed, 100.0);
    QCOMPARE(c.placeSpeed, 150.0);
}

void TrajectoryPlannerTest::testConfigRoundTrip() {
    TrajectoryPlanner planner;
    TrajectoryPlanner::Config config;
    config.maxSpeed = 800; config.maxAcceleration = 2000;
    config.safeHeight = 300; config.approachSpeed = 50; config.placeSpeed = 75;
    planner.setConfig(config);

    auto r = planner.config();
    QCOMPARE(r.maxSpeed, 800.0);
    QCOMPARE(r.safeHeight, 300.0);
    QCOMPARE(r.approachSpeed, 50.0);
}

void TrajectoryPlannerTest::testCustomSafeHeight() {
    TrajectoryPlanner planner;
    TrajectoryPlanner::Config config; config.safeHeight = 400.0;
    planner.setConfig(config);

    CartesianPose current = {0, 0, 500, 0, 0, 0};
    GraspCandidate grasp;
    grasp.approachPose = {100, 0, 320, 0, 0, 0};
    grasp.graspPose = {100, 0, 400, 0, 0, 0};
    grasp.retreatPose = {100, 0, 300, 0, 0, 0};
    CartesianPose place = {200, 0, 400, 0, 0, 0};

    auto plan = planner.planPickPlace(current, grasp, place);
    // First waypoint should be at safe height
    QCOMPARE(plan.waypoints.first().pose.z, 400.0);
}

void TrajectoryPlannerTest::testCustomSpeeds() {
    TrajectoryPlanner planner;
    TrajectoryPlanner::Config config;
    config.maxSpeed = 200; config.approachSpeed = 30; config.placeSpeed = 40;
    planner.setConfig(config);

    CartesianPose current = {0, 0, 500, 0, 0, 0};
    GraspCandidate grasp;
    grasp.approachPose = {100, 0, 320, 0, 0, 0};
    grasp.graspPose = {100, 0, 400, 0, 0, 0};
    grasp.retreatPose = {100, 0, 300, 0, 0, 0};
    CartesianPose place = {200, 0, 400, 0, 0, 0};

    auto plan = planner.planPickPlace(current, grasp, place);
    // Transit waypoints should use maxSpeed
    QCOMPARE(plan.waypoints[0].speed, 200.0);
}

void TrajectoryPlannerTest::testExecuteSuccess() {
    SimRobotDriver robot;
    robot.connect({});

    TrajectoryPlanner planner;
    CartesianPose current = robot.currentPose();
    GraspCandidate grasp;
    grasp.approachPose = {100, 0, 420, 0, 0, 0};
    grasp.graspPose = {100, 0, 500, 0, 0, 0};
    grasp.retreatPose = {100, 0, 400, 0, 0, 0};
    CartesianPose place = {200, 0, 500, 0, 0, 0};

    auto plan = planner.planPickPlace(current, grasp, place);
    QVERIFY(TrajectoryPlanner::execute(&robot, plan));
}

void TrajectoryPlannerTest::testExecuteWithFailingRobot() {
    SimRobotDriver robot;
    robot.connect({});
    robot.setFailNextMove(true);

    TrajectoryPlanner planner;
    CartesianPose current = {0, 0, 500, 0, 0, 0};
    GraspCandidate grasp;
    grasp.approachPose = {100, 0, 420, 0, 0, 0};
    grasp.graspPose = {100, 0, 500, 0, 0, 0};
    grasp.retreatPose = {100, 0, 400, 0, 0, 0};
    CartesianPose place = {200, 0, 500, 0, 0, 0};

    auto plan = planner.planPickPlace(current, grasp, place);
    QVERIFY(!TrajectoryPlanner::execute(&robot, plan));
}

void TrajectoryPlannerTest::testExecuteNullRobot() {
    TrajectoryPlan plan;
    plan.valid = true;
    QVERIFY(!TrajectoryPlanner::execute(nullptr, plan));
}

void TrajectoryPlannerTest::testExecuteInvalidPlan() {
    SimRobotDriver robot;
    robot.connect({});

    TrajectoryPlan plan;
    plan.valid = false;
    QVERIFY(!TrajectoryPlanner::execute(&robot, plan));
}

void TrajectoryPlannerTest::testExecuteGripperToggle() {
    SimRobotDriver robot;
    robot.connect({});

    TrajectoryPlanner planner;
    CartesianPose current = robot.currentPose();
    GraspCandidate grasp;
    grasp.approachPose = {100, 0, 420, 0, 0, 0};
    grasp.graspPose = {100, 0, 500, 0, 0, 0};
    grasp.retreatPose = {100, 0, 400, 0, 0, 0};
    CartesianPose place = {200, 0, 500, 0, 0, 0};

    auto plan = planner.planPickPlace(current, grasp, place);
    QVERIFY(TrajectoryPlanner::execute(&robot, plan));

    // Gripper should be open (toggled twice: close then open)
    QVERIFY(!robot.digitalOutput(0));
}

void TrajectoryPlannerTest::testPlanFromSamePosition() {
    TrajectoryPlanner planner;
    CartesianPose pos = {100, 100, 400, 0, 0, 0};
    GraspCandidate grasp;
    grasp.approachPose = pos; grasp.graspPose = pos; grasp.retreatPose = pos;

    auto plan = planner.planPickPlace(pos, grasp, pos);
    QVERIFY(plan.valid);
}

void TrajectoryPlannerTest::testPlanToSamePosition() {
    TrajectoryPlanner planner;
    CartesianPose current = {0, 0, 500, 0, 0, 0};
    GraspCandidate grasp;
    grasp.approachPose = {100, 0, 420, 0, 0, 0};
    grasp.graspPose = {100, 0, 500, 0, 0, 0};
    grasp.retreatPose = {100, 0, 400, 0, 0, 0};
    // Place at same position as grasp
    auto plan = planner.planPickPlace(current, grasp, grasp.graspPose);
    QVERIFY(plan.valid);
}

void TrajectoryPlannerTest::testPlanWithZeroCoordinates() {
    TrajectoryPlanner planner;
    CartesianPose origin = {0, 0, 0, 0, 0, 0};
    GraspCandidate grasp;
    grasp.approachPose = origin; grasp.graspPose = origin; grasp.retreatPose = origin;
    auto plan = planner.planPickPlace(origin, grasp, origin);
    QVERIFY(plan.valid);
}

void TrajectoryPlannerTest::testPlanWithNegativeCoordinates() {
    TrajectoryPlanner planner;
    CartesianPose current = {-100, -200, -300, 0, 0, 0};
    GraspCandidate grasp;
    grasp.approachPose = {-50, -50, -100, 0, 0, 0};
    grasp.graspPose = {-50, -50, 0, 0, 0, 0};
    grasp.retreatPose = {-50, -50, -150, 0, 0, 0};
    CartesianPose place = {-200, -200, 0, 0, 0, 0};
    auto plan = planner.planPickPlace(current, grasp, place);
    QVERIFY(plan.valid);
}

void TrajectoryPlannerTest::testPlanWithLargeDistance() {
    TrajectoryPlanner planner;
    CartesianPose current = {0, 0, 500, 0, 0, 0};
    GraspCandidate grasp;
    grasp.approachPose = {10000, 10000, 9920, 0, 0, 0};
    grasp.graspPose = {10000, 10000, 10000, 0, 0, 0};
    grasp.retreatPose = {10000, 10000, 9900, 0, 0, 0};
    CartesianPose place = {-10000, -10000, 10000, 0, 0, 0};

    auto plan = planner.planPickPlace(current, grasp, place);
    QVERIFY(plan.valid);
    QVERIFY(plan.estimatedDuration > 10.0); // long distance = long time
}

void TrajectoryPlannerTest::testExecuteFailsMidway() {
    SimRobotDriver robot;
    robot.connect({});

    TrajectoryPlanner planner;
    CartesianPose current = robot.currentPose();
    GraspCandidate grasp;
    grasp.approachPose = {100, 0, 420, 0, 0, 0};
    grasp.graspPose = {100, 0, 500, 0, 0, 0};
    grasp.retreatPose = {100, 0, 400, 0, 0, 0};
    CartesianPose place = {200, 0, 500, 0, 0, 0};

    auto plan = planner.planPickPlace(current, grasp, place);

    // Make robot fail after first successful move
    // (SimRobot succeeds first move, then we set fail before second)
    // We can't easily test mid-execution failure with current sync API,
    // but we can test with immediate failure
    robot.setFailNextMove(true);
    QVERIFY(!TrajectoryPlanner::execute(&robot, plan));
}

void TrajectoryPlannerTest::testExecuteAfterDisconnect() {
    SimRobotDriver robot;
    robot.connect({});
    robot.disconnect(); // disconnect

    TrajectoryPlanner planner;
    CartesianPose current = {0, 0, 500, 0, 0, 0};
    GraspCandidate grasp;
    grasp.approachPose = {100, 0, 420, 0, 0, 0};
    grasp.graspPose = {100, 0, 500, 0, 0, 0};
    grasp.retreatPose = {100, 0, 400, 0, 0, 0};
    CartesianPose place = {200, 0, 500, 0, 0, 0};

    auto plan = planner.planPickPlace(current, grasp, place);
    QVERIFY(!TrajectoryPlanner::execute(&robot, plan));
}

QTEST_MAIN(TrajectoryPlannerTest)
#include "test_trajectoryplanner.moc"
