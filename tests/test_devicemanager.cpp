#include <QtTest/QtTest>

#include "device/devicemanager.h"
#include "device/sim/simcameradriver.h"
#include "device/sim/simrobotdriver.h"

class DeviceManagerTest : public QObject {
    Q_OBJECT

private slots:
    // === 注册 ===
    void testRegisterAndRetrieve();
    void testDuplicateRegister();
    void testRegisterNullDriver();
    void testRegisterMultipleDevices();
    void testRegisterSignal();

    // === 移除 ===
    void testRemoveDevice();
    void testRemoveNonexistentDevice();
    void testRemoveThenReRegister();

    // === 类型访问（设备类型混淆/误判） ===
    void testTypedAccess();
    void testCameraAsRobot();
    void testRobotAsCamera();
    void testPlcAccessOnEmptyManager();
    void testNonexistentDeviceAccess();

    // === 状态管理（模拟连接/断开/异常） ===
    void testAllStatus();
    void testStatusAfterConnect();
    void testStatusAfterDisconnect();
    void testStatusMixedDevices();

    // === 设备热插拔（运行时增删） ===
    void testAddDeviceDuringOperation();
    void testRemoveDeviceDuringOperation();

    // === 边界条件 ===
    void testEmptyManager();
    void testEmptyStringId();
    void testSpecialCharacterId();
    void testLargeNumberOfDevices();
};

// === 注册 ===

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
    QVERIFY(!mgr.registerDevice("cam-01", std::move(cam2)));
    QCOMPARE(mgr.deviceIds().size(), 1);
}

void DeviceManagerTest::testRegisterNullDriver() {
    DeviceManager mgr;
    QVERIFY(!mgr.registerDevice("null-dev", nullptr));
    QCOMPARE(mgr.deviceIds().size(), 0);
}

void DeviceManagerTest::testRegisterMultipleDevices() {
    DeviceManager mgr;

    mgr.registerDevice("cam-01", std::make_unique<SimCameraDriver>("cam-01"));
    mgr.registerDevice("cam-02", std::make_unique<SimCameraDriver>("cam-02"));
    mgr.registerDevice("robot-01", std::make_unique<SimRobotDriver>("robot-01"));
    mgr.registerDevice("robot-02", std::make_unique<SimRobotDriver>("robot-02"));

    QCOMPARE(mgr.deviceIds().size(), 4);
    QVERIFY(mgr.camera("cam-01") != nullptr);
    QVERIFY(mgr.camera("cam-02") != nullptr);
    QVERIFY(mgr.robot("robot-01") != nullptr);
    QVERIFY(mgr.robot("robot-02") != nullptr);
}

void DeviceManagerTest::testRegisterSignal() {
    DeviceManager mgr;
    QSignalSpy spy(&mgr, &DeviceManager::deviceRegistered);

    mgr.registerDevice("cam-01", std::make_unique<SimCameraDriver>("cam-01"));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().first().toString(), QString("cam-01"));
}

// === 移除 ===

void DeviceManagerTest::testRemoveDevice() {
    DeviceManager mgr;
    mgr.registerDevice("cam-01", std::make_unique<SimCameraDriver>("cam-01"));

    QSignalSpy spy(&mgr, &DeviceManager::deviceRemoved);
    mgr.removeDevice("cam-01");

    QVERIFY(mgr.device("cam-01") == nullptr);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().first().toString(), QString("cam-01"));
    QCOMPARE(mgr.deviceIds().size(), 0);
}

void DeviceManagerTest::testRemoveNonexistentDevice() {
    DeviceManager mgr;
    QSignalSpy spy(&mgr, &DeviceManager::deviceRemoved);
    mgr.removeDevice("nonexistent");
    QCOMPARE(spy.count(), 0); // no signal for nonexistent removal
}

void DeviceManagerTest::testRemoveThenReRegister() {
    DeviceManager mgr;
    mgr.registerDevice("cam-01", std::make_unique<SimCameraDriver>("cam-01"));
    mgr.removeDevice("cam-01");
    QVERIFY(mgr.device("cam-01") == nullptr);

    // Re-register with same ID
    QVERIFY(mgr.registerDevice("cam-01", std::make_unique<SimCameraDriver>("cam-01-v2")));
    QVERIFY(mgr.device("cam-01") != nullptr);
    QCOMPARE(mgr.device("cam-01")->deviceId(), QString("cam-01-v2"));
}

// === 类型访问（设备类型混淆/误判） ===

void DeviceManagerTest::testTypedAccess() {
    DeviceManager mgr;
    mgr.registerDevice("cam-01", std::make_unique<SimCameraDriver>("cam-01"));
    mgr.registerDevice("robot-01", std::make_unique<SimRobotDriver>("robot-01"));

    QVERIFY(mgr.camera("cam-01") != nullptr);
    QVERIFY(mgr.robot("cam-01") == nullptr);
    QVERIFY(mgr.robot("robot-01") != nullptr);
    QVERIFY(mgr.camera("robot-01") == nullptr);
}

void DeviceManagerTest::testCameraAsRobot() {
    DeviceManager mgr;
    mgr.registerDevice("cam", std::make_unique<SimCameraDriver>("cam"));
    QVERIFY(mgr.robot("cam") == nullptr);
    QVERIFY(mgr.plc("cam") == nullptr);
}

void DeviceManagerTest::testRobotAsCamera() {
    DeviceManager mgr;
    mgr.registerDevice("bot", std::make_unique<SimRobotDriver>("bot"));
    QVERIFY(mgr.camera("bot") == nullptr);
    QVERIFY(mgr.plc("bot") == nullptr);
}

