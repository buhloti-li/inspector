#include <QtTest/QtTest>
#include <QApplication>
#include <QTabWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QSlider>
#include <QCheckBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QMenuBar>
#include <QStatusBar>
#include <QSignalSpy>

#include "mainwindow.h"
#include "dashboardwidget.h"
#include "devicetablewidget.h"
#include "controlpanelwidget.h"
#include "alertwidget.h"
#include "connectiondialog.h"

#include "mobile/mobileclient.h"
#include "mobile/workcellstatusprovider.h"
#include "mobile/remotecommandservice.h"
#include "mobile/alertnotificationservice.h"
#include "mobile/devicelistmodel.h"

class DesktopClientTest : public QObject {
    Q_OBJECT

private:
    // Helper to create full service stack
    struct Services {
        MobileClient client;
        WorkcellStatusProvider* statusProvider;
        RemoteCommandService* commandService;
        AlertNotificationService* alertService;
        DeviceListModel* deviceModel;

        Services() {
            client.setSimulationMode(true);
            client.connectToServer("test", 9600);
            statusProvider = new WorkcellStatusProvider(&client);
            commandService = new RemoteCommandService(&client);
            alertService = new AlertNotificationService(&client);
            deviceModel = new DeviceListModel(&client);
        }

        ~Services() {
            delete deviceModel;
            delete alertService;
            delete commandService;
            delete statusProvider;
        }
    };

private slots:
    // === MainWindow Tests ===
    void testMainWindowCreation();
    void testMainWindowTabs();
    void testMainWindowMenuBar();
    void testMainWindowStatusBar();
    void testMainWindowConnectionUpdate();
    void testMainWindowStateUpdate();
    void testMainWindowAlertBadge();

    // === DashboardWidget Tests ===
    void testDashboardInitialState();
    void testDashboardStateChange();
    void testDashboardCycleCountUpdate();
    void testDashboardSuccessRate();
    void testDashboardRecipeChange();
    void testDashboardAutoModeToggle();

    // === DeviceTableWidget Tests ===
    void testDeviceTableInitialEmpty();
    void testDeviceTablePopulate();
    void testDeviceTableStatusColors();
    void testDeviceTableFilterByType();
    void testDeviceTableFilterByStatus();
    void testDeviceTableDeviceAddRemove();

    // === ControlPanelWidget Tests ===
    void testControlPanelButtons();
    void testControlPanelSpeedSlider();
    void testControlPanelCommandLog();
    void testControlPanelStartCycle();
    void testControlPanelStopCycle();
    void testControlPanelEmergencyStop();
    void testControlPanelAutoMode();
    void testControlPanelLoadRecipe();

    // === AlertWidget Tests ===
    void testAlertWidgetInitialEmpty();
    void testAlertWidgetPopulate();
    void testAlertWidgetSeverityColors();
    void testAlertWidgetAcknowledge();
    void testAlertWidgetAcknowledgeAll();
    void testAlertWidgetClear();
    void testAlertWidgetSeverityFilter();

    // === ConnectionDialog Tests ===
    void testConnectionDialogCreation();
    void testConnectionDialogDefaults();
    void testConnectionDialogConnect();
    void testConnectionDialogDisconnect();
    void testConnectionDialogStateUpdate();

    // === Integration Tests ===
    void testFullStatusUpdateFlow();
    void testDeviceListFromServer();
    void testAlertFromServer();
};

// === MainWindow Tests ===

void DesktopClientTest::testMainWindowCreation()
{
    Services svc;
    MainWindow win(&svc.client, svc.statusProvider, svc.commandService,
                   svc.alertService, svc.deviceModel);
    QVERIFY(win.windowTitle().contains(QStringLiteral("\u5DE5\u4E1A\u76D1\u63A7")));
    QCOMPARE(win.minimumWidth(), 900);
    QCOMPARE(win.minimumHeight(), 600);
}

