#include <QtTest/QtTest>

#include "vision/visionpipeline.h"
#include "device/sim/simcameradriver.h"

class VisionPipelineTest : public QObject {
    Q_OBJECT

private slots:
    // === 正常流程 ===
    void testExecuteWithSimCamera();
    void testExecuteMultipleObjects();
    void testExecuteSingleObject();
    void testResultsHaveValidPose();
    void testResultsSortedByConfidence();
    void testSignalsEmitted();

    // === 异常流程（设备故障/数据异常） ===
    void testExecuteNoCameraFails();
    void testCameraDisconnectedDuringPipeline();
    void testEmptyScene();
    void testSceneWithOnlyBackgroundNoise();

    // === 后处理：过滤与排序 ===
    void testPostProcessFiltering();
    void testPostProcessSorting();
    void testPostProcessAllBelowThreshold();
    void testPostProcessAllAboveThreshold();
    void testPostProcessEmptyInput();
    void testPostProcessSingleResult();
    void testPostProcessBoundaryConfidence();
    void testPostProcessZeroThreshold();
    void testPostProcessMaxThreshold();

    // === 配置边界 ===
    void testMinConfidenceZero();
    void testMinConfidenceOne();
    void testMinConfidenceNegative();
    void testChangeConfigBetweenRuns();

    // === 高噪声/恶劣条件（模拟遮挡/反光/干扰） ===
    void testHighNoiseStillDetects();

    // === 多次连续执行（稳定性） ===
    void testConsecutiveExecutions();
    void testPipelineReuse();

    // === 分步调试接口 ===
    void testCaptureAndPreprocessSeparately();
    void testDetectWithEmptyCloud();
    void testDetectWithValidCloud();
};

// === 正常流程 ===

void VisionPipelineTest::testExecuteWithSimCamera() {
    SimCameraDriver camera("test-cam");
    camera.connect({});
    camera.setObjectCount(2);

    VisionPipeline pipeline;
    pipeline.setCameraDriver(&camera);
    pipeline.setMinConfidence(0.3f);

    QSignalSpy startSpy(&pipeline, &VisionPipeline::pipelineStarted);
    QSignalSpy finishSpy(&pipeline, &VisionPipeline::pipelineFinished);

    auto results = pipeline.execute();

    QCOMPARE(startSpy.count(), 1);
    QCOMPARE(finishSpy.count(), 1);
    QVERIFY(results.size() >= 1);

    for (const auto& det : results)
        QVERIFY(det.confidence >= 0.3f);
}

void VisionPipelineTest::testExecuteMultipleObjects() {
    SimCameraDriver camera;
    camera.connect({});
    camera.setObjectCount(10);

    VisionPipeline pipeline;
    pipeline.setCameraDriver(&camera);
    pipeline.setMinConfidence(0.1f);

    auto results = pipeline.execute();
    QVERIFY(results.size() >= 1);
}

void VisionPipelineTest::testExecuteSingleObject() {
    SimCameraDriver camera;
    camera.connect({});
    camera.setObjectCount(1);
    camera.setNoiseLevel(0.001);

    VisionPipeline pipeline;
    pipeline.setCameraDriver(&camera);
    pipeline.setMinConfidence(0.1f);

    // With a single random object, it may land near the z-filter boundary,
    // so we retry a few times to account for randomness.
    bool detected = false;
    for (int attempt = 0; attempt < 5; ++attempt) {
        auto results = pipeline.execute();
        if (!results.isEmpty()) {
            detected = true;
            break;
        }
    }
    QVERIFY(detected);
}

void VisionPipelineTest::testResultsHaveValidPose() {
    SimCameraDriver camera;
    camera.connect({});
    camera.setObjectCount(3);

    VisionPipeline pipeline;
    pipeline.setCameraDriver(&camera);
    pipeline.setMinConfidence(0.1f);

    auto results = pipeline.execute();
    for (const auto& det : results) {
        QVERIFY(det.pose6D.z >= 100.0 && det.pose6D.z <= 800.0);
    }
}

void VisionPipelineTest::testResultsSortedByConfidence() {
    SimCameraDriver camera;
    camera.connect({});
    camera.setObjectCount(5);

    VisionPipeline pipeline;
    pipeline.setCameraDriver(&camera);
    pipeline.setMinConfidence(0.1f);

    auto results = pipeline.execute();
    for (int i = 1; i < results.size(); ++i)
        QVERIFY(results[i - 1].confidence >= results[i].confidence);
}

void VisionPipelineTest::testSignalsEmitted() {
    SimCameraDriver camera;
    camera.connect({});
    camera.setObjectCount(1);

    VisionPipeline pipeline;
    pipeline.setCameraDriver(&camera);

    QSignalSpy startSpy(&pipeline, &VisionPipeline::pipelineStarted);
    QSignalSpy finishSpy(&pipeline, &VisionPipeline::pipelineFinished);
    QSignalSpy errorSpy(&pipeline, &VisionPipeline::pipelineError);

    pipeline.execute();

    QCOMPARE(startSpy.count(), 1);
    QCOMPARE(finishSpy.count(), 1);
    QCOMPARE(errorSpy.count(), 0);
}

