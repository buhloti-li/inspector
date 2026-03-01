#ifndef IDEVICEDRIVER_H
#define IDEVICEDRIVER_H

#include <QString>
#include <QVariantMap>

class IDeviceDriver {
public:
    enum class DeviceStatus { Disconnected, Connected, Ready, Error };

    virtual ~IDeviceDriver() = default;

    virtual bool connect(const QVariantMap& config) = 0;
    virtual void disconnect() = 0;
    virtual DeviceStatus status() const = 0;
    virtual QString deviceId() const = 0;
    virtual QString deviceType() const = 0;
};

#endif // IDEVICEDRIVER_H