void DesktopClientTest::testMainWindowTabs()
{
    Services svc;
    MainWindow win(&svc.client, svc.statusProvider, svc.commandService,
                   svc.alertService, svc.deviceModel);

    auto* tabs = win.findChild<QTabWidget*>();
    QVERIFY(tabs);
    QCOMPARE(tabs->count(), 4);
    QVERIFY(tabs->tabText(0).contains(QStringLiteral("\u4EEA\u8868\u76D8")));     // 仪表盘
    QVERIFY(tabs->tabText(1).contains(QStringLiteral("\u8BBE\u5907")));             // 设备
    QVERIFY(tabs->tabText(2).contains(QStringLiteral("\u63A7\u5236")));             // 控制
    QVERIFY(tabs->tabText(3).contains(QStringLiteral("\u544A\u8B66")));             // 告警
}

void DesktopClientTest::testMainWindowMenuBar()
{
    Services svc;
    MainWindow win(&svc.client, svc.statusProvider, svc.commandService,
                   svc.alertService, svc.deviceModel);

    auto* menuBar = win.menuBar();
    QVERIFY(menuBar);
    QVERIFY(menuBar->actions().size() >= 3);  // File, View, Help
}

void DesktopClientTest::testMainWindowStatusBar()
{
    Services svc;
    MainWindow win(&svc.client, svc.statusProvider, svc.commandService,
                   svc.alertService, svc.deviceModel);

    auto* statusBar = win.statusBar();
    QVERIFY(statusBar);
}

void DesktopClientTest::testMainWindowConnectionUpdate()
{
    Services svc;
    MainWindow win(&svc.client, svc.statusProvider, svc.commandService,
                   svc.alertService, svc.deviceModel);

    // Simulate connection
    emit svc.client.connected();
    QApplication::processEvents();

    // Find connection label in status bar
    auto labels = win.statusBar()->findChildren<QLabel*>();
    QVERIFY(!labels.isEmpty());
}

void DesktopClientTest::testMainWindowStateUpdate()
{
    Services svc;
    MainWindow win(&svc.client, svc.statusProvider, svc.commandService,
                   svc.alertService, svc.deviceModel);

    QVariantMap status;
    status["state"] = "Running";
    status["cycleCount"] = 10;
    status["successCount"] = 8;
    status["failCount"] = 2;
    svc.client.injectSimResponse(1, status);
    QApplication::processEvents();

    // State label should reflect running
    auto labels = win.statusBar()->findChildren<QLabel*>();
    bool foundState = false;
    for (auto* label : labels) {
        if (label->text().contains(QStringLiteral("\u8FD0\u884C"))) {
            foundState = true;
            break;
        }
    }
    QVERIFY(foundState);
}

void DesktopClientTest::testMainWindowAlertBadge()
{
    Services svc;
    MainWindow win(&svc.client, svc.statusProvider, svc.commandService,
                   svc.alertService, svc.deviceModel);

    auto* tabs = win.findChild<QTabWidget*>();
    QVERIFY(tabs);

    // Initially no badge
    QVERIFY(!tabs->tabText(3).contains("("));

    // Add alert
    svc.alertService->addAlert(AlertNotificationService::Severity::Error, "test", "msg");
    QApplication::processEvents();

    // Tab should show count
    QVERIFY(tabs->tabText(3).contains("(1)"));
}

// === DashboardWidget Tests ===

void DesktopClientTest::testDashboardInitialState()
{
    Services svc;
    DashboardWidget dashboard(svc.statusProvider);

    auto* stateLabel = dashboard.findChild<QLabel*>();
    QVERIFY(stateLabel);
}

void DesktopClientTest::testDashboardStateChange()
{
    Services svc;
    DashboardWidget dashboard(svc.statusProvider);

    QVariantMap status;
    status["state"] = "Fault";
    svc.client.injectSimResponse(1, status);
    QApplication::processEvents();

    // Find label with fault text
    auto labels = dashboard.findChildren<QLabel*>();
    bool found = false;
    for (auto* l : labels) {
        if (l->text() == QStringLiteral("\u6545\u969C")) {
            found = true;
            break;
        }
    }
    QVERIFY(found);
}

void DesktopClientTest::testDashboardCycleCountUpdate()
{
    Services svc;
    DashboardWidget dashboard(svc.statusProvider);

    QVariantMap status;
    status["state"] = "Running";
    status["cycleCount"] = 42;
    status["successCount"] = 35;
    status["failCount"] = 7;
    svc.client.injectSimResponse(1, status);
    QApplication::processEvents();

    // Verify labels contain the numbers
    auto labels = dashboard.findChildren<QLabel*>();
    bool foundTotal = false, foundSuccess = false, foundFail = false;
    for (auto* l : labels) {
        if (l->text() == "42") foundTotal = true;
        if (l->text() == "35") foundSuccess = true;
        if (l->text() == "7") foundFail = true;
    }
    QVERIFY(foundTotal);
    QVERIFY(foundSuccess);
    QVERIFY(foundFail);
}

