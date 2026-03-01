#include <QtTest>
#include <QSignalSpy>
#include "mobile/mobileclient.h"
#include "mobile/devicelistmodel.h"

class DeviceListModelTest : public QObject {
    Q_OBJECT
private slots:
    // === Initial State ===
    void testInitialState();

    // === Add/Remove Devices ===
    void testAddDevice();
    void testAddDuplicateDevice();
    void testRemoveDevice();
    void testRemoveNonExistent();
    void testClear();

    // === Query ===
    void testDeviceCount();
    void testDeviceAt();
    void testDeviceAtOutOfBounds();
    void testDeviceById();
    void testDeviceByIdNotFound();
    void testHasDevice();
    void testAllDevices();
    void testDevicesByType();
    void testDevicesByStatus();

    // === Status Update ===
    void testUpdateDeviceStatus();
    void testUpdateStatusNoChange();
    void testUpdateStatusNonExistent();

    // === Bulk Update ===
    void testUpdateDeviceList();
    void testUpdateDeviceListEmpty();
    void testUpdateDeviceListReplace();

    // === Server Integration ===
    void testDeviceListFromServerStatus();
    void testStatusWithoutDevices();

    // === Signals ===
    void testDeviceAddedSignal();
    void testDeviceRemovedSignal();
    void testDeviceStatusChangedSignal();
    void testDeviceListUpdatedSignal();

    // === Edge Cases ===
    void testMultipleDeviceTypes();
    void testDeviceWithProperties();
    void testFilterByMultipleStatuses();
};

void DeviceListModelTest::testInitialState() {
    MobileClient client;
    DeviceListModel model(&client);
    QCOMPARE(model.deviceCount(), 0);
    QVERIFY(model.allDevices().isEmpty());
}

void DeviceListModelTest::testAddDevice() {
    MobileClient client;
    DeviceListModel model(&client);

    DeviceListModel::DeviceInfo info;
    info.deviceId = "cam_01";
    info.deviceType = "camera";
    info.status = "Ready";
    model.addDevice(info);

    QCOMPARE(model.deviceCount(), 1);
    QCOMPARE(model.deviceAt(0).deviceId, QString("cam_01"));
}

void DeviceListModelTest::testAddDuplicateDevice() {
    MobileClient client;
    DeviceListModel model(&client);

    DeviceListModel::DeviceInfo info;
    info.deviceId = "cam_01";
    info.deviceType = "camera";
    info.status = "Ready";
    model.addDevice(info);
    model.addDevice(info); // duplicate

    QCOMPARE(model.deviceCount(), 1); // not added twice
}

void DeviceListModelTest::testRemoveDevice() {
    MobileClient client;
    DeviceListModel model(&client);

    DeviceListModel::DeviceInfo info;
    info.deviceId = "robot_01";
    info.deviceType = "robot";
    info.status = "Ready";
    model.addDevice(info);
    model.removeDevice("robot_01");

    QCOMPARE(model.deviceCount(), 0);
}

void DeviceListModelTest::testRemoveNonExistent() {
    MobileClient client;
    DeviceListModel model(&client);
    QSignalSpy spy(&model, &DeviceListModel::deviceRemoved);
    model.removeDevice("does_not_exist");
    QCOMPARE(spy.count(), 0);
}

void DeviceListModelTest::testClear() {
    MobileClient client;
    DeviceListModel model(&client);

    DeviceListModel::DeviceInfo info;
    info.deviceId = "dev1";
    model.addDevice(info);
    info.deviceId = "dev2";
    model.addDevice(info);

    model.clear();
    QCOMPARE(model.deviceCount(), 0);
}

void DeviceListModelTest::testDeviceCount() {
    MobileClient client;
    DeviceListModel model(&client);

    for (int i = 0; i < 5; ++i) {
        DeviceListModel::DeviceInfo info;
        info.deviceId = QString("dev_%1").arg(i);
        model.addDevice(info);
    }
    QCOMPARE(model.deviceCount(), 5);
}

