#include <QtTest/QtTest>

#include "workcellcontroller.h"
#include "device/devicemanager.h"
#include "device/sim/simcameradriver.h"
#include "device/sim/simrobotdriver.h"
#include "vision/visionpipeline.h"
#include "planning/graspplanner.h"
#include "planning/trajectoryplanner.h"
#include "orchestration/recoverymanager.h"
#include "orchestration/workflowengine.h"

class IntegrationPickCycleTest : public QObject {
    Q_OBJECT

private slots:
    // === 完整抓取循环（正常） ===
    void testFullPickCycleSuccess();
    void testFullPickCycleWithRecipe();

    // === 设备层 → 视觉 → 规划 → 执行 全链路 ===
    void testEndToEndVisionToExecution();

    // === 设备故障场景 ===
    void testRobotFailureDuringExecution();
    void testCameraDisconnectDuringCycle();
    void testEmptySceneTriggersAlert();

    // === 恢复管理器集成 ===
    void testRecoveryAfterGraspFailure();
    void testRecoveryEStopOnCommLoss();

    // === 多次循环稳定性 ===
    void testMultipleCyclesStable();

    // === 工作流引擎 + 控制器集成 ===
    void testWorkflowEngineWithController();

    // === 自动模式前置条件验证 ===
    void testAutoModeFullSetup();

    // === 设备热更换后继续工作 ===
    void testDeviceReplacementMidSession();
};

void IntegrationPickCycleTest::testFullPickCycleSuccess() {
    // Setup devices
    DeviceManager mgr;
    auto cam = std::make_unique<SimCameraDriver>("cam-01");
    auto robot = std::make_unique<SimRobotDriver>("robot-01");
    cam->connect({});
    cam->setObjectCount(3);
    robot->connect({});

    mgr.registerDevice("cam-01", std::move(cam));
    mgr.registerDevice("robot-01", std::move(robot));

    // Vision
    VisionPipeline vision;
    vision.setCameraDriver(mgr.camera("cam-01"));
    vision.setMinConfidence(0.1f);
    auto detections = vision.execute();
    QVERIFY(!detections.isEmpty());

    // Grasp planning
    GraspPlanner graspPlanner;
    GraspPlanner::Config gc; gc.minScore = 0.0; graspPlanner.setConfig(gc);
    auto candidates = graspPlanner.plan(detections.first());
    QVERIFY(!candidates.isEmpty());

    // Trajectory planning
    TrajectoryPlanner trajPlanner;
    CartesianPose place = {300, 0, 400, 0, 0, 0};
    auto plan = trajPlanner.planPickPlace(
        mgr.robot("robot-01")->currentPose(),
        candidates.first(),
        place
    );
    QVERIFY(plan.valid);

    // Execute
    QVERIFY(TrajectoryPlanner::execute(mgr.robot("robot-01"), plan));
}

void IntegrationPickCycleTest::testFullPickCycleWithRecipe() {
    DeviceManager mgr;
    auto cam = std::make_unique<SimCameraDriver>("cam-01");
    auto robot = std::make_unique<SimRobotDriver>("robot-01");
    cam->connect({});
    cam->setObjectCount(2);
    robot->connect({});
    mgr.registerDevice("cam-01", std::move(cam));
    mgr.registerDevice("robot-01", std::move(robot));

    WorkcellController controller;
    controller.setDeviceManager(&mgr);
    controller.bindCamera("cam-01");
    controller.bindRobot("robot-01");

    QVariantMap recipe;
    recipe["minConfidence"] = 0.1;
    recipe["placeX"] = 400.0;
    recipe["placeY"] = 100.0;
    recipe["placeZ"] = 350.0;
    controller.loadRecipe(recipe);

    QVERIFY(controller.startTask());
    QCOMPARE(controller.recipe()["placeX"].toDouble(), 400.0);
}

void IntegrationPickCycleTest::testEndToEndVisionToExecution() {
    // Full chain: camera -> vision -> grasp -> trajectory -> robot
    SimCameraDriver cam;
    cam.connect({});
    cam.setObjectCount(2);
    cam.setNoiseLevel(0.01);

    SimRobotDriver robot;
    robot.connect({});

    // Vision
    VisionPipeline vision;
    vision.setCameraDriver(&cam);
    vision.setMinConfidence(0.1f);
    auto dets = vision.execute();
    QVERIFY(!dets.isEmpty());

    // Plan
    GraspPlanner gp;
    GraspPlanner::Config gc; gc.minScore = 0.0; gp.setConfig(gc);
    auto grasps = gp.plan(dets.first());
    QVERIFY(!grasps.isEmpty());

    TrajectoryPlanner tp;
    CartesianPose place = {200, 100, 400, 0, 0, 0};
    auto trajectory = tp.planPickPlace(robot.currentPose(), grasps.first(), place);
    QVERIFY(trajectory.valid);
    QVERIFY(trajectory.estimatedDuration > 0);

    // Execute
    QVERIFY(TrajectoryPlanner::execute(&robot, trajectory));

    // Verify robot moved to final position (place retreat at safe height)
    auto finalPose = robot.currentPose();
    QCOMPARE(finalPose.z, tp.config().safeHeight);
}