void DesktopClientTest::testDashboardSuccessRate()
{
    Services svc;
    DashboardWidget dashboard(svc.statusProvider);

    QVariantMap status;
    status["state"] = "Idle";
    status["cycleCount"] = 100;
    status["successCount"] = 75;
    status["failCount"] = 25;
    svc.client.injectSimResponse(1, status);
    QApplication::processEvents();

    auto labels = dashboard.findChildren<QLabel*>();
    bool found = false;
    for (auto* l : labels) {
        if (l->text().contains("75.0%")) {
            found = true;
            break;
        }
    }
    QVERIFY(found);
}

void DesktopClientTest::testDashboardRecipeChange()
{
    Services svc;
    DashboardWidget dashboard(svc.statusProvider);

    QVariantMap status;
    status["state"] = "Idle";
    status["recipe"] = "TestRecipeAlpha";
    svc.client.injectSimResponse(1, status);
    QApplication::processEvents();

    auto labels = dashboard.findChildren<QLabel*>();
    bool found = false;
    for (auto* l : labels) {
        if (l->text() == "TestRecipeAlpha") {
            found = true;
            break;
        }
    }
    QVERIFY(found);
}

void DesktopClientTest::testDashboardAutoModeToggle()
{
    Services svc;
    DashboardWidget dashboard(svc.statusProvider);

    // Default should be manual
    auto labels = dashboard.findChildren<QLabel*>();
    bool foundManual = false;
    for (auto* l : labels) {
        if (l->text() == QStringLiteral("\u624B\u52A8")) {  // 手动
            foundManual = true;
            break;
        }
    }
    QVERIFY(foundManual);

    // Switch to auto
    QVariantMap status;
    status["state"] = "Idle";
    status["autoMode"] = true;
    svc.client.injectSimResponse(1, status);
    QApplication::processEvents();

    bool foundAuto = false;
    for (auto* l : labels) {
        if (l->text() == QStringLiteral("\u81EA\u52A8")) {  // 自动
            foundAuto = true;
            break;
        }
    }
    QVERIFY(foundAuto);
}

// === DeviceTableWidget Tests ===

void DesktopClientTest::testDeviceTableInitialEmpty()
{
    Services svc;
    DeviceTableWidget table(svc.deviceModel);

    auto* tw = table.findChild<QTableWidget*>();
    QVERIFY(tw);
    QCOMPARE(tw->rowCount(), 0);
}

void DesktopClientTest::testDeviceTablePopulate()
{
    Services svc;
    DeviceTableWidget table(svc.deviceModel);

    DeviceListModel::DeviceInfo info;
    info.deviceId = "cam_01";
    info.deviceType = "camera";
    info.status = "Ready";
    svc.deviceModel->addDevice(info);

    info.deviceId = "rob_01";
    info.deviceType = "robot";
    info.status = "Busy";
    svc.deviceModel->addDevice(info);

    QApplication::processEvents();

    auto* tw = table.findChild<QTableWidget*>();
    QVERIFY(tw);
    QCOMPARE(tw->rowCount(), 2);
}

void DesktopClientTest::testDeviceTableStatusColors()
{
    Services svc;
    DeviceTableWidget table(svc.deviceModel);

    DeviceListModel::DeviceInfo info;
    info.deviceId = "dev_err";
    info.deviceType = "plc";
    info.status = "Error";
    svc.deviceModel->addDevice(info);
    QApplication::processEvents();

    auto* tw = table.findChild<QTableWidget*>();
    QVERIFY(tw);
    QCOMPARE(tw->rowCount(), 1);

    // Status column (2) should have error color
    auto* statusItem = tw->item(0, 2);
    QVERIFY(statusItem);
    QCOMPARE(statusItem->foreground().color(), QColor("#c62828"));
}

