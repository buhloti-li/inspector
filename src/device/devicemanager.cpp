#include "devicemanager.h"

DeviceManager::DeviceManager(QObject* parent)
    : QObject(parent) {
}

bool DeviceManager::registerDevice(const QString& id, std::unique_ptr<IDeviceDriver> driver) {
    if (m_devices.count(id) > 0 || !driver)
        return false;

    m_devices.emplace(id, std::move(driver));
    emit deviceRegistered(id);
    return true;
}

void DeviceManager::removeDevice(const QString& id) {
    if (m_devices.erase(id) > 0)
        emit deviceRemoved(id);
}

IDeviceDriver* DeviceManager::device(const QString& id) const {
    auto it = m_devices.find(id);
    return it != m_devices.end() ? it->second.get() : nullptr;
}

ICameraDriver* DeviceManager::camera(const QString& id) const {
    return dynamic_cast<ICameraDriver*>(device(id));
}

IRobotDriver* DeviceManager::robot(const QString& id) const {
    return dynamic_cast<IRobotDriver*>(device(id));
}

IPLCDriver* DeviceManager::plc(const QString& id) const {
    return dynamic_cast<IPLCDriver*>(device(id));
}

QStringList DeviceManager::deviceIds() const {
    QStringList ids;
    for (const auto& pair : m_devices)
        ids.append(pair.first);
    return ids;
}

QMap<QString, IDeviceDriver::DeviceStatus> DeviceManager::allStatus() const {
    QMap<QString, IDeviceDriver::DeviceStatus> result;
    for (const auto& pair : m_devices) {
        result.insert(pair.first, pair.second->status());
    }
    return result;
}
