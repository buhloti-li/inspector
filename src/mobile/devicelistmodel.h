#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QVariantMap>
#include <QVariantList>

class MobileClient;

/// Data model for displaying device list on mobile UI.
/// Receives device status updates from the server and maintains a local list.
class DeviceListModel : public QObject {
    Q_OBJECT
public:
    struct DeviceInfo {
        QString deviceId;
        QString deviceType; // "camera", "robot", "plc"
        QString status;     // "Ready", "Busy", "Error", "Disconnected"
        QVariantMap properties;
    };

    explicit DeviceListModel(MobileClient* client, QObject* parent = nullptr);

    // Device access
    int deviceCount() const;
    DeviceInfo deviceAt(int index) const;
    DeviceInfo deviceById(const QString& id) const;
    bool hasDevice(const QString& id) const;
    QVector<DeviceInfo> allDevices() const;
    QVector<DeviceInfo> devicesByType(const QString& type) const;
    QVector<DeviceInfo> devicesByStatus(const QString& status) const;

    // Update from server data
    void updateDeviceList(const QVariantList& deviceList);
    void updateDeviceStatus(const QString& deviceId, const QString& status);
    void addDevice(const DeviceInfo& device);
    void removeDevice(const QString& deviceId);
    void clear();

signals:
    void deviceAdded(const QString& deviceId);
    void deviceRemoved(const QString& deviceId);
    void deviceStatusChanged(const QString& deviceId, const QString& oldStatus,
                              const QString& newStatus);
    void deviceListUpdated();

private slots:
    void onStatusUpdated(const QVariantMap& status);

private:
    int indexOfDevice(const QString& id) const;

    MobileClient* m_client;
    QVector<DeviceInfo> m_devices;
};