void DesktopClientTest::testDeviceTableFilterByType()
{
    Services svc;
    DeviceTableWidget table(svc.deviceModel);

    DeviceListModel::DeviceInfo info;
    info.deviceId = "cam_01"; info.deviceType = "camera"; info.status = "Ready";
    svc.deviceModel->addDevice(info);
    info.deviceId = "rob_01"; info.deviceType = "robot"; info.status = "Ready";
    svc.deviceModel->addDevice(info);
    info.deviceId = "cam_02"; info.deviceType = "camera"; info.status = "Busy";
    svc.deviceModel->addDevice(info);
    QApplication::processEvents();

    auto* tw = table.findChild<QTableWidget*>();
    QCOMPARE(tw->rowCount(), 3);

    // Set type filter to "camera"
    auto* typeCombo = table.findChildren<QComboBox*>().first();
    QVERIFY(typeCombo);
    typeCombo->setCurrentIndex(1);  // camera
    QApplication::processEvents();

    QCOMPARE(tw->rowCount(), 2);
}

void DesktopClientTest::testDeviceTableFilterByStatus()
{
    Services svc;
    DeviceTableWidget table(svc.deviceModel);

    DeviceListModel::DeviceInfo info;
    info.deviceId = "d1"; info.deviceType = "camera"; info.status = "Ready";
    svc.deviceModel->addDevice(info);
    info.deviceId = "d2"; info.deviceType = "robot"; info.status = "Error";
    svc.deviceModel->addDevice(info);
    info.deviceId = "d3"; info.deviceType = "plc"; info.status = "Ready";
    svc.deviceModel->addDevice(info);
    QApplication::processEvents();

    auto comboBoxes = table.findChildren<QComboBox*>();
    QVERIFY(comboBoxes.size() >= 2);

    // Status filter is the second combo
    auto* statusCombo = comboBoxes[1];
    statusCombo->setCurrentIndex(1);  // Ready
    QApplication::processEvents();

    auto* tw = table.findChild<QTableWidget*>();
    QCOMPARE(tw->rowCount(), 2);
}

void DesktopClientTest::testDeviceTableDeviceAddRemove()
{
    Services svc;
    DeviceTableWidget table(svc.deviceModel);

    auto* tw = table.findChild<QTableWidget*>();

    DeviceListModel::DeviceInfo info;
    info.deviceId = "temp";
    info.deviceType = "sensor";
    info.status = "Ready";
    svc.deviceModel->addDevice(info);
    QApplication::processEvents();
    QCOMPARE(tw->rowCount(), 1);

    svc.deviceModel->removeDevice("temp");
    QApplication::processEvents();
    QCOMPARE(tw->rowCount(), 0);
}

// === ControlPanelWidget Tests ===

void DesktopClientTest::testControlPanelButtons()
{
    Services svc;
    ControlPanelWidget panel(&svc.client, svc.commandService, svc.statusProvider);

    auto buttons = panel.findChildren<QPushButton*>();
    QVERIFY(buttons.size() >= 3);  // start, stop, e-stop, load recipe

    // Verify e-stop button exists
    bool foundEStop = false;
    for (auto* btn : buttons) {
        if (btn->text().contains(QStringLiteral("\u7D27"))) {  // 紧
            foundEStop = true;
            break;
        }
    }
    QVERIFY(foundEStop);
}

void DesktopClientTest::testControlPanelSpeedSlider()
{
    Services svc;
    ControlPanelWidget panel(&svc.client, svc.commandService, svc.statusProvider);

    auto* slider = panel.findChild<QSlider*>();
    QVERIFY(slider);
    QCOMPARE(slider->minimum(), 10);
    QCOMPARE(slider->maximum(), 100);
    QCOMPARE(slider->value(), 100);
}

void DesktopClientTest::testControlPanelCommandLog()
{
    Services svc;
    ControlPanelWidget panel(&svc.client, svc.commandService, svc.statusProvider);

    auto* log = panel.findChild<QTextEdit*>();
    QVERIFY(log);
    QVERIFY(log->isReadOnly());
}