void DeviceManagerTest::testPlcAccessOnEmptyManager() {
    DeviceManager mgr;
    QVERIFY(mgr.plc("any") == nullptr);
}

void DeviceManagerTest::testNonexistentDeviceAccess() {
    DeviceManager mgr;
    QVERIFY(mgr.device("xxx") == nullptr);
    QVERIFY(mgr.camera("xxx") == nullptr);
    QVERIFY(mgr.robot("xxx") == nullptr);
    QVERIFY(mgr.plc("xxx") == nullptr);
}

// === 状态管理 ===

void DeviceManagerTest::testAllStatus() {
    DeviceManager mgr;
    mgr.registerDevice("cam-01", std::make_unique<SimCameraDriver>("cam-01"));
    mgr.registerDevice("robot-01", std::make_unique<SimRobotDriver>("robot-01"));

    auto statuses = mgr.allStatus();
    QCOMPARE(statuses.size(), 2);
    QCOMPARE(statuses["cam-01"], IDeviceDriver::DeviceStatus::Disconnected);
    QCOMPARE(statuses["robot-01"], IDeviceDriver::DeviceStatus::Disconnected);
}

void DeviceManagerTest::testStatusAfterConnect() {
    DeviceManager mgr;
    mgr.registerDevice("cam-01", std::make_unique<SimCameraDriver>("cam-01"));

    mgr.camera("cam-01")->connect({});
    QCOMPARE(mgr.allStatus()["cam-01"], IDeviceDriver::DeviceStatus::Ready);
}

void DeviceManagerTest::testStatusAfterDisconnect() {
    DeviceManager mgr;
    mgr.registerDevice("cam-01", std::make_unique<SimCameraDriver>("cam-01"));

    mgr.camera("cam-01")->connect({});
    QCOMPARE(mgr.allStatus()["cam-01"], IDeviceDriver::DeviceStatus::Ready);

    mgr.camera("cam-01")->disconnect();
    QCOMPARE(mgr.allStatus()["cam-01"], IDeviceDriver::DeviceStatus::Disconnected);
}

void DeviceManagerTest::testStatusMixedDevices() {
    DeviceManager mgr;
    mgr.registerDevice("cam", std::make_unique<SimCameraDriver>("cam"));
    mgr.registerDevice("bot", std::make_unique<SimRobotDriver>("bot"));

    // Only connect camera, robot stays disconnected
    mgr.camera("cam")->connect({});

    auto statuses = mgr.allStatus();
    QCOMPARE(statuses["cam"], IDeviceDriver::DeviceStatus::Ready);
    QCOMPARE(statuses["bot"], IDeviceDriver::DeviceStatus::Disconnected);
}

// === 设备热插拔 ===

void DeviceManagerTest::testAddDeviceDuringOperation() {
    DeviceManager mgr;
    mgr.registerDevice("cam", std::make_unique<SimCameraDriver>("cam"));
    mgr.camera("cam")->connect({});

    // Add a robot while camera is connected and operational
    mgr.registerDevice("bot", std::make_unique<SimRobotDriver>("bot"));
    QCOMPARE(mgr.deviceIds().size(), 2);
    QCOMPARE(mgr.allStatus()["cam"], IDeviceDriver::DeviceStatus::Ready);
    QCOMPARE(mgr.allStatus()["bot"], IDeviceDriver::DeviceStatus::Disconnected);
}

void DeviceManagerTest::testRemoveDeviceDuringOperation() {
    DeviceManager mgr;
    mgr.registerDevice("cam", std::make_unique<SimCameraDriver>("cam"));
    mgr.registerDevice("bot", std::make_unique<SimRobotDriver>("bot"));

    mgr.camera("cam")->connect({});
    mgr.robot("bot")->connect({});

    // Remove camera while both are connected
    mgr.removeDevice("cam");
    QCOMPARE(mgr.deviceIds().size(), 1);
    QVERIFY(mgr.camera("cam") == nullptr);
    QVERIFY(mgr.robot("bot") != nullptr);
    QCOMPARE(mgr.robot("bot")->status(), IDeviceDriver::DeviceStatus::Ready);
}

// === 边界条件 ===

void DeviceManagerTest::testEmptyManager() {
    DeviceManager mgr;
    QCOMPARE(mgr.deviceIds().size(), 0);
    QCOMPARE(mgr.allStatus().size(), 0);
}

void DeviceManagerTest::testEmptyStringId() {
    DeviceManager mgr;
    auto cam = std::make_unique<SimCameraDriver>("empty-id");
    QVERIFY(mgr.registerDevice("", std::move(cam)));
    QVERIFY(mgr.device("") != nullptr);
}

void DeviceManagerTest::testSpecialCharacterId() {
    DeviceManager mgr;
    auto cam = std::make_unique<SimCameraDriver>("special");
    QString specialId = "设备/相机-01@工位#1";
    QVERIFY(mgr.registerDevice(specialId, std::move(cam)));
    QVERIFY(mgr.device(specialId) != nullptr);
    QVERIFY(mgr.camera(specialId) != nullptr);
}

void DeviceManagerTest::testLargeNumberOfDevices() {
    DeviceManager mgr;
    const int count = 100;
    for (int i = 0; i < count; ++i) {
        QString id = QString("cam-%1").arg(i, 3, 10, QChar('0'));
        mgr.registerDevice(id, std::make_unique<SimCameraDriver>(id));
    }
    QCOMPARE(mgr.deviceIds().size(), count);
    QVERIFY(mgr.camera("cam-000") != nullptr);
    QVERIFY(mgr.camera("cam-099") != nullptr);
    QVERIFY(mgr.camera("cam-100") == nullptr);
}

QTEST_MAIN(DeviceManagerTest)
#include "test_devicemanager.moc"
