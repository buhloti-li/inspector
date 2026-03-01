#include <QtTest/QtTest>

#include "device/sim/simcameradriver.h"
#include "device/sim/simrobotdriver.h"

class SimDriversTest : public QObject {
    Q_OBJECT

private slots:
    // === SimCameraDriver ===
    void testCameraInitialState();
    void testCameraConnectDisconnect();
    void testCameraDoubleConnect();
    void testCameraDoubleDisconnect();
    void testCameraCaptureBeforeConnect();
    void testCameraCaptureAfterConnect();
    void testCameraCaptureWithROI();
    void testCameraPointCloudNotEmpty();
    void testCameraColorImageReturned();
    void testCameraZeroObjects();
    void testCameraSingleObject();
    void testCameraManyObjects();
    void testCameraHighNoise();
    void testCameraZeroNoise();
    void testCameraDeviceTypeAndId();
    void testCameraReconnectAndCapture();
    void testCameraConsecutiveCaptures();

    // === SimRobotDriver ===
    void testRobotInitialState();
    void testRobotConnectDisconnect();
    void testRobotDoubleConnect();
    void testRobotMoveBeforeConnect();
    void testRobotMoveTo();
    void testRobotMoveJoint();
    void testRobotStopMotion();
    void testRobotDigitalOutput();
    void testRobotDigitalOutputDefault();
    void testRobotFailNextMove();
    void testRobotFailNextMoveResetsAfterOne();
    void testRobotHomePosition();
    void testRobotDeviceTypeAndId();
    void testRobotMoveToExtremePositions();
    void testRobotMultipleOutputPins();
    void testRobotDisconnectClearsMotion();
};

// === SimCameraDriver ===

void SimDriversTest::testCameraInitialState() {
    SimCameraDriver cam;
    QCOMPARE(cam.status(), IDeviceDriver::DeviceStatus::Disconnected);
    QCOMPARE(cam.deviceType(), QString("Camera"));
    QVERIFY(cam.getPointCloud().empty());
}

void SimDriversTest::testCameraConnectDisconnect() {
    SimCameraDriver cam;
    QVERIFY(cam.connect({}));
    QCOMPARE(cam.status(), IDeviceDriver::DeviceStatus::Ready);

    cam.disconnect();
    QCOMPARE(cam.status(), IDeviceDriver::DeviceStatus::Disconnected);
    QVERIFY(cam.getPointCloud().empty()); // data cleared on disconnect
}

void SimDriversTest::testCameraDoubleConnect() {
    SimCameraDriver cam;
    QVERIFY(cam.connect({}));
    QVERIFY(!cam.connect({})); // already connected
    QCOMPARE(cam.status(), IDeviceDriver::DeviceStatus::Ready);
}

void SimDriversTest::testCameraDoubleDisconnect() {
    SimCameraDriver cam;
    cam.connect({});
    cam.disconnect();
    cam.disconnect(); // should not crash
    QCOMPARE(cam.status(), IDeviceDriver::DeviceStatus::Disconnected);
}

void SimDriversTest::testCameraCaptureBeforeConnect() {
    SimCameraDriver cam;
    CaptureConfig config;
    QVERIFY(!cam.capture(config)); // not connected
}

void SimDriversTest::testCameraCaptureAfterConnect() {
    SimCameraDriver cam;
    cam.connect({});
    QVERIFY(cam.capture(CaptureConfig{}));

    auto cloud = cam.getPointCloud();
    QVERIFY(!cloud.empty());
    QVERIFY(cloud.pointCount() > 0);
}

void SimDriversTest::testCameraCaptureWithROI() {
    SimCameraDriver cam;
    cam.connect({});

    CaptureConfig config;
    config.roiW = 320;
    config.roiH = 240;
    QVERIFY(cam.capture(config));

    auto cloud = cam.getPointCloud();
    QCOMPARE(cloud.width, 320);
    QCOMPARE(cloud.height, 240);
}

void SimDriversTest::testCameraPointCloudNotEmpty() {
    SimCameraDriver cam;
    cam.connect({});
    cam.setObjectCount(5);
    cam.capture(CaptureConfig{});

    auto cloud = cam.getPointCloud();
    QVERIFY(cloud.pointCount() > 100); // background + 5 objects
    // Each point should have 3 coordinates
    QCOMPARE(cloud.points.size() % 3, size_t(0));
}

void SimDriversTest::testCameraColorImageReturned() {
    SimCameraDriver cam;
    cam.connect({});
    cam.capture(CaptureConfig{});

    auto img = cam.getColorImage();
    QVERIFY(!img.empty());
    QCOMPARE(img.width, 640);
    QCOMPARE(img.height, 480);
    QCOMPARE(img.data.size(), size_t(640 * 480 * 3));
}

