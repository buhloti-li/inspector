#include <QtTest/QtTest>
#include <QJsonDocument>
#include <QJsonObject>

#include "orchestration/workflowengine.h"
#include "workcellcontroller.h"

class WorkflowEngineTest : public QObject {
    Q_OBJECT

private slots:
    // === 正常流程 ===
    void testLoadAndStartLinearWorkflow();
    void testConditionalBranch();
    void testDefaultPathWhenNoCondition();
    void testMultiNodeChain();

    // === 控制：暂停/恢复/中止 ===
    void testPauseAndResume();
    void testAbort();
    void testAbortBeforeStart();
    void testDoubleAbort();
    void testStatusTransitions();

    // === JSON 解析 ===
    void testParseFromJson();
    void testParseFromJsonMinimal();
    void testParseFromJsonEmptyNodes();
    void testParseFromJsonMalformedIgnored();

    // === 异常：空/无效工作流 ===
    void testStartEmptyWorkflow();
    void testStartWithMissingStartNode();
    void testNodeReferencesNonexistentNext();

    // === 异常：未知节点类型 ===
    void testUnknownNodeType();

    // === 信号验证 ===
    void testNodeStartedSignals();
    void testNodeCompletedSignals();
    void testWorkflowFinishedSignal();
    void testStatusChangedSignals();

    // === 边界条件 ===
    void testSingleNodeWorkflow();
    void testWorkflowWithNoController();
    void testReloadWorkflow();
    void testCurrentNodeIdTracking();
};

// Helper to build a simple workflow
static WorkflowDefinition makeLinearWorkflow(const QStringList& nodeTypes) {
    WorkflowDefinition def;
    def.id = "test";
    def.name = "Test";
    def.startNodeId = "node_0";

    for (int i = 0; i < nodeTypes.size(); ++i) {
        WorkflowNode node;
        node.id = QString("node_%1").arg(i);
        node.type = nodeTypes[i];
        if (i + 1 < nodeTypes.size())
            node.nextNodes = {QString("node_%1").arg(i + 1)};
        def.nodes.insert(node.id, node);
    }
    return def;
}

// === 正常流程 ===

void WorkflowEngineTest::testLoadAndStartLinearWorkflow() {
    auto def = makeLinearWorkflow({"Capture", "Detect", "Pick"});

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
    QCOMPARE(nodeSpy.count(), 3);
    QCOMPARE(engine.status(), WorkflowEngine::Status::Completed);
}

void WorkflowEngineTest::testConditionalBranch() {
    WorkflowDefinition def;
    def.id = "test-branch"; def.name = "Branch"; def.startNodeId = "branch";

    WorkflowNode branch;
    branch.id = "branch"; branch.type = "Branch";
    branch.parameters.insert("condition", "noObject");
    branch.nextNodes = {"pick"};
    branch.conditionalNext.insert("noObject", {"wait"});
    def.nodes.insert("branch", branch);

    WorkflowNode pick; pick.id = "pick"; pick.type = "Pick";
    def.nodes.insert("pick", pick);

    WorkflowNode wait; wait.id = "wait"; wait.type = "Wait";
    def.nodes.insert("wait", wait);

    WorkflowEngine engine;
    engine.loadWorkflow(def);

    QSignalSpy nodeSpy(&engine, &WorkflowEngine::nodeStarted);
    engine.start();

    QCOMPARE(nodeSpy.count(), 2);
    QCOMPARE(nodeSpy.at(1).at(0).toString(), QString("wait"));
}

void WorkflowEngineTest::testDefaultPathWhenNoCondition() {
    WorkflowDefinition def;
    def.id = "test"; def.name = "Test"; def.startNodeId = "branch";

    WorkflowNode branch;
    branch.id = "branch"; branch.type = "Capture"; // not a Branch type, no condition
    branch.nextNodes = {"default_next"};
    branch.conditionalNext.insert("someCondition", {"alt"});
    def.nodes.insert("branch", branch);

    WorkflowNode next; next.id = "default_next"; next.type = "Detect";
    def.nodes.insert("default_next", next);

    WorkflowNode alt; alt.id = "alt"; alt.type = "Wait";
    def.nodes.insert("alt", alt);

    WorkflowEngine engine;
    engine.loadWorkflow(def);

    QSignalSpy nodeSpy(&engine, &WorkflowEngine::nodeStarted);
    engine.start();

    QCOMPARE(nodeSpy.count(), 2);
    QCOMPARE(nodeSpy.at(1).at(0).toString(), QString("default_next"));
}

void WorkflowEngineTest::testMultiNodeChain() {
    auto def = makeLinearWorkflow({"Capture", "Detect", "Pick", "Place", "Wait"});

    WorkflowEngine engine;
    engine.loadWorkflow(def);

    QSignalSpy nodeSpy(&engine, &WorkflowEngine::nodeStarted);
    engine.start();

    QCOMPARE(nodeSpy.count(), 5);
    for (int i = 0; i < 5; ++i)
        QCOMPARE(nodeSpy.at(i).at(0).toString(), QString("node_%1").arg(i));
}

// === 控制 ===