void DeviceListModelTest::testDeviceAt() {
    MobileClient client;
    DeviceListModel model(&client);

    DeviceListModel::DeviceInfo info;
    info.deviceId = "first";
    model.addDevice(info);
    info.deviceId = "second";
    model.addDevice(info);

    QCOMPARE(model.deviceAt(0).deviceId, QString("first"));
    QCOMPARE(model.deviceAt(1).deviceId, QString("second"));
}

void DeviceListModelTest::testDeviceAtOutOfBounds() {
    MobileClient client;
    DeviceListModel model(&client);
    auto dev = model.deviceAt(-1);
    QVERIFY(dev.deviceId.isEmpty());
    dev = model.deviceAt(100);
    QVERIFY(dev.deviceId.isEmpty());
}

void DeviceListModelTest::testDeviceById() {
    MobileClient client;
    DeviceListModel model(&client);

    DeviceListModel::DeviceInfo info;
    info.deviceId = "unique_id";
    info.deviceType = "plc";
    info.status = "Ready";
    model.addDevice(info);

    auto found = model.deviceById("unique_id");
    QCOMPARE(found.deviceType, QString("plc"));
}

void DeviceListModelTest::testDeviceByIdNotFound() {
    MobileClient client;
    DeviceListModel model(&client);
    auto found = model.deviceById("no_such");
    QVERIFY(found.deviceId.isEmpty());
}

void DeviceListModelTest::testHasDevice() {
    MobileClient client;
    DeviceListModel model(&client);

    DeviceListModel::DeviceInfo info;
    info.deviceId = "cam_01";
    model.addDevice(info);

    QVERIFY(model.hasDevice("cam_01"));
    QVERIFY(!model.hasDevice("cam_02"));
}

void DeviceListModelTest::testAllDevices() {
    MobileClient client;
    DeviceListModel model(&client);

    DeviceListModel::DeviceInfo info;
    info.deviceId = "a";
    model.addDevice(info);
    info.deviceId = "b";
    model.addDevice(info);

    QCOMPARE(model.allDevices().size(), 2);
}

void DeviceListModelTest::testDevicesByType() {
    MobileClient client;
    DeviceListModel model(&client);

    DeviceListModel::DeviceInfo cam;
    cam.deviceId = "cam1";
    cam.deviceType = "camera";
    model.addDevice(cam);

    cam.deviceId = "cam2";
    model.addDevice(cam);

    DeviceListModel::DeviceInfo robot;
    robot.deviceId = "rob1";
    robot.deviceType = "robot";
    model.addDevice(robot);

    QCOMPARE(model.devicesByType("camera").size(), 2);
    QCOMPARE(model.devicesByType("robot").size(), 1);
    QCOMPARE(model.devicesByType("plc").size(), 0);
}

void DeviceListModelTest::testDevicesByStatus() {
    MobileClient client;
    DeviceListModel model(&client);

    DeviceListModel::DeviceInfo info;
    info.deviceId = "a";
    info.status = "Ready";
    model.addDevice(info);

    info.deviceId = "b";
    info.status = "Error";
    model.addDevice(info);

    info.deviceId = "c";
    info.status = "Ready";
    model.addDevice(info);

    QCOMPARE(model.devicesByStatus("Ready").size(), 2);
    QCOMPARE(model.devicesByStatus("Error").size(), 1);
    QCOMPARE(model.devicesByStatus("Disconnected").size(), 0);
}

void DeviceListModelTest::testUpdateDeviceStatus() {
    MobileClient client;
    DeviceListModel model(&client);

    DeviceListModel::DeviceInfo info;
    info.deviceId = "dev1";
    info.status = "Ready";
    model.addDevice(info);

    model.updateDeviceStatus("dev1", "Error");
    QCOMPARE(model.deviceById("dev1").status, QString("Error"));
}