void DesktopClientTest::testControlPanelStartCycle()
{
    Services svc;
    ControlPanelWidget panel(&svc.client, svc.commandService, svc.statusProvider);

    QSignalSpy spy(svc.commandService, &RemoteCommandService::commandSent);

    // Find and click start button
    auto buttons = panel.findChildren<QPushButton*>();
    for (auto* btn : buttons) {
        if (btn->text().contains(QStringLiteral("\u542F\u52A8"))) {  // 启动
            btn->click();
            break;
        }
    }

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().at(1).toString(), QString("startCycle"));
}

void DesktopClientTest::testControlPanelStopCycle()
{
    Services svc;
    ControlPanelWidget panel(&svc.client, svc.commandService, svc.statusProvider);

    QSignalSpy spy(svc.commandService, &RemoteCommandService::commandSent);

    auto buttons = panel.findChildren<QPushButton*>();
    for (auto* btn : buttons) {
        if (btn->text() == QStringLiteral("\u505C\u6B62")) {  // 停止
            btn->click();
            break;
        }
    }

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().at(1).toString(), QString("stopCycle"));
}

void DesktopClientTest::testControlPanelEmergencyStop()
{
    Services svc;
    ControlPanelWidget panel(&svc.client, svc.commandService, svc.statusProvider);

    QSignalSpy spy(svc.commandService, &RemoteCommandService::commandSent);

    auto buttons = panel.findChildren<QPushButton*>();
    for (auto* btn : buttons) {
        if (btn->text().contains(QStringLiteral("\u7D27"))) {  // 紧急
            btn->click();
            break;
        }
    }

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().at(1).toString(), QString("emergencyStop"));
}

void DesktopClientTest::testControlPanelAutoMode()
{
    Services svc;
    ControlPanelWidget panel(&svc.client, svc.commandService, svc.statusProvider);

    QSignalSpy spy(svc.commandService, &RemoteCommandService::commandSent);

    auto* check = panel.findChild<QCheckBox*>();
    QVERIFY(check);
    QVERIFY(!check->isChecked());

    check->setChecked(true);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().at(1).toString(), QString("setAutoMode"));
}

void DesktopClientTest::testControlPanelLoadRecipe()
{
    Services svc;
    ControlPanelWidget panel(&svc.client, svc.commandService, svc.statusProvider);

    QSignalSpy spy(svc.commandService, &RemoteCommandService::commandSent);

    auto* input = panel.findChild<QLineEdit*>();
    QVERIFY(input);
    input->setText("recipe_abc");

    // Find the load button
    auto buttons = panel.findChildren<QPushButton*>();
    for (auto* btn : buttons) {
        if (btn->text() == QStringLiteral("\u52A0\u8F7D")) {  // 加载
            btn->click();
            break;
        }
    }

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().at(1).toString(), QString("loadRecipe"));
}

// === AlertWidget Tests ===

void DesktopClientTest::testAlertWidgetInitialEmpty()
{
    Services svc;
    AlertWidget widget(svc.alertService);

    auto* tw = widget.findChild<QTableWidget*>();
    QVERIFY(tw);
    QCOMPARE(tw->rowCount(), 0);
}

void DesktopClientTest::testAlertWidgetPopulate()
{
    Services svc;
    AlertWidget widget(svc.alertService);

    svc.alertService->addAlert(AlertNotificationService::Severity::Info, "src", "msg1");
    svc.alertService->addAlert(AlertNotificationService::Severity::Warning, "src", "msg2");
    svc.alertService->addAlert(AlertNotificationService::Severity::Error, "src", "msg3");
    QApplication::processEvents();

    auto* tw = widget.findChild<QTableWidget*>();
    QCOMPARE(tw->rowCount(), 3);
}

void DesktopClientTest::testAlertWidgetSeverityColors()
{
    Services svc;
    AlertWidget widget(svc.alertService);

    svc.alertService->addAlert(AlertNotificationService::Severity::Critical, "test", "critical msg");
    QApplication::processEvents();

    auto* tw = widget.findChild<QTableWidget*>();
    QCOMPARE(tw->rowCount(), 1);

    // Severity column (1) should be red for Critical
    auto* sevItem = tw->item(0, 1);
    QVERIFY(sevItem);
    QCOMPARE(sevItem->text(), QStringLiteral("\u4E25\u91CD"));  // 严重
    QCOMPARE(sevItem->foreground().color(), QColor("#c62828"));
}