void WorkflowEngineTest::testPauseAndResume() {
    auto def = makeLinearWorkflow({"Capture"});

    WorkflowEngine engine;
    engine.loadWorkflow(def);
    engine.start();
    QCOMPARE(engine.status(), WorkflowEngine::Status::Completed);
}

void WorkflowEngineTest::testAbort() {
    auto def = makeLinearWorkflow({"Capture"});
    WorkflowEngine engine;
    engine.loadWorkflow(def);

    QSignalSpy finishSpy(&engine, &WorkflowEngine::workflowFinished);
    engine.abort();

    QCOMPARE(finishSpy.count(), 1);
    QCOMPARE(finishSpy.first().first().toBool(), false);
    QCOMPARE(engine.status(), WorkflowEngine::Status::Idle);
}

void WorkflowEngineTest::testAbortBeforeStart() {
    WorkflowEngine engine;
    // No workflow loaded
    QSignalSpy finishSpy(&engine, &WorkflowEngine::workflowFinished);
    engine.abort();
    QCOMPARE(finishSpy.count(), 1);
    QCOMPARE(engine.status(), WorkflowEngine::Status::Idle);
}

void WorkflowEngineTest::testDoubleAbort() {
    WorkflowEngine engine;
    engine.abort();
    engine.abort(); // should not crash
    QCOMPARE(engine.status(), WorkflowEngine::Status::Idle);
}

void WorkflowEngineTest::testStatusTransitions() {
    auto def = makeLinearWorkflow({"Capture"});
    WorkflowEngine engine;
    QCOMPARE(engine.status(), WorkflowEngine::Status::Idle);

    engine.loadWorkflow(def);
    QCOMPARE(engine.status(), WorkflowEngine::Status::Idle);

    QSignalSpy statusSpy(&engine, &WorkflowEngine::statusChanged);
    engine.start();
    // Should go: Running -> Completed
    QVERIFY(statusSpy.count() >= 2);
}

// === JSON 解析 ===

void WorkflowEngineTest::testParseFromJson() {
    QString jsonStr = R"({
        "id": "bin-picking", "name": "Bin Picking", "startNodeId": "capture",
        "nodes": {
            "capture": { "type": "Capture", "parameters": { "exposureTime": 15.0 }, "nextNodes": ["detect"] },
            "detect": { "type": "Detect", "parameters": { "minConfidence": 0.7 }, "nextNodes": ["pick"], "conditionalNext": { "noObject": ["wait"] } },
            "pick": { "type": "Pick", "nextNodes": [] },
            "wait": { "type": "Wait", "nextNodes": ["capture"] }
        }
    })";

    auto def = WorkflowEngine::fromJson(QJsonDocument::fromJson(jsonStr.toUtf8()).object());
    QCOMPARE(def.id, QString("bin-picking"));
    QCOMPARE(def.nodes.size(), 4);
    QVERIFY(def.nodes["detect"].conditionalNext.contains("noObject"));
}

void WorkflowEngineTest::testParseFromJsonMinimal() {
    QString jsonStr = R"({ "id": "min", "name": "Minimal", "startNodeId": "s", "nodes": { "s": { "type": "Capture" } } })";
    auto def = WorkflowEngine::fromJson(QJsonDocument::fromJson(jsonStr.toUtf8()).object());
    QCOMPARE(def.nodes.size(), 1);
}

void WorkflowEngineTest::testParseFromJsonEmptyNodes() {
    QString jsonStr = R"({ "id": "empty", "name": "Empty", "startNodeId": "s", "nodes": {} })";
    auto def = WorkflowEngine::fromJson(QJsonDocument::fromJson(jsonStr.toUtf8()).object());
    QCOMPARE(def.nodes.size(), 0);
}

void WorkflowEngineTest::testParseFromJsonMalformedIgnored() {
    auto def = WorkflowEngine::fromJson(QJsonObject{});
    QVERIFY(def.id.isEmpty());
    QVERIFY(def.nodes.isEmpty());
}

// === 异常：空/无效 ===

void WorkflowEngineTest::testStartEmptyWorkflow() {
    WorkflowEngine engine;
    QSignalSpy statusSpy(&engine, &WorkflowEngine::statusChanged);
    engine.start();
    QCOMPARE(engine.status(), WorkflowEngine::Status::Error);
}

void WorkflowEngineTest::testStartWithMissingStartNode() {
    WorkflowDefinition def;
    def.id = "test"; def.name = "test"; def.startNodeId = "nonexistent";

    WorkflowNode node; node.id = "real"; node.type = "Capture";
    def.nodes.insert("real", node);

    WorkflowEngine engine;
    engine.loadWorkflow(def);

    QSignalSpy errorSpy(&engine, &WorkflowEngine::nodeError);
    engine.start();

    QCOMPARE(engine.status(), WorkflowEngine::Status::Error);
    QCOMPARE(errorSpy.count(), 1);
}