void DeviceListModelTest::testUpdateStatusNoChange() {
    MobileClient client;
    DeviceListModel model(&client);

    DeviceListModel::DeviceInfo info;
    info.deviceId = "dev1";
    info.status = "Ready";
    model.addDevice(info);

    QSignalSpy spy(&model, &DeviceListModel::deviceStatusChanged);
    model.updateDeviceStatus("dev1", "Ready"); // same status
    QCOMPARE(spy.count(), 0);
}

void DeviceListModelTest::testUpdateStatusNonExistent() {
    MobileClient client;
    DeviceListModel model(&client);
    QSignalSpy spy(&model, &DeviceListModel::deviceStatusChanged);
    model.updateDeviceStatus("no_exist", "Error");
    QCOMPARE(spy.count(), 0);
}

void DeviceListModelTest::testUpdateDeviceList() {
    MobileClient client;
    DeviceListModel model(&client);

    QVariantList list;
    QVariantMap dev1;
    dev1["deviceId"] = "cam_01";
    dev1["deviceType"] = "camera";
    dev1["status"] = "Ready";
    list.append(dev1);

    QVariantMap dev2;
    dev2["deviceId"] = "rob_01";
    dev2["deviceType"] = "robot";
    dev2["status"] = "Busy";
    list.append(dev2);

    model.updateDeviceList(list);
    QCOMPARE(model.deviceCount(), 2);
    QCOMPARE(model.deviceById("cam_01").deviceType, QString("camera"));
    QCOMPARE(model.deviceById("rob_01").status, QString("Busy"));
}

void DeviceListModelTest::testUpdateDeviceListEmpty() {
    MobileClient client;
    DeviceListModel model(&client);

    DeviceListModel::DeviceInfo info;
    info.deviceId = "existing";
    model.addDevice(info);

    model.updateDeviceList({});
    QCOMPARE(model.deviceCount(), 0);
}

void DeviceListModelTest::testUpdateDeviceListReplace() {
    MobileClient client;
    DeviceListModel model(&client);

    // First list
    QVariantList list1;
    list1.append(QVariantMap{{"deviceId", "a"}, {"deviceType", "cam"}, {"status", "Ready"}});
    model.updateDeviceList(list1);
    QCOMPARE(model.deviceCount(), 1);

    // Replace with new list
    QVariantList list2;
    list2.append(QVariantMap{{"deviceId", "x"}, {"deviceType", "robot"}, {"status", "Ready"}});
    list2.append(QVariantMap{{"deviceId", "y"}, {"deviceType", "plc"}, {"status", "Ready"}});
    model.updateDeviceList(list2);
    QCOMPARE(model.deviceCount(), 2);
    QVERIFY(!model.hasDevice("a"));
    QVERIFY(model.hasDevice("x"));
}

void DeviceListModelTest::testDeviceListFromServerStatus() {
    MobileClient client;
    client.setSimulationMode(true);
    client.connectToServer("host", 8080);
    DeviceListModel model(&client);

    QSignalSpy spy(&model, &DeviceListModel::deviceListUpdated);

    QVariantMap status;
    status["state"] = "Idle";
    QVariantList devices;
    devices.append(QVariantMap{{"deviceId", "cam"}, {"deviceType", "camera"}, {"status", "Ready"}});
    status["devices"] = devices;

    client.injectSimResponse(1, status);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(model.deviceCount(), 1);
}

void DeviceListModelTest::testStatusWithoutDevices() {
    MobileClient client;
    client.setSimulationMode(true);
    client.connectToServer("host", 8080);
    DeviceListModel model(&client);

    QVariantMap status;
    status["state"] = "Running";
    // No "devices" key
    client.injectSimResponse(1, status);
    QCOMPARE(model.deviceCount(), 0);
}

