#include <QtTest/QtTest>
#include <QJsonDocument>
#include <QJsonObject>

#include "orchestration/workflowengine.h"
#include "workcellcontroller.h"

class WorkflowEngineTest : public QObject {
    Q_OBJECT

private slots:
    void testLoadAndStartLinearWorkflow();
    void testConditionalBranch();
    void testPauseAndResume();
    void testAbort();
    void testParseFromJson();
};

void WorkflowEngineTest::testLoadAndStartLinearWorkflow() {
    // Build a simple linear workflow: Capture -> Detect -> Pick
    WorkflowDefinition def;
    def.id = "test-linear";
    def.name = "Linear Test";
    def.startNodeId = "capture";

    WorkflowNode capture;
    capture.id = "capture";
    capture.type = "Capture";
    capture.nextNodes = {"detect"};
    def.nodes.insert("capture", capture);

    WorkflowNode detect;
    detect.id = "detect";
    detect.type = "Detect";
    detect.nextNodes = {"pick"};
    def.nodes.insert("detect", detect);

    WorkflowNode pick;
    pick.id = "pick";
    pick.type = "Pick";
    // No nextNodes → workflow should complete
    def.nodes.insert("pick", pick);

    WorkflowEngine engine;
    WorkcellController controller;
    controller.startTask();

    engine.setWorkcellController(&controller);
    engine.loadWorkflow(def);

    QSignalSpy finishSpy(&engine, &WorkflowEngine::workflowFinished);
    QSignalSpy nodeSpy(&engine, &WorkflowEngine::nodeStarted);

    engine.start();

    QCOMPARE(finishSpy.count(), 1);
    QCOMPARE(finishSpy.first().first().toBool(), true);
    QCOMPARE(nodeSpy.count(), 3); // capture, detect, pick

    QCOMPARE(engine.status(), WorkflowEngine::Status::Completed);
}

void WorkflowEngineTest::testConditionalBranch() {
    WorkflowDefinition def;
    def.id = "test-branch";
    def.name = "Branch Test";
    def.startNodeId = "branch_node";

    WorkflowNode branch;
    branch.id = "branch_node";
    branch.type = "Branch";
    branch.parameters.insert("condition", "noObject");
    branch.nextNodes = {"pick"};    // default path
    branch.conditionalNext.insert("noObject", {"wait"});
    def.nodes.insert("branch_node", branch);

    WorkflowNode pick;
    pick.id = "pick";
    pick.type = "Pick";
    def.nodes.insert("pick", pick);

    WorkflowNode wait;
    wait.id = "wait";
    wait.type = "Wait";
    def.nodes.insert("wait", wait);

    WorkflowEngine engine;
    engine.loadWorkflow(def);

    QSignalSpy nodeSpy(&engine, &WorkflowEngine::nodeStarted);
    engine.start();

    // Should have gone: branch_node -> wait (conditional path)
    QCOMPARE(nodeSpy.count(), 2);
    QCOMPARE(nodeSpy.at(0).at(0).toString(), QString("branch_node"));
    QCOMPARE(nodeSpy.at(1).at(0).toString(), QString("wait"));
}

void WorkflowEngineTest::testPauseAndResume() {
    WorkflowDefinition def;
    def.id = "test-pause";
    def.name = "Pause Test";
    def.startNodeId = "step1";

    WorkflowNode step1;
    step1.id = "step1";
    step1.type = "Capture";
    def.nodes.insert("step1", step1);

    WorkflowEngine engine;
    engine.loadWorkflow(def);

    engine.start();
    QCOMPARE(engine.status(), WorkflowEngine::Status::Completed);

    // Test pause on a fresh workflow
    WorkflowEngine engine2;
    engine2.loadWorkflow(def);
    engine2.start();

    engine2.pause();
    QCOMPARE(engine2.status(), WorkflowEngine::Status::Completed); // Already ran through
}

void WorkflowEngineTest::testAbort() {
    WorkflowDefinition def;
    def.id = "test-abort";
    def.name = "Abort Test";
    def.startNodeId = "step1";

    WorkflowNode step1;
    step1.id = "step1";
    step1.type = "Capture";
    def.nodes.insert("step1", step1);

    WorkflowEngine engine;
    engine.loadWorkflow(def);

    QSignalSpy finishSpy(&engine, &WorkflowEngine::workflowFinished);
    engine.abort();

    QCOMPARE(finishSpy.count(), 1);
    QCOMPARE(finishSpy.first().first().toBool(), false);
    QCOMPARE(engine.status(), WorkflowEngine::Status::Idle);
}

void WorkflowEngineTest::testParseFromJson() {
    QString jsonStr = R"({
        "id": "bin-picking",
        "name": "Bin Picking",
        "startNodeId": "capture",
        "nodes": {
            "capture": {
                "type": "Capture",
                "parameters": { "exposureTime": 15.0 },
                "nextNodes": ["detect"]
            },
            "detect": {
                "type": "Detect",
                "parameters": { "minConfidence": 0.7 },
                "nextNodes": ["pick"],
                "conditionalNext": { "noObject": ["wait"] }
            },
            "pick": {
                "type": "Pick",
                "nextNodes": []
            },
            "wait": {
                "type": "Wait",
                "nextNodes": ["capture"]
            }
        }
    })";

    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
    QVERIFY(!doc.isNull());

    auto def = WorkflowEngine::fromJson(doc.object());
    QCOMPARE(def.id, QString("bin-picking"));
    QCOMPARE(def.name, QString("Bin Picking"));
    QCOMPARE(def.startNodeId, QString("capture"));
    QCOMPARE(def.nodes.size(), 4);

    QVERIFY(def.nodes.contains("detect"));
    const auto& detectNode = def.nodes["detect"];
    QCOMPARE(detectNode.type, QString("Detect"));
    QVERIFY(detectNode.conditionalNext.contains("noObject"));
    QCOMPARE(detectNode.conditionalNext["noObject"].first(), QString("wait"));
}

QTEST_MAIN(WorkflowEngineTest)
#include "test_workflowengine.moc"