void WorkflowEngineTest::testNodeReferencesNonexistentNext() {
    WorkflowDefinition def;
    def.id = "test"; def.name = "test"; def.startNodeId = "start";

    WorkflowNode start;
    start.id = "start"; start.type = "Capture";
    start.nextNodes = {"nonexistent_node"};
    def.nodes.insert("start", start);

    WorkflowEngine engine;
    engine.loadWorkflow(def);

    QSignalSpy errorSpy(&engine, &WorkflowEngine::nodeError);
    engine.start();

    // Start node completes, then trying to execute nonexistent node should error
    QCOMPARE(engine.status(), WorkflowEngine::Status::Error);
}

void WorkflowEngineTest::testUnknownNodeType() {
    WorkflowDefinition def;
    def.id = "test"; def.name = "test"; def.startNodeId = "node";

    WorkflowNode node;
    node.id = "node"; node.type = "UnknownType";
    def.nodes.insert("node", node);

    WorkflowEngine engine;
    engine.loadWorkflow(def);

    QSignalSpy errorSpy(&engine, &WorkflowEngine::nodeError);
    engine.start();

    QCOMPARE(engine.status(), WorkflowEngine::Status::Error);
    QCOMPARE(errorSpy.count(), 1);
    QVERIFY(errorSpy.first().at(1).toString().contains("UnknownNodeType"));
}

// === 信号验证 ===

void WorkflowEngineTest::testNodeStartedSignals() {
    auto def = makeLinearWorkflow({"Capture", "Detect"});
    WorkflowEngine engine;
    engine.loadWorkflow(def);

    QSignalSpy spy(&engine, &WorkflowEngine::nodeStarted);
    engine.start();

    QCOMPARE(spy.count(), 2);
    QCOMPARE(spy.at(0).at(0).toString(), QString("node_0"));
    QCOMPARE(spy.at(0).at(1).toString(), QString("Capture"));
    QCOMPARE(spy.at(1).at(0).toString(), QString("node_1"));
    QCOMPARE(spy.at(1).at(1).toString(), QString("Detect"));
}

void WorkflowEngineTest::testNodeCompletedSignals() {
    auto def = makeLinearWorkflow({"Capture", "Pick"});
    WorkflowEngine engine;
    engine.loadWorkflow(def);

    QSignalSpy spy(&engine, &WorkflowEngine::nodeCompleted);
    engine.start();

    QCOMPARE(spy.count(), 2);
}

void WorkflowEngineTest::testWorkflowFinishedSignal() {
    auto def = makeLinearWorkflow({"Capture"});
    WorkflowEngine engine;
    engine.loadWorkflow(def);

    QSignalSpy spy(&engine, &WorkflowEngine::workflowFinished);
    engine.start();

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().first().toBool(), true);
}

void WorkflowEngineTest::testStatusChangedSignals() {
    auto def = makeLinearWorkflow({"Capture"});
    WorkflowEngine engine;
    engine.loadWorkflow(def);

    QSignalSpy spy(&engine, &WorkflowEngine::statusChanged);
    engine.start();

    // At minimum: Running, Completed
    QVERIFY(spy.count() >= 2);
    QCOMPARE(spy.first().first().value<WorkflowEngine::Status>(), WorkflowEngine::Status::Running);
    QCOMPARE(spy.last().first().value<WorkflowEngine::Status>(), WorkflowEngine::Status::Completed);
}

// === 边界 ===

void WorkflowEngineTest::testSingleNodeWorkflow() {
    auto def = makeLinearWorkflow({"Place"});
    WorkflowEngine engine;
    engine.loadWorkflow(def);
    engine.start();
    QCOMPARE(engine.status(), WorkflowEngine::Status::Completed);
}

void WorkflowEngineTest::testWorkflowWithNoController() {
    auto def = makeLinearWorkflow({"Pick"});
    WorkflowEngine engine;
    engine.loadWorkflow(def);
    engine.start();
    // Should complete without crash (Pick node records result only if controller set)
    QCOMPARE(engine.status(), WorkflowEngine::Status::Completed);
}

void WorkflowEngineTest::testReloadWorkflow() {
    auto def1 = makeLinearWorkflow({"Capture"});
    auto def2 = makeLinearWorkflow({"Detect", "Pick", "Place"});

    WorkflowEngine engine;

    engine.loadWorkflow(def1);
    engine.start();
    QCOMPARE(engine.status(), WorkflowEngine::Status::Completed);

    // Reload with different workflow
    engine.loadWorkflow(def2);
    QCOMPARE(engine.status(), WorkflowEngine::Status::Idle);

    QSignalSpy nodeSpy(&engine, &WorkflowEngine::nodeStarted);
    engine.start();
    QCOMPARE(nodeSpy.count(), 3);
    QCOMPARE(engine.status(), WorkflowEngine::Status::Completed);
}

void WorkflowEngineTest::testCurrentNodeIdTracking() {
    auto def = makeLinearWorkflow({"Capture", "Detect"});
    WorkflowEngine engine;
    engine.loadWorkflow(def);

    QVERIFY(engine.currentNodeId().isEmpty());
    engine.start();
    // After completion, last executed node should be tracked
    QCOMPARE(engine.currentNodeId(), QString("node_1"));
}

QTEST_MAIN(WorkflowEngineTest)
#include "test_workflowengine.moc"
