#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include <QObject>
#include <QMap>
#include <map>
#include <memory>

#include "idevicedriver.h"
#include "icameradriver.h"
#include "irobotdriver.h"
#include "iplcdriver.h"

class DeviceManager : public QObject {
    Q_OBJECT
public:
    explicit DeviceManager(QObject* parent = nullptr);

    bool registerDevice(const QString& id, std::unique_ptr<IDeviceDriver> driver);
    void removeDevice(const QString& id);

    IDeviceDriver* device(const QString& id) const;
    ICameraDriver* camera(const QString& id) const;
    IRobotDriver* robot(const QString& id) const;
    IPLCDriver* plc(const QString& id) const;

    QStringList deviceIds() const;
    QMap<QString, IDeviceDriver::DeviceStatus> allStatus() const;

signals:
    void deviceRegistered(const QString& id);
    void deviceRemoved(const QString& id);
    void deviceStatusChanged(const QString& id, IDeviceDriver::DeviceStatus status);

private:
    std::map<QString, std::unique_ptr<IDeviceDriver>> m_devices;
};

#endif // DEVICEMANAGER_H