void DesktopClientTest::testAlertWidgetAcknowledge()
{
    Services svc;
    AlertWidget widget(svc.alertService);

    svc.alertService->addAlert(AlertNotificationService::Severity::Error, "src", "err");
    QApplication::processEvents();

    QCOMPARE(svc.alertService->unacknowledgedCount(), 1);

    // Acknowledge via service
    svc.alertService->acknowledgeAlert(1);
    QApplication::processEvents();

    QCOMPARE(svc.alertService->unacknowledgedCount(), 0);

    auto* tw = widget.findChild<QTableWidget*>();
    auto* ackItem = tw->item(0, 5);
    QVERIFY(ackItem);
    QCOMPARE(ackItem->text(), QStringLiteral("\u5DF2\u786E\u8BA4"));  // 已确认
}

void DesktopClientTest::testAlertWidgetAcknowledgeAll()
{
    Services svc;
    AlertWidget widget(svc.alertService);

    svc.alertService->addAlert(AlertNotificationService::Severity::Info, "s", "m1");
    svc.alertService->addAlert(AlertNotificationService::Severity::Warning, "s", "m2");
    svc.alertService->addAlert(AlertNotificationService::Severity::Error, "s", "m3");
    QApplication::processEvents();

    QCOMPARE(svc.alertService->unacknowledgedCount(), 3);

    // Click acknowledge all
    auto buttons = widget.findChildren<QPushButton*>();
    for (auto* btn : buttons) {
        if (btn->text().contains(QStringLiteral("\u5168\u90E8\u786E\u8BA4"))) {  // 全部确认
            btn->click();
            break;
        }
    }
    QApplication::processEvents();

    QCOMPARE(svc.alertService->unacknowledgedCount(), 0);
}

void DesktopClientTest::testAlertWidgetClear()
{
    Services svc;
    AlertWidget widget(svc.alertService);

    svc.alertService->addAlert(AlertNotificationService::Severity::Info, "s", "m");
    QApplication::processEvents();

    auto* tw = widget.findChild<QTableWidget*>();
    QCOMPARE(tw->rowCount(), 1);

    // Click clear
    auto buttons = widget.findChildren<QPushButton*>();
    for (auto* btn : buttons) {
        if (btn->text() == QStringLiteral("\u6E05\u9664")) {  // 清除
            btn->click();
            break;
        }
    }
    QApplication::processEvents();

    QCOMPARE(tw->rowCount(), 0);
}

void DesktopClientTest::testAlertWidgetSeverityFilter()
{
    Services svc;
    AlertWidget widget(svc.alertService);

    svc.alertService->addAlert(AlertNotificationService::Severity::Info, "s", "info");
    svc.alertService->addAlert(AlertNotificationService::Severity::Error, "s", "error1");
    svc.alertService->addAlert(AlertNotificationService::Severity::Error, "s", "error2");
    QApplication::processEvents();

    auto* tw = widget.findChild<QTableWidget*>();
    QCOMPARE(tw->rowCount(), 3);

    // Filter by Error
    auto* combo = widget.findChild<QComboBox*>();
    QVERIFY(combo);
    combo->setCurrentIndex(3);  // Error
    QApplication::processEvents();

    QCOMPARE(tw->rowCount(), 2);
}

// === ConnectionDialog Tests ===

void DesktopClientTest::testConnectionDialogCreation()
{
    Services svc;
    ConnectionDialog dialog(&svc.client, svc.statusProvider);
    QVERIFY(dialog.windowTitle().contains(QStringLiteral("\u8FDE\u63A5")));
}

void DesktopClientTest::testConnectionDialogDefaults()
{
    Services svc;
    ConnectionDialog dialog(&svc.client, svc.statusProvider);

    auto* portSpin = dialog.findChild<QSpinBox*>();
    QVERIFY(portSpin);
    QCOMPARE(portSpin->minimum(), 1);
    QCOMPARE(portSpin->maximum(), 65535);
}

void DesktopClientTest::testConnectionDialogConnect()
{
    Services svc;
    ConnectionDialog dialog(&svc.client, svc.statusProvider);

    // Find host input and set value
    auto* hostEdit = dialog.findChild<QLineEdit*>();
    QVERIFY(hostEdit);
    hostEdit->setText("10.0.0.1");

    // Find connect button
    auto buttons = dialog.findChildren<QPushButton*>();
    bool foundConnect = false;
    for (auto* btn : buttons) {
        if (btn->text() == QStringLiteral("\u8FDE\u63A5")) {
            foundConnect = true;
            break;
        }
    }
    QVERIFY(foundConnect);
}

