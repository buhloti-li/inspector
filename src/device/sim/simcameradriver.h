#ifndef SIMCAMERADRIVER_H
#define SIMCAMERADRIVER_H

#include "../icameradriver.h"
#include <random>

class SimCameraDriver : public ICameraDriver {
public:
    explicit SimCameraDriver(const QString& id = QStringLiteral("sim-camera-01"));

    // IDeviceDriver
    bool connect(const QVariantMap& config) override;
    void disconnect() override;
    DeviceStatus status() const override;
    QString deviceId() const override;

    // ICameraDriver
    bool capture(const CaptureConfig& config) override;
    PointCloud getPointCloud() const override;
    ColorImage getColorImage() const override;

    // Simulation controls
    void setObjectCount(int count);
    void setNoiseLevel(double noise);

private:
    void generateSimulatedScene(int width, int height);

    QString m_id;
    DeviceStatus m_status = DeviceStatus::Disconnected;
    PointCloud m_lastCloud;
    int m_objectCount = 3;
    double m_noiseLevel = 0.01;
    std::mt19937 m_rng;
};

#endif // SIMCAMERADRIVER_H
