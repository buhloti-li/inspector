#include <QtTest/QtTest>

#include "device/devicemanager.h"
#include "device/sim/simcameradriver.h"
#include "device/sim/simrobotdriver.h"

class DeviceManagerTest : public QObject {
    Q_OBJECT

private slots:
    void testRegisterAndRetrieve();
    void testDuplicateRegister();
    void testRemoveDevice();
    void testTypedAccess();
    void testAllStatus();
};

void DeviceManagerTest::testRegisterAndRetrieve() {
    DeviceManager mgr;

    auto cam = std::make_unique<SimCameraDriver>("cam-01");
    QVERIFY(mgr.registerDevice("cam-01", std::move(cam)));

    QVERIFY(mgr.device("cam-01") != nullptr);
    QVERIFY(mgr.device("nonexistent") == nullptr);

    QStringList ids = mgr.deviceIds();
    QCOMPARE(ids.size(), 1);
    QVERIFY(ids.contains("cam-01"));
}

void DeviceManagerTest::testDuplicateRegister() {
    DeviceManager mgr;

    auto cam1 = std::make_unique<SimCameraDriver>("cam-01");
    auto cam2 = std::make_unique<SimCameraDriver>("cam-01");

    QVERIFY(mgr.registerDevice("cam-01", std::move(cam1)));
    QVERIFY(!mgr.registerDevice("cam-01", std::move(cam2))); // duplicate
}

void DeviceManagerTest::testRemoveDevice() {
    DeviceManager mgr;

    auto cam = std::make_unique<SimCameraDriver>("cam-01");
    mgr.registerDevice("cam-01", std::move(cam));

    QSignalSpy spy(&mgr, &DeviceManager::deviceRemoved);
    mgr.removeDevice("cam-01");

    QVERIFY(mgr.device("cam-01") == nullptr);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().first().toString(), QString("cam-01"));
}

void DeviceManagerTest::testTypedAccess() {
    DeviceManager mgr;

    auto cam = std::make_unique<SimCameraDriver>("cam-01");
    auto robot = std::make_unique<SimRobotDriver>("robot-01");

    mgr.registerDevice("cam-01", std::move(cam));
    mgr.registerDevice("robot-01", std::move(robot));

    QVERIFY(mgr.camera("cam-01") != nullptr);
    QVERIFY(mgr.robot("cam-01") == nullptr); // camera is not a robot
    QVERIFY(mgr.robot("robot-01") != nullptr);
    QVERIFY(mgr.camera("robot-01") == nullptr);
}

void DeviceManagerTest::testAllStatus() {
    DeviceManager mgr;

    auto cam = std::make_unique<SimCameraDriver>("cam-01");
    auto robot = std::make_unique<SimRobotDriver>("robot-01");

    mgr.registerDevice("cam-01", std::move(cam));
    mgr.registerDevice("robot-01", std::move(robot));

    auto statuses = mgr.allStatus();
    QCOMPARE(statuses.size(), 2);
    QCOMPARE(statuses["cam-01"], IDeviceDriver::DeviceStatus::Disconnected);

    // Connect camera
    mgr.camera("cam-01")->connect({});
    statuses = mgr.allStatus();
    QCOMPARE(statuses["cam-01"], IDeviceDriver::DeviceStatus::Ready);
}

QTEST_MAIN(DeviceManagerTest)
#include "test_devicemanager.moc"
