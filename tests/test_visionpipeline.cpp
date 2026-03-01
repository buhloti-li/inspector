#include <QtTest/QtTest>

#include "vision/visionpipeline.h"
#include "device/sim/simcameradriver.h"

class VisionPipelineTest : public QObject {
    Q_OBJECT

private slots:
    void testExecuteWithSimCamera();
    void testExecuteNoCameraFails();
    void testPostProcessFiltering();
    void testPostProcessSorting();
    void testEmptyScene();
};

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
    // With 2 simulated objects, we should get at least 1 detection
    QVERIFY(results.size() >= 1);

    for (const auto& det : results) {
        QVERIFY(det.confidence >= 0.3f);
    }
}

void VisionPipelineTest::testExecuteNoCameraFails() {
    VisionPipeline pipeline;
    // No camera set

    QSignalSpy errorSpy(&pipeline, &VisionPipeline::pipelineError);

    auto results = pipeline.execute();
    QVERIFY(results.isEmpty());
    QVERIFY(errorSpy.count() > 0);
}

void VisionPipelineTest::testPostProcessFiltering() {
    QVector<DetectionResult> raw;

    DetectionResult d1;
    d1.confidence = 0.9f;
    d1.objectClass = "part_a";
    raw.append(d1);

    DetectionResult d2;
    d2.confidence = 0.3f;
    d2.objectClass = "part_b";
    raw.append(d2);

    DetectionResult d3;
    d3.confidence = 0.1f;
    d3.objectClass = "noise";
    raw.append(d3);

    auto filtered = VisionPipeline::postProcess(raw, 0.5f);
    QCOMPARE(filtered.size(), 1);
    QCOMPARE(filtered.first().objectClass, QString("part_a"));
}

void VisionPipelineTest::testPostProcessSorting() {
    QVector<DetectionResult> raw;

    DetectionResult d1;
    d1.confidence = 0.6f;
    raw.append(d1);

    DetectionResult d2;
    d2.confidence = 0.9f;
    raw.append(d2);

    DetectionResult d3;
    d3.confidence = 0.7f;
    raw.append(d3);

    auto sorted = VisionPipeline::postProcess(raw, 0.5f);
    QCOMPARE(sorted.size(), 3);
    QVERIFY(sorted[0].confidence >= sorted[1].confidence);
    QVERIFY(sorted[1].confidence >= sorted[2].confidence);
}

void VisionPipelineTest::testEmptyScene() {
    SimCameraDriver camera("test-cam");
    camera.connect({});
    camera.setObjectCount(0);

    VisionPipeline pipeline;
    pipeline.setCameraDriver(&camera);
    pipeline.setMinConfidence(0.5f);

    auto results = pipeline.execute();
    QVERIFY(results.isEmpty());
}

QTEST_MAIN(VisionPipelineTest)
#include "test_visionpipeline.moc"
