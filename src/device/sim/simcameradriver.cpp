#include "simcameradriver.h"
#include <cmath>

SimCameraDriver::SimCameraDriver(const QString& id)
    : m_id(id)
    , m_rng(std::random_device{}()) {
}

bool SimCameraDriver::connect(const QVariantMap& config) {
    Q_UNUSED(config);
    if (m_status != DeviceStatus::Disconnected)
        return false;
    m_status = DeviceStatus::Ready;
    return true;
}

void SimCameraDriver::disconnect() {
    m_status = DeviceStatus::Disconnected;
    m_lastCloud = PointCloud{};
}

IDeviceDriver::DeviceStatus SimCameraDriver::status() const {
    return m_status;
}

QString SimCameraDriver::deviceId() const {
    return m_id;
}

bool SimCameraDriver::capture(const CaptureConfig& config) {
    if (m_status != DeviceStatus::Ready)
        return false;

    int w = config.roiW > 0 ? config.roiW : 640;
    int h = config.roiH > 0 ? config.roiH : 480;
    generateSimulatedScene(w, h);
    return true;
}

PointCloud SimCameraDriver::getPointCloud() const {
    return m_lastCloud;
}

ColorImage SimCameraDriver::getColorImage() const {
    ColorImage img;
    img.width = 640;
    img.height = 480;
    img.data.resize(640 * 480 * 3, 128); // gray placeholder
    return img;
}

void SimCameraDriver::setObjectCount(int count) {
    m_objectCount = count;
}

void SimCameraDriver::setNoiseLevel(double noise) {
    m_noiseLevel = noise;
}

void SimCameraDriver::generateSimulatedScene(int width, int height) {
    m_lastCloud = PointCloud{};
    m_lastCloud.width = width;
    m_lastCloud.height = height;

    std::uniform_real_distribution<float> noiseDist(-static_cast<float>(m_noiseLevel),
                                                     static_cast<float>(m_noiseLevel));
    std::uniform_real_distribution<float> xDist(-200.0f, 200.0f);
    std::uniform_real_distribution<float> yDist(-200.0f, 200.0f);
    std::uniform_real_distribution<float> zDist(300.0f, 600.0f);

    // Generate background plane points
    int bgPoints = (width * height) / 10;
    for (int i = 0; i < bgPoints; ++i) {
        m_lastCloud.points.push_back(xDist(m_rng) + noiseDist(m_rng));
        m_lastCloud.points.push_back(yDist(m_rng) + noiseDist(m_rng));
        m_lastCloud.points.push_back(600.0f + noiseDist(m_rng)); // flat bg at z=600
    }

    // Generate object clusters
    for (int obj = 0; obj < m_objectCount; ++obj) {
        float cx = xDist(m_rng);
        float cy = yDist(m_rng);
        float cz = zDist(m_rng);

        std::normal_distribution<float> clusterDist(0.0f, 15.0f);
        int clusterSize = 200;
        for (int i = 0; i < clusterSize; ++i) {
            m_lastCloud.points.push_back(cx + clusterDist(m_rng) + noiseDist(m_rng));
            m_lastCloud.points.push_back(cy + clusterDist(m_rng) + noiseDist(m_rng));
            m_lastCloud.points.push_back(cz + clusterDist(m_rng) + noiseDist(m_rng));
        }
    }
}
