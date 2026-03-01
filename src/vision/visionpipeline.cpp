#include "visionpipeline.h"
#include <algorithm>
#include <cmath>
#include <QMap>

VisionPipeline::VisionPipeline(QObject* parent)
    : QObject(parent) {
}

void VisionPipeline::setCameraDriver(ICameraDriver* camera) {
    m_camera = camera;
}

void VisionPipeline::setModelPath(const QString& onnxModelPath) {
    m_modelPath = onnxModelPath;
}

void VisionPipeline::setMinConfidence(float confidence) {
    m_minConfidence = confidence;
}

QVector<DetectionResult> VisionPipeline::execute() {
    emit pipelineStarted();

    PointCloud cloud = captureAndPreprocess();
    if (cloud.empty()) {
        emit pipelineError(QStringLiteral("CaptureEmpty"));
        emit pipelineFinished(0);
        return {};
    }

    auto results = detect(cloud);
    results = postProcess(results, m_minConfidence);

    emit pipelineFinished(results.size());
    return results;
}

PointCloud VisionPipeline::captureAndPreprocess() {
    if (!m_camera) {
        emit pipelineError(QStringLiteral("NoCameraConfigured"));
        return {};
    }

    CaptureConfig config;
    if (!m_camera->capture(config)) {
        emit pipelineError(QStringLiteral("CaptureFailed"));
        return {};
    }

    PointCloud cloud = m_camera->getPointCloud();

    // Preprocess: remove NaN points and statistical outliers
    PointCloud filtered;
    filtered.width = cloud.width;
    filtered.height = cloud.height;

    for (int i = 0; i < cloud.pointCount(); ++i) {
        float x = cloud.points[i * 3 + 0];
        float y = cloud.points[i * 3 + 1];
        float z = cloud.points[i * 3 + 2];

        if (std::isnan(x) || std::isnan(y) || std::isnan(z))
            continue;

        // Basic z-range filter (workspace bounds)
        if (z < 100.0f || z > 800.0f)
            continue;

        filtered.points.push_back(x);
        filtered.points.push_back(y);
        filtered.points.push_back(z);
    }

    return filtered;
}

QVector<DetectionResult> VisionPipeline::detect(const PointCloud& cloud) {
    // In production, this would invoke an ONNX model.
    // For now, use simple Euclidean clustering as a placeholder.
    return clusterDetect(cloud);
}

QVector<DetectionResult> VisionPipeline::postProcess(const QVector<DetectionResult>& raw,
                                                      float minConfidence) {
    QVector<DetectionResult> filtered;
    for (const auto& r : raw) {
        if (r.confidence >= minConfidence)
            filtered.append(r);
    }

    // Sort by confidence descending (best first)
    std::sort(filtered.begin(), filtered.end(),
              [](const DetectionResult& a, const DetectionResult& b) {
                  return a.confidence > b.confidence;
              });

    return filtered;
}

// Simple grid-based clustering for simulation
QVector<DetectionResult> VisionPipeline::clusterDetect(const PointCloud& cloud) {
    if (cloud.empty())
        return {};

    // Voxelize into coarse grid to find clusters
    const float gridSize = 40.0f;
    struct Cell {
        float sumX = 0, sumY = 0, sumZ = 0;
        int count = 0;
    };

    QMap<qint64, Cell> grid;

    for (int i = 0; i < cloud.pointCount(); ++i) {
        float x = cloud.points[i * 3 + 0];
        float y = cloud.points[i * 3 + 1];
        float z = cloud.points[i * 3 + 2];

        // Skip background plane (high z values)
        if (z > 580.0f)
            continue;

        int gx = static_cast<int>(std::floor(x / gridSize));
        int gy = static_cast<int>(std::floor(y / gridSize));
        int gz = static_cast<int>(std::floor(z / gridSize));

        qint64 key = (static_cast<qint64>(gx) * 10000 + gy) * 10000 + gz;
        auto& cell = grid[key];
        cell.sumX += x;
        cell.sumY += y;
        cell.sumZ += z;
        cell.count++;
    }

    // Each cell with enough points becomes a detection
    QVector<DetectionResult> results;
    const int minClusterSize = 50;

    for (auto it = grid.begin(); it != grid.end(); ++it) {
        const Cell& cell = it.value();
        if (cell.count < minClusterSize)
            continue;

        DetectionResult det;
        det.objectClass = QStringLiteral("unknown_part");
        det.confidence = std::min(1.0f, static_cast<float>(cell.count) / 200.0f);
        det.pose6D.x = cell.sumX / cell.count;
        det.pose6D.y = cell.sumY / cell.count;
        det.pose6D.z = cell.sumZ / cell.count;
        det.pose6D.rx = 0;
        det.pose6D.ry = 0;
        det.pose6D.rz = 0;

        results.append(det);
    }

    return results;
}
