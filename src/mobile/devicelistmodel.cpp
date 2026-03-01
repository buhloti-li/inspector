#include "devicelistmodel.h"
#include "mobileclient.h"

DeviceListModel::DeviceListModel(MobileClient* client, QObject* parent)
    : QObject(parent)
    , m_client(client) {
    QObject::connect(m_client, &MobileClient::statusUpdated,
                     this, &DeviceListModel::onStatusUpdated);
}

int DeviceListModel::deviceCount() const {
    return m_devices.size();
}

DeviceListModel::DeviceInfo DeviceListModel::deviceAt(int index) const {
    if (index < 0 || index >= m_devices.size())
        return DeviceInfo{};
    return m_devices.at(index);
}

DeviceListModel::DeviceInfo DeviceListModel::deviceById(const QString& id) const {
    int idx = indexOfDevice(id);
    if (idx < 0)
        return DeviceInfo{};
    return m_devices.at(idx);
}

bool DeviceListModel::hasDevice(const QString& id) const {
    return indexOfDevice(id) >= 0;
}

QVector<DeviceListModel::DeviceInfo> DeviceListModel::allDevices() const {
    return m_devices;
}

QVector<DeviceListModel::DeviceInfo> DeviceListModel::devicesByType(const QString& type) const {
    QVector<DeviceInfo> result;
    for (const auto& d : m_devices) {
        if (d.deviceType == type)
            result.append(d);
    }
    return result;
}

QVector<DeviceListModel::DeviceInfo> DeviceListModel::devicesByStatus(const QString& status) const {
    QVector<DeviceInfo> result;
    for (const auto& d : m_devices) {
        if (d.status == status)
            result.append(d);
    }
    return result;
}

void DeviceListModel::updateDeviceList(const QVariantList& deviceList) {
    m_devices.clear();
    for (const auto& var : deviceList) {
        QVariantMap map = var.toMap();
        DeviceInfo info;
        info.deviceId = map.value(QStringLiteral("deviceId")).toString();
        info.deviceType = map.value(QStringLiteral("deviceType")).toString();
        info.status = map.value(QStringLiteral("status")).toString();
        info.properties = map.value(QStringLiteral("properties")).toMap();
        m_devices.append(info);
    }
    emit deviceListUpdated();
}

void DeviceListModel::updateDeviceStatus(const QString& deviceId, const QString& status) {
    int idx = indexOfDevice(deviceId);
    if (idx < 0)
        return;

    QString oldStatus = m_devices[idx].status;
    if (oldStatus != status) {
        m_devices[idx].status = status;
        emit deviceStatusChanged(deviceId, oldStatus, status);
    }
}

void DeviceListModel::addDevice(const DeviceInfo& device) {
    if (hasDevice(device.deviceId))
        return;

    m_devices.append(device);
    emit deviceAdded(device.deviceId);
}

void DeviceListModel::removeDevice(const QString& deviceId) {
    int idx = indexOfDevice(deviceId);
    if (idx < 0)
        return;

    m_devices.removeAt(idx);
    emit deviceRemoved(deviceId);
}

void DeviceListModel::clear() {
    m_devices.clear();
    emit deviceListUpdated();
}

void DeviceListModel::onStatusUpdated(const QVariantMap& status) {
    // Check if the status contains device list
    if (status.contains(QStringLiteral("devices"))) {
        QVariantList devices = status.value(QStringLiteral("devices")).toList();
        updateDeviceList(devices);
    }
}

int DeviceListModel::indexOfDevice(const QString& id) const {
    for (int i = 0; i < m_devices.size(); ++i) {
        if (m_devices.at(i).deviceId == id)
            return i;
    }
    return -1;
}
