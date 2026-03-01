#pragma once

#include <QMainWindow>
#include <QTabWidget>
#include <QLabel>
#include <QTimer>

class MobileClient;
class WorkcellStatusProvider;
class RemoteCommandService;
class AlertNotificationService;
class DeviceListModel;

class DashboardWidget;
class DeviceTableWidget;
class ControlPanelWidget;
class AlertWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(MobileClient* client,
                        WorkcellStatusProvider* statusProvider,
                        RemoteCommandService* commandService,
                        AlertNotificationService* alertService,
                        DeviceListModel* deviceModel,
                        QWidget* parent = nullptr);

private slots:
    void onConnected();
    void onDisconnected();
    void onStateChanged();
    void onAlertReceived();
    void showConnectionDialog();

private:
    void setupUi();
    void setupMenuBar();
    void setupStatusBar();
    void updateConnectionIndicator(bool connected);

    MobileClient* m_client;
    WorkcellStatusProvider* m_statusProvider;
    RemoteCommandService* m_commandService;
    AlertNotificationService* m_alertService;
    DeviceListModel* m_deviceModel;

    QTabWidget* m_tabWidget;
    DashboardWidget* m_dashboardWidget;
    DeviceTableWidget* m_deviceTableWidget;
    ControlPanelWidget* m_controlPanelWidget;
    AlertWidget* m_alertWidget;

    QLabel* m_connectionLabel;
    QLabel* m_stateLabel;
};