// === 异常流程 ===

void VisionPipelineTest::testExecuteNoCameraFails() {
    VisionPipeline pipeline;

    QSignalSpy errorSpy(&pipeline, &VisionPipeline::pipelineError);
    auto results = pipeline.execute();

    QVERIFY(results.isEmpty());
    QVERIFY(errorSpy.count() > 0);
    QCOMPARE(errorSpy.first().first().toString(), QString("NoCameraConfigured"));
}

void VisionPipelineTest::testCameraDisconnectedDuringPipeline() {
    SimCameraDriver camera;
    // Camera NOT connected

    VisionPipeline pipeline;
    pipeline.setCameraDriver(&camera);

    QSignalSpy errorSpy(&pipeline, &VisionPipeline::pipelineError);
    auto results = pipeline.execute();

    QVERIFY(results.isEmpty());
    // Two errors: "CaptureFailed" from captureAndPreprocess, "CaptureEmpty" from execute
    QCOMPARE(errorSpy.count(), 2);
    QCOMPARE(errorSpy.at(0).first().toString(), QString("CaptureFailed"));
    QCOMPARE(errorSpy.at(1).first().toString(), QString("CaptureEmpty"));
}

void VisionPipelineTest::testEmptyScene() {
    SimCameraDriver camera;
    camera.connect({});
    camera.setObjectCount(0);

    VisionPipeline pipeline;
    pipeline.setCameraDriver(&camera);
    pipeline.setMinConfidence(0.5f);

    auto results = pipeline.execute();
    QVERIFY(results.isEmpty());
}

void VisionPipelineTest::testSceneWithOnlyBackgroundNoise() {
    SimCameraDriver camera;
    camera.connect({});
    camera.setObjectCount(0);
    camera.setNoiseLevel(0.0);

    VisionPipeline pipeline;
    pipeline.setCameraDriver(&camera);
    pipeline.setMinConfidence(0.5f);

    auto results = pipeline.execute();
    QVERIFY(results.isEmpty());
}

// === 后处理 ===

void VisionPipelineTest::testPostProcessFiltering() {
    QVector<DetectionResult> raw;
    DetectionResult d1; d1.confidence = 0.9f; d1.objectClass = "part_a";
    DetectionResult d2; d2.confidence = 0.3f; d2.objectClass = "part_b";
    DetectionResult d3; d3.confidence = 0.1f; d3.objectClass = "noise";
    raw << d1 << d2 << d3;

    auto filtered = VisionPipeline::postProcess(raw, 0.5f);
    QCOMPARE(filtered.size(), 1);
    QCOMPARE(filtered.first().objectClass, QString("part_a"));
}

void VisionPipelineTest::testPostProcessSorting() {
    QVector<DetectionResult> raw;
    DetectionResult d1; d1.confidence = 0.6f;
    DetectionResult d2; d2.confidence = 0.9f;
    DetectionResult d3; d3.confidence = 0.7f;
    raw << d1 << d2 << d3;

    auto sorted = VisionPipeline::postProcess(raw, 0.5f);
    QCOMPARE(sorted.size(), 3);
    QVERIFY(sorted[0].confidence >= sorted[1].confidence);
    QVERIFY(sorted[1].confidence >= sorted[2].confidence);
}

void VisionPipelineTest::testPostProcessAllBelowThreshold() {
    QVector<DetectionResult> raw;
    DetectionResult d1; d1.confidence = 0.1f;
    DetectionResult d2; d2.confidence = 0.2f;
    raw << d1 << d2;
    QVERIFY(VisionPipeline::postProcess(raw, 0.5f).isEmpty());
}

void VisionPipelineTest::testPostProcessAllAboveThreshold() {
    QVector<DetectionResult> raw;
    DetectionResult d1; d1.confidence = 0.8f;
    DetectionResult d2; d2.confidence = 0.9f;
    raw << d1 << d2;
    QCOMPARE(VisionPipeline::postProcess(raw, 0.5f).size(), 2);
}

void VisionPipelineTest::testPostProcessEmptyInput() {
    QVERIFY(VisionPipeline::postProcess({}, 0.5f).isEmpty());
}

void VisionPipelineTest::testPostProcessSingleResult() {
    QVector<DetectionResult> raw;
    DetectionResult d1; d1.confidence = 0.8f; d1.objectClass = "solo";
    raw << d1;
    auto filtered = VisionPipeline::postProcess(raw, 0.5f);
    QCOMPARE(filtered.size(), 1);
    QCOMPARE(filtered.first().objectClass, QString("solo"));
}

void VisionPipelineTest::testPostProcessBoundaryConfidence() {
    QVector<DetectionResult> raw;
    DetectionResult d1; d1.confidence = 0.5f; // exactly at threshold
    raw << d1;
    QCOMPARE(VisionPipeline::postProcess(raw, 0.5f).size(), 1);

    QVector<DetectionResult> raw2;
    DetectionResult d2; d2.confidence = 0.4999f; // just below
    raw2 << d2;
    QVERIFY(VisionPipeline::postProcess(raw2, 0.5f).isEmpty());
}

