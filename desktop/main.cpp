#include <QApplication>
#include "mainwindow.h"
#include "mobile/mobileclient.h"
#include "mobile/workcellstatusprovider.h"
#include "mobile/remotecommandservice.h"
#include "mobile/alertnotificationservice.h"
#include "mobile/devicelistmodel.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("IndustrialDesktop"));
    app.setOrganizationName(QStringLiteral("IndustrialRuntime"));
    app.setApplicationVersion(QStringLiteral("1.0.0"));

    // Global stylesheet for Windows native feel with modern touches
    app.setStyleSheet(
        "QMainWindow { background: #f5f5f5; }"
        "QTabWidget::pane { border: 1px solid #ddd; background: white; }"
        "QTabBar::tab { padding: 8px 20px; font-size: 13px; }"
        "QTabBar::tab:selected { background: white; border-bottom: 2px solid #1a73e8; font-weight: bold; }"
        "QTabBar::tab:!selected { background: #f0f0f0; }"
        "QGroupBox { font-weight: bold; border: 1px solid #e0e0e0; border-radius: 6px; "
        "  margin-top: 8px; padding-top: 16px; background: white; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 4px; }");

    // Create core services (reuse the same service layer as mobile)
    MobileClient client;
    client.setSimulationMode(false);

    WorkcellStatusProvider statusProvider(&client);
    RemoteCommandService commandService(&client);
    AlertNotificationService alertService(&client);
    DeviceListModel deviceModel(&client);

    // Create and show main window
    MainWindow mainWindow(&client, &statusProvider, &commandService, &alertService, &deviceModel);
    mainWindow.show();

    return app.exec();
}