void SimDriversTest::testCameraZeroObjects() {
    SimCameraDriver cam;
    cam.connect({});
    cam.setObjectCount(0);
    cam.capture(CaptureConfig{});

    auto cloud = cam.getPointCloud();
    // Should still have background points
    QVERIFY(cloud.pointCount() > 0);
}

void SimDriversTest::testCameraSingleObject() {
    SimCameraDriver cam;
    cam.connect({});
    cam.setObjectCount(1);
    cam.capture(CaptureConfig{});

    auto cloud = cam.getPointCloud();
    QVERIFY(cloud.pointCount() > 0);
}

void SimDriversTest::testCameraManyObjects() {
    SimCameraDriver cam;
    cam.connect({});
    cam.setObjectCount(50);
    cam.capture(CaptureConfig{});

    auto cloud = cam.getPointCloud();
    // 50 objects * 200 points + background
    QVERIFY(cloud.pointCount() > 10000);
}

void SimDriversTest::testCameraHighNoise() {
    SimCameraDriver cam;
    cam.connect({});
    cam.setNoiseLevel(100.0); // extreme noise
    cam.setObjectCount(1);
    cam.capture(CaptureConfig{});

    auto cloud = cam.getPointCloud();
    QVERIFY(!cloud.empty());
}

void SimDriversTest::testCameraZeroNoise() {
    SimCameraDriver cam;
    cam.connect({});
    cam.setNoiseLevel(0.0);
    cam.setObjectCount(0);
    cam.capture(CaptureConfig{});

    auto cloud = cam.getPointCloud();
    // All background points should be at z=600 with zero noise
    for (int i = 0; i < cloud.pointCount(); ++i) {
        float z = cloud.points[i * 3 + 2];
        QCOMPARE(z, 600.0f);
    }
}

void SimDriversTest::testCameraDeviceTypeAndId() {
    SimCameraDriver cam("my-camera-42");
    QCOMPARE(cam.deviceId(), QString("my-camera-42"));
    QCOMPARE(cam.deviceType(), QString("Camera"));
}

void SimDriversTest::testCameraReconnectAndCapture() {
    SimCameraDriver cam;
    cam.connect({});
    cam.capture(CaptureConfig{});
    auto cloud1 = cam.getPointCloud();
    QVERIFY(!cloud1.empty());

    cam.disconnect();
    QVERIFY(cam.getPointCloud().empty());

    cam.connect({});
    cam.capture(CaptureConfig{});
    auto cloud2 = cam.getPointCloud();
    QVERIFY(!cloud2.empty());
}

void SimDriversTest::testCameraConsecutiveCaptures() {
    SimCameraDriver cam;
    cam.connect({});

    // Each capture should generate new data (random, not identical)
    cam.setObjectCount(2);
    cam.capture(CaptureConfig{});
    auto cloud1 = cam.getPointCloud();

    cam.capture(CaptureConfig{});
    auto cloud2 = cam.getPointCloud();

    // Point counts may differ due to random generation
    QVERIFY(!cloud1.empty());
    QVERIFY(!cloud2.empty());
}

// === SimRobotDriver ===

void SimDriversTest::testRobotInitialState() {
    SimRobotDriver robot;
    QCOMPARE(robot.status(), IDeviceDriver::DeviceStatus::Disconnected);
    QCOMPARE(robot.deviceType(), QString("Robot"));
    QVERIFY(!robot.isInMotion());
}

void SimDriversTest::testRobotConnectDisconnect() {
    SimRobotDriver robot;
    QVERIFY(robot.connect({}));
    QCOMPARE(robot.status(), IDeviceDriver::DeviceStatus::Ready);

    robot.disconnect();
    QCOMPARE(robot.status(), IDeviceDriver::DeviceStatus::Disconnected);
}

void SimDriversTest::testRobotDoubleConnect() {
    SimRobotDriver robot;
    QVERIFY(robot.connect({}));
    QVERIFY(!robot.connect({})); // already connected
}

void SimDriversTest::testRobotMoveBeforeConnect() {
    SimRobotDriver robot;
    CartesianPose target = {100, 200, 300, 0, 0, 0};
    QVERIFY(!robot.moveTo(target, 100.0)); // not connected
}

void SimDriversTest::testRobotMoveTo() {
    SimRobotDriver robot;
    robot.connect({});

    CartesianPose target = {100, 200, 300, 0.1, 0.2, 0.3};
    QVERIFY(robot.moveTo(target, 100.0));

    auto pose = robot.currentPose();
    QCOMPARE(pose.x, 100.0);
    QCOMPARE(pose.y, 200.0);
    QCOMPARE(pose.z, 300.0);
    QCOMPARE(pose.rx, 0.1);
    QCOMPARE(pose.ry, 0.2);
    QCOMPARE(pose.rz, 0.3);
}