void DesktopClientTest::testConnectionDialogDisconnect()
{
    Services svc;
    ConnectionDialog dialog(&svc.client, svc.statusProvider);

    auto buttons = dialog.findChildren<QPushButton*>();
    bool foundDisconnect = false;
    for (auto* btn : buttons) {
        if (btn->text().contains(QStringLiteral("\u65AD\u5F00"))) {
            foundDisconnect = true;
            break;
        }
    }
    QVERIFY(foundDisconnect);
}

void DesktopClientTest::testConnectionDialogStateUpdate()
{
    Services svc;
    ConnectionDialog dialog(&svc.client, svc.statusProvider);

    auto* statusLabel = dialog.findChildren<QLabel*>().first();
    QVERIFY(statusLabel);
    // Should not crash when state changes
    emit svc.client.connected();
    QApplication::processEvents();
    emit svc.client.disconnected();
    QApplication::processEvents();
}

// === Integration Tests ===

void DesktopClientTest::testFullStatusUpdateFlow()
{
    Services svc;
    MainWindow win(&svc.client, svc.statusProvider, svc.commandService,
                   svc.alertService, svc.deviceModel);

    QVariantMap status;
    status["state"] = "Running";
    status["cycleCount"] = 50;
    status["successCount"] = 45;
    status["failCount"] = 5;
    status["autoMode"] = true;
    status["recipe"] = "PickAndPlace_v2";

    QVariantList devices;
    devices.append(QVariantMap{{"deviceId", "cam_01"}, {"deviceType", "camera"}, {"status", "Ready"}});
    devices.append(QVariantMap{{"deviceId", "rob_01"}, {"deviceType", "robot"}, {"status", "Busy"}});
    status["devices"] = devices;

    svc.client.injectSimResponse(1, status);
    QApplication::processEvents();

    // Verify state propagation
    QCOMPARE(svc.statusProvider->workcellState(), WorkcellStatusProvider::WorkcellState::Running);
    QCOMPARE(svc.statusProvider->cycleCount(), 50);
    QCOMPARE(svc.statusProvider->successCount(), 45);
    QCOMPARE(svc.statusProvider->failCount(), 5);
    QVERIFY(svc.statusProvider->isAutoMode());
    QCOMPARE(svc.statusProvider->currentRecipe(), QString("PickAndPlace_v2"));
}

void DesktopClientTest::testDeviceListFromServer()
{
    Services svc;
    DeviceTableWidget table(svc.deviceModel);

    QVariantMap status;
    status["state"] = "Idle";
    QVariantList devices;
    devices.append(QVariantMap{{"deviceId", "s1"}, {"deviceType", "sensor"}, {"status", "Ready"}});
    devices.append(QVariantMap{{"deviceId", "s2"}, {"deviceType", "sensor"}, {"status", "Error"}});
    status["devices"] = devices;

    svc.client.injectSimResponse(1, status);
    QApplication::processEvents();

    QCOMPARE(svc.deviceModel->deviceCount(), 2);
    auto* tw = table.findChild<QTableWidget*>();
    QCOMPARE(tw->rowCount(), 2);
}

void DesktopClientTest::testAlertFromServer()
{
    Services svc;
    AlertWidget widget(svc.alertService);

    svc.alertService->processServerAlert(QVariantMap{
        {"severity", "critical"},
        {"source", "robot"},
        {"message", "Collision detected"}
    });
    QApplication::processEvents();

    QCOMPARE(svc.alertService->alertCount(), 1);
    auto alert = svc.alertService->alertById(1);
    QCOMPARE(alert.severity, AlertNotificationService::Severity::Critical);
    QCOMPARE(alert.source, QString("robot"));
    QCOMPARE(alert.message, QString("Collision detected"));

    auto* tw = widget.findChild<QTableWidget*>();
    QCOMPARE(tw->rowCount(), 1);
}

QTEST_MAIN(DesktopClientTest)
#include "test_desktopclient.moc"