void DeviceListModelTest::testDeviceAddedSignal() {
    MobileClient client;
    DeviceListModel model(&client);
    QSignalSpy spy(&model, &DeviceListModel::deviceAdded);

    DeviceListModel::DeviceInfo info;
    info.deviceId = "new_dev";
    model.addDevice(info);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().first().toString(), QString("new_dev"));
}

void DeviceListModelTest::testDeviceRemovedSignal() {
    MobileClient client;
    DeviceListModel model(&client);

    DeviceListModel::DeviceInfo info;
    info.deviceId = "rm_dev";
    model.addDevice(info);

    QSignalSpy spy(&model, &DeviceListModel::deviceRemoved);
    model.removeDevice("rm_dev");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().first().toString(), QString("rm_dev"));
}

void DeviceListModelTest::testDeviceStatusChangedSignal() {
    MobileClient client;
    DeviceListModel model(&client);

    DeviceListModel::DeviceInfo info;
    info.deviceId = "dev";
    info.status = "Ready";
    model.addDevice(info);

    QSignalSpy spy(&model, &DeviceListModel::deviceStatusChanged);
    model.updateDeviceStatus("dev", "Error");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().at(0).toString(), QString("dev"));
    QCOMPARE(spy.first().at(1).toString(), QString("Ready"));  // old
    QCOMPARE(spy.first().at(2).toString(), QString("Error"));   // new
}

void DeviceListModelTest::testDeviceListUpdatedSignal() {
    MobileClient client;
    DeviceListModel model(&client);
    QSignalSpy spy(&model, &DeviceListModel::deviceListUpdated);
    model.updateDeviceList({});
    QCOMPARE(spy.count(), 1);
}

void DeviceListModelTest::testMultipleDeviceTypes() {
    MobileClient client;
    DeviceListModel model(&client);

    QStringList types = {"camera", "robot", "plc", "sensor", "gripper"};
    for (int i = 0; i < types.size(); ++i) {
        DeviceListModel::DeviceInfo info;
        info.deviceId = QString("dev_%1").arg(i);
        info.deviceType = types[i];
        info.status = "Ready";
        model.addDevice(info);
    }

    QCOMPARE(model.deviceCount(), 5);
    for (const auto& t : types) {
        QCOMPARE(model.devicesByType(t).size(), 1);
    }
}

void DeviceListModelTest::testDeviceWithProperties() {
    MobileClient client;
    DeviceListModel model(&client);

    QVariantList list;
    QVariantMap dev;
    dev["deviceId"] = "cam_01";
    dev["deviceType"] = "camera";
    dev["status"] = "Ready";
    QVariantMap props;
    props["resolution"] = "1920x1080";
    props["fps"] = 30;
    dev["properties"] = props;
    list.append(dev);

    model.updateDeviceList(list);
    auto found = model.deviceById("cam_01");
    QCOMPARE(found.properties["resolution"].toString(), QString("1920x1080"));
    QCOMPARE(found.properties["fps"].toInt(), 30);
}

void DeviceListModelTest::testFilterByMultipleStatuses() {
    MobileClient client;
    DeviceListModel model(&client);

    DeviceListModel::DeviceInfo info;
    for (int i = 0; i < 3; ++i) {
        info.deviceId = QString("r_%1").arg(i);
        info.status = "Ready";
        model.addDevice(info);
    }
    for (int i = 0; i < 2; ++i) {
        info.deviceId = QString("e_%1").arg(i);
        info.status = "Error";
        model.addDevice(info);
    }
    info.deviceId = "d_0";
    info.status = "Disconnected";
    model.addDevice(info);

    QCOMPARE(model.devicesByStatus("Ready").size(), 3);
    QCOMPARE(model.devicesByStatus("Error").size(), 2);
    QCOMPARE(model.devicesByStatus("Disconnected").size(), 1);
}

QTEST_MAIN(DeviceListModelTest)
#include "test_devicelistmodel.moc"