void VisionPipelineTest::testPostProcessZeroThreshold() {
    QVector<DetectionResult> raw;
    DetectionResult d1; d1.confidence = 0.001f;
    DetectionResult d2; d2.confidence = 0.0f;
    raw << d1 << d2;
    QCOMPARE(VisionPipeline::postProcess(raw, 0.0f).size(), 2);
}

void VisionPipelineTest::testPostProcessMaxThreshold() {
    QVector<DetectionResult> raw;
    DetectionResult d1; d1.confidence = 0.99f;
    DetectionResult d2; d2.confidence = 1.0f;
    raw << d1 << d2;
    QCOMPARE(VisionPipeline::postProcess(raw, 1.0f).size(), 1);
}

// === 配置边界 ===

void VisionPipelineTest::testMinConfidenceZero() {
    SimCameraDriver camera;
    camera.connect({});
    camera.setObjectCount(3);

    VisionPipeline pipeline;
    pipeline.setCameraDriver(&camera);
    pipeline.setMinConfidence(0.0f);

    auto results = pipeline.execute();
    QVERIFY(results.size() >= 1);
}

void VisionPipelineTest::testMinConfidenceOne() {
    SimCameraDriver camera;
    camera.connect({});
    camera.setObjectCount(3);

    VisionPipeline pipeline;
    pipeline.setCameraDriver(&camera);
    pipeline.setMinConfidence(1.0f);

    auto results = pipeline.execute();
    for (const auto& det : results)
        QCOMPARE(det.confidence, 1.0f);
}

void VisionPipelineTest::testMinConfidenceNegative() {
    QVector<DetectionResult> raw;
    DetectionResult d1; d1.confidence = 0.0f;
    raw << d1;
    QCOMPARE(VisionPipeline::postProcess(raw, -1.0f).size(), 1);
}

void VisionPipelineTest::testChangeConfigBetweenRuns() {
    SimCameraDriver camera;
    camera.connect({});
    camera.setObjectCount(3);

    VisionPipeline pipeline;
    pipeline.setCameraDriver(&camera);

    pipeline.setMinConfidence(1.0f);
    auto strict = pipeline.execute();

    pipeline.setMinConfidence(0.01f);
    auto loose = pipeline.execute();

    QVERIFY(loose.size() >= strict.size());
}

// === 高噪声 ===

void VisionPipelineTest::testHighNoiseStillDetects() {
    SimCameraDriver camera;
    camera.connect({});
    camera.setObjectCount(5);
    camera.setNoiseLevel(5.0);

    VisionPipeline pipeline;
    pipeline.setCameraDriver(&camera);
    pipeline.setMinConfidence(0.1f);

    auto results = pipeline.execute();
    QVERIFY(results.size() >= 1);
}

// === 稳定性 ===

void VisionPipelineTest::testConsecutiveExecutions() {
    SimCameraDriver camera;
    camera.connect({});
    camera.setObjectCount(2);

    VisionPipeline pipeline;
    pipeline.setCameraDriver(&camera);
    pipeline.setMinConfidence(0.1f);

    for (int i = 0; i < 20; ++i) {
        auto results = pipeline.execute();
        Q_UNUSED(results);
    }
}

void VisionPipelineTest::testPipelineReuse() {
    VisionPipeline pipeline;

    SimCameraDriver cam1("cam-1");
    cam1.connect({});
    cam1.setObjectCount(1);
    pipeline.setCameraDriver(&cam1);
    auto r1 = pipeline.execute();

    SimCameraDriver cam2("cam-2");
    cam2.connect({});
    cam2.setObjectCount(5);
    pipeline.setCameraDriver(&cam2);
    auto r2 = pipeline.execute();

    Q_UNUSED(r1); Q_UNUSED(r2);
}

// === 分步调试 ===

void VisionPipelineTest::testCaptureAndPreprocessSeparately() {
    SimCameraDriver camera;
    camera.connect({});
    camera.setObjectCount(2);

    VisionPipeline pipeline;
    pipeline.setCameraDriver(&camera);

    auto cloud = pipeline.captureAndPreprocess();
    QVERIFY(!cloud.empty());
    QVERIFY(cloud.pointCount() > 0);
}

void VisionPipelineTest::testDetectWithEmptyCloud() {
    VisionPipeline pipeline;
    PointCloud empty;
    auto results = pipeline.detect(empty);
    QVERIFY(results.isEmpty());
}

void VisionPipelineTest::testDetectWithValidCloud() {
    SimCameraDriver camera;
    camera.connect({});
    camera.setObjectCount(3);

    VisionPipeline pipeline;
    pipeline.setCameraDriver(&camera);

    auto cloud = pipeline.captureAndPreprocess();
    auto results = pipeline.detect(cloud);
    Q_UNUSED(results);
}

QTEST_MAIN(VisionPipelineTest)
#include "test_visionpipeline.moc"
