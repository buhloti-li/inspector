#ifndef ICAMERADRIVER_H
#define ICAMERADRIVER_H

#include "idevicedriver.h"
#include <vector>
#include <cstdint>

struct PointCloud {
    std::vector<float> points;    // x,y,z interleaved
    std::vector<uint8_t> colors;  // r,g,b interleaved (optional)
    int width = 0;
    int height = 0;

    int pointCount() const { return static_cast<int>(points.size()) / 3; }
    bool empty() const { return points.empty(); }
};

struct ColorImage {
    std::vector<uint8_t> data;  // RGB interleaved
    int width = 0;
    int height = 0;
    bool empty() const { return data.empty(); }
};

struct CaptureConfig {
    double exposureTime = 15.0;  // ms
    int roiX = 0;
    int roiY = 0;
    int roiW = 0;  // 0 = full frame
    int roiH = 0;
    bool enableHDR = false;
};

class ICameraDriver : public IDeviceDriver {
public:
    QString deviceType() const override { return QStringLiteral("Camera"); }

    virtual bool capture(const CaptureConfig& config = CaptureConfig{}) = 0;
    virtual PointCloud getPointCloud() const = 0;
    virtual ColorImage getColorImage() const = 0;
};

#endif // ICAMERADRIVER_H
