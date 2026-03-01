#ifndef VISIONPIPELINE_H
#define VISIONPIPELINE_H

#include <QObject>
#include <QVector>

#include "../device/icameradriver.h"
#include "../device/irobotdriver.h"

struct Vec3 {
    float x = 0, y = 0, z = 0;
};

struct DetectionResult {
    QString objectClass;
    float confidence = 0.0f;
    CartesianPose pose6D;
    QVector<Vec3> boundingBox;  // 8 corners of 3D bounding box
};

class VisionPipeline : public QObject {
    Q_OBJECT
public:
    explicit VisionPipeline(QObject* parent = nullptr);

    void setCameraDriver(ICameraDriver* camera);
    void setModelPath(const QString& onnxModelPath);
    void setMinConfidence(float confidence);

    // Execute full pipeline: capture -> preprocess -> detect -> postprocess
    QVector<DetectionResult> execute();

    // Step-by-step interfaces for debugging
    PointCloud captureAndPreprocess();
    QVector<DetectionResult> detect(const PointCloud& cloud);

    // Filter and sort results
    static QVector<DetectionResult> postProcess(const QVector<DetectionResult>& raw,
                                                 float minConfidence);

signals:
    void pipelineStarted();
    void pipelineFinished(int objectCount);
    void pipelineError(const QString& error);

private:
    // Simple clustering-based detection (placeholder for real ML model)
    QVector<DetectionResult> clusterDetect(const PointCloud& cloud);

    ICameraDriver* m_camera = nullptr;
    QString m_modelPath;
    float m_minConfidence = 0.5f;
};

#endif // VISIONPIPELINE_H
