#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "mobile/mobileclient.h"
#include "mobile/workcellstatusprovider.h"
#include "mobile/remotecommandservice.h"
#include "mobile/alertnotificationservice.h"
#include "mobile/devicelistmodel.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("IndustrialMonitor"));
    app.setOrganizationName(QStringLiteral("IndustrialRuntime"));
    app.setApplicationVersion(QStringLiteral("1.0.0"));

    // Create core services
    MobileClient client;
    client.setSimulationMode(false);

    WorkcellStatusProvider statusProvider(&client);
    RemoteCommandService commandService(&client);
    AlertNotificationService alertService(&client);
    DeviceListModel deviceModel(&client);

    // Set up QML engine
    QQmlApplicationEngine engine;

    // Expose C++ objects to QML
    engine.rootContext()->setContextProperty("mobileClient", &client);
    engine.rootContext()->setContextProperty("statusProvider", &statusProvider);
    engine.rootContext()->setContextProperty("commandService", &commandService);
    engine.rootContext()->setContextProperty("alertService", &alertService);
    engine.rootContext()->setContextProperty("deviceModel", &deviceModel);

    const QUrl url(QStringLiteral("qrc:/qml/Main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