void IntegrationPickCycleTest::testRobotFailureDuringExecution() {
    SimCameraDriver cam;
    cam.connect({});
    cam.setObjectCount(2);

    SimRobotDriver robot;
    robot.connect({});
    robot.setFailNextMove(true); // will fail on first move

    VisionPipeline vision;
    vision.setCameraDriver(&cam);
    vision.setMinConfidence(0.1f);
    auto dets = vision.execute();
    QVERIFY(!dets.isEmpty());

    GraspPlanner gp;
    GraspPlanner::Config gc; gc.minScore = 0.0; gp.setConfig(gc);
    auto grasps = gp.plan(dets.first());

    TrajectoryPlanner tp;
    auto plan = tp.planPickPlace(robot.currentPose(), grasps.first(), {200, 0, 400, 0, 0, 0});

    // Execution should fail
    QVERIFY(!TrajectoryPlanner::execute(&robot, plan));
}

void IntegrationPickCycleTest::testCameraDisconnectDuringCycle() {
    SimCameraDriver cam;
    // NOT connected

    VisionPipeline vision;
    vision.setCameraDriver(&cam);
    vision.setMinConfidence(0.1f);

    QSignalSpy errorSpy(&vision, &VisionPipeline::pipelineError);
    auto dets = vision.execute();

    QVERIFY(dets.isEmpty());
    // Two errors: "CaptureFailed" from captureAndPreprocess, "CaptureEmpty" from execute
    QCOMPARE(errorSpy.count(), 2);
    QCOMPARE(errorSpy.at(0).first().toString(), QString("CaptureFailed"));
    QCOMPARE(errorSpy.at(1).first().toString(), QString("CaptureEmpty"));
}

void IntegrationPickCycleTest::testEmptySceneTriggersAlert() {
    DeviceManager mgr;
    auto cam = std::make_unique<SimCameraDriver>("cam-01");
    cam->connect({});
    cam->setObjectCount(0); // empty scene
    mgr.registerDevice("cam-01", std::move(cam));

    VisionPipeline vision;
    vision.setCameraDriver(mgr.camera("cam-01"));
    vision.setMinConfidence(0.5f);
    auto dets = vision.execute();

    QVERIFY(dets.isEmpty());
}

void IntegrationPickCycleTest::testRecoveryAfterGraspFailure() {
    RecoveryManager recovery;
    recovery.registerDefaults();

    WorkcellController ctrl;
    ctrl.startTask();

    // Simulate grasp failure
    ctrl.recordPickResult(false);
    QCOMPARE(ctrl.stats().failureCount, 1);

    // Recovery manager says retry
    QVERIFY(recovery.handleError("GraspFailed", &ctrl));
    QCOMPARE(ctrl.state(), WorkcellController::State::Running);
}

void IntegrationPickCycleTest::testRecoveryEStopOnCommLoss() {
    RecoveryManager recovery;
    recovery.registerDefaults();

    WorkcellController ctrl;
    ctrl.startTask();

    // Exhaust RobotCommLost retries (5)
    for (int i = 0; i < 5; ++i)
        recovery.handleError("RobotCommLost", &ctrl);

    // 6th call triggers EStop
    QVERIFY(!recovery.handleError("RobotCommLost", &ctrl));
    QCOMPARE(ctrl.state(), WorkcellController::State::Fault);

    // Recover and verify can restart
    QVERIFY(ctrl.recoverFromFault());
    QVERIFY(ctrl.startTask());
    QCOMPARE(ctrl.state(), WorkcellController::State::Running);
}