void SimDriversTest::testRobotMoveJoint() {
    SimRobotDriver robot;
    robot.connect({});

    JointPosition target;
    target.joints = {0.1, 0.2, 0.3, 0.4, 0.5, 0.6};
    QVERIFY(robot.moveJoint(target, 50.0));

    auto joints = robot.currentJoints();
    for (int i = 0; i < 6; ++i)
        QCOMPARE(joints.joints[i], target.joints[i]);
}

void SimDriversTest::testRobotStopMotion() {
    SimRobotDriver robot;
    robot.connect({});
    QVERIFY(robot.stopMotion());
    QVERIFY(!robot.isInMotion());
}

void SimDriversTest::testRobotDigitalOutput() {
    SimRobotDriver robot;
    robot.connect({});
    robot.setDigitalOutput(0, true);
    QVERIFY(robot.digitalOutput(0));

    robot.setDigitalOutput(0, false);
    QVERIFY(!robot.digitalOutput(0));
}

void SimDriversTest::testRobotDigitalOutputDefault() {
    SimRobotDriver robot;
    // Unset pins should default to false
    QVERIFY(!robot.digitalOutput(0));
    QVERIFY(!robot.digitalOutput(99));
}

void SimDriversTest::testRobotFailNextMove() {
    SimRobotDriver robot;
    robot.connect({});
    robot.setFailNextMove(true);

    CartesianPose target = {100, 100, 100, 0, 0, 0};
    QVERIFY(!robot.moveTo(target, 100.0));

    // Position should NOT have changed
    auto pose = robot.currentPose();
    QCOMPARE(pose.x, 0.0); // still at home
}

void SimDriversTest::testRobotFailNextMoveResetsAfterOne() {
    SimRobotDriver robot;
    robot.connect({});
    robot.setFailNextMove(true);

    CartesianPose t1 = {100, 0, 0, 0, 0, 0};
    QVERIFY(!robot.moveTo(t1, 100.0)); // fails

    CartesianPose t2 = {200, 0, 0, 0, 0, 0};
    QVERIFY(robot.moveTo(t2, 100.0)); // succeeds (flag auto-resets)
    QCOMPARE(robot.currentPose().x, 200.0);
}

void SimDriversTest::testRobotHomePosition() {
    SimRobotDriver robot;
    robot.connect({});

    auto pose = robot.currentPose();
    QCOMPARE(pose.x, 0.0);
    QCOMPARE(pose.y, 0.0);
    QCOMPARE(pose.z, 500.0); // home z=500
}

void SimDriversTest::testRobotDeviceTypeAndId() {
    SimRobotDriver robot("kuka-kr6");
    QCOMPARE(robot.deviceId(), QString("kuka-kr6"));
    QCOMPARE(robot.deviceType(), QString("Robot"));
}

void SimDriversTest::testRobotMoveToExtremePositions() {
    SimRobotDriver robot;
    robot.connect({});

    // Very large coordinates
    CartesianPose far = {99999.0, -99999.0, 99999.0, 3.14, -3.14, 0};
    QVERIFY(robot.moveTo(far, 1000.0));
    QCOMPARE(robot.currentPose().x, 99999.0);
    QCOMPARE(robot.currentPose().y, -99999.0);

    // Zero coordinates
    CartesianPose zero = {0, 0, 0, 0, 0, 0};
    QVERIFY(robot.moveTo(zero, 0.1));
    QCOMPARE(robot.currentPose().x, 0.0);
}

void SimDriversTest::testRobotMultipleOutputPins() {
    SimRobotDriver robot;
    robot.connect({});

    // Set multiple pins
    robot.setDigitalOutput(0, true);   // gripper
    robot.setDigitalOutput(1, true);   // vacuum
    robot.setDigitalOutput(2, false);  // light
    robot.setDigitalOutput(7, true);   // custom

    QVERIFY(robot.digitalOutput(0));
    QVERIFY(robot.digitalOutput(1));
    QVERIFY(!robot.digitalOutput(2));
    QVERIFY(robot.digitalOutput(7));
    QVERIFY(!robot.digitalOutput(3)); // not set = false
}

void SimDriversTest::testRobotDisconnectClearsMotion() {
    SimRobotDriver robot;
    robot.connect({});
    robot.disconnect();
    QVERIFY(!robot.isInMotion());
}

QTEST_MAIN(SimDriversTest)
#include "test_simdrivers.moc"
