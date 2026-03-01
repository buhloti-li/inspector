#include <QCoreApplication>
#include <QCommandLineParser>
#include <QTextStream>
#include "server/workcellserver.h"
#include "workcellcontroller.h"
#include "device/devicemanager.h"
#include "device/sim/simcameradriver.h"
#include "device/sim/simrobotdriver.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    app.setApplicationName("industrial-server");
    app.setApplicationVersion("1.0.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("Industrial Workcell Monitoring Server");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption portOption({"p", "port"}, "TCP port to listen on (default: 9600)", "port", "9600");
    QCommandLineOption simOption({"s", "sim"}, "Run with simulated devices");
    parser.addOption(portOption);
    parser.addOption(simOption);
    parser.process(app);

    quint16 port = static_cast<quint16>(parser.value(portOption).toUInt());

    QTextStream out(stdout);
    out << "=== Industrial Workcell Server v1.0.0 ===" << Qt::endl;

    // Create controller
    WorkcellController controller;

    // Set up simulated devices if requested
    DeviceManager deviceManager;
    SimCameraDriver* simCam = nullptr;
    SimRobotDriver* simRobot = nullptr;

    if (parser.isSet(simOption)) {
        out << "[SIM] Creating simulated devices..." << Qt::endl;
        auto camPtr = std::make_unique<SimCameraDriver>("sim_camera_01");
        auto robPtr = std::make_unique<SimRobotDriver>("sim_robot_01");
        simCam = camPtr.get();
        simRobot = robPtr.get();

        simCam->connect({});
        simRobot->connect({});

        deviceManager.registerDevice("sim_camera_01", std::move(camPtr));
        deviceManager.registerDevice("sim_robot_01", std::move(robPtr));

        controller.setDeviceManager(&deviceManager);
        controller.bindCamera("sim_camera_01");
        controller.bindRobot("sim_robot_01");
        out << "[SIM] Devices ready: sim_camera_01, sim_robot_01" << Qt::endl;
    }

    // Create and start server
    WorkcellServer server;
    server.setController(&controller);

    QObject::connect(&server, &WorkcellServer::clientConnected, [&out](const QString& addr) {
        out << "[CONN] Client connected: " << addr << Qt::endl;
    });
    QObject::connect(&server, &WorkcellServer::clientDisconnected, [&out](const QString& addr) {
        out << "[CONN] Client disconnected: " << addr << Qt::endl;
    });
    QObject::connect(&server, &WorkcellServer::commandProcessed,
                     [&out](const QString& cmd, int id) {
        out << "[CMD] " << cmd << " (id=" << id << ")" << Qt::endl;
    });

    if (!server.start(port)) {
        out << "[ERROR] Failed to start server on port " << port << Qt::endl;
        return 1;
    }

    out << "[OK] Listening on port " << server.serverPort() << Qt::endl;
    out << "Press Ctrl+C to stop." << Qt::endl;

    return app.exec();
}