void IntegrationPickCycleTest::testMultipleCyclesStable() {
    SimCameraDriver cam;
    cam.connect({});
    cam.setObjectCount(3);

    SimRobotDriver robot;
    robot.connect({});

    WorkcellController ctrl;
    ctrl.startTask();

    GraspPlanner gp;
    GraspPlanner::Config gc; gc.minScore = 0.0; gp.setConfig(gc);
    TrajectoryPlanner tp;

    for (int cycle = 0; cycle < 10; ++cycle) {
        VisionPipeline vision;
        vision.setCameraDriver(&cam);
        vision.setMinConfidence(0.1f);
        auto dets = vision.execute();

        if (dets.isEmpty()) {
            ctrl.recordPickResult(false);
            continue;
        }

        auto grasps = gp.plan(dets.first());
        if (grasps.isEmpty()) {
            ctrl.recordPickResult(false);
            continue;
        }

        auto plan = tp.planPickPlace(robot.currentPose(), grasps.first(), {300, 0, 400, 0, 0, 0});
        bool ok = TrajectoryPlanner::execute(&robot, plan);
        ctrl.recordPickResult(ok);
    }

    auto stats = ctrl.stats();
    QCOMPARE(stats.totalAttempts, 10);
    QVERIFY(stats.successRate() > 0.0); // at least some should succeed
}

void IntegrationPickCycleTest::testWorkflowEngineWithController() {
    WorkcellController ctrl;
    ctrl.startTask();

    WorkflowDefinition def;
    def.id = "integration-test";
    def.name = "Integration";
    def.startNodeId = "capture";

    WorkflowNode capture; capture.id = "capture"; capture.type = "Capture"; capture.nextNodes = {"detect"};
    WorkflowNode detect; detect.id = "detect"; detect.type = "Detect"; detect.nextNodes = {"pick"};
    WorkflowNode pick; pick.id = "pick"; pick.type = "Pick"; pick.nextNodes = {"place"};
    WorkflowNode place; place.id = "place"; place.type = "Place";

    def.nodes.insert("capture", capture);
    def.nodes.insert("detect", detect);
    def.nodes.insert("pick", pick);
    def.nodes.insert("place", place);

    WorkflowEngine engine;
    engine.setWorkcellController(&ctrl);
    engine.loadWorkflow(def);

    QSignalSpy finishSpy(&engine, &WorkflowEngine::workflowFinished);
    engine.start();

    QCOMPARE(finishSpy.count(), 1);
    QCOMPARE(finishSpy.first().first().toBool(), true);
    QCOMPARE(engine.status(), WorkflowEngine::Status::Completed);
}

void IntegrationPickCycleTest::testAutoModeFullSetup() {
    DeviceManager mgr;
    mgr.registerDevice("cam", std::make_unique<SimCameraDriver>("cam"));
    mgr.registerDevice("bot", std::make_unique<SimRobotDriver>("bot"));

    mgr.camera("cam")->connect({});
    mgr.robot("bot")->connect({});

    WorkcellController ctrl;
    ctrl.setDeviceManager(&mgr);
    ctrl.bindCamera("cam");
    ctrl.bindRobot("bot");

    QString error;
    QVERIFY(ctrl.startAutoMode(&error));
    QVERIFY(ctrl.isAutoMode());
    QCOMPARE(ctrl.state(), WorkcellController::State::Running);

    // Stop auto mode
    ctrl.stopAutoMode();
    QVERIFY(!ctrl.isAutoMode());
    QCOMPARE(ctrl.state(), WorkcellController::State::Running); // still running

    // Full stop
    ctrl.stopTask();
    QCOMPARE(ctrl.state(), WorkcellController::State::Stopped);
}

void IntegrationPickCycleTest::testDeviceReplacementMidSession() {
    DeviceManager mgr;
    mgr.registerDevice("cam", std::make_unique<SimCameraDriver>("cam-v1"));
    mgr.camera("cam")->connect({});

    // Use first camera
    VisionPipeline vision;
    vision.setCameraDriver(mgr.camera("cam"));
    vision.setMinConfidence(0.1f);
    auto r1 = vision.execute();

    // Replace camera (simulate hardware swap)
    mgr.removeDevice("cam");
    auto newCam = std::make_unique<SimCameraDriver>("cam-v2");
    newCam->connect({});
    newCam->setObjectCount(5);
    mgr.registerDevice("cam", std::move(newCam));

    // Use replacement camera
    vision.setCameraDriver(mgr.camera("cam"));
    auto r2 = vision.execute();

    // Both should work without crash
    Q_UNUSED(r1); Q_UNUSED(r2);
    QVERIFY(mgr.camera("cam")->deviceId() == "cam-v2");
}

QTEST_MAIN(IntegrationPickCycleTest)
#include "test_integration_pickcycle.moc"
