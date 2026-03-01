#ifndef WORKFLOWENGINE_H
#define WORKFLOWENGINE_H

#include <QObject>
#include <QMap>
#include <QVariantMap>
#include <QJsonObject>

class WorkcellController;

struct WorkflowNode {
    QString id;
    QString type;              // "Capture", "Detect", "Pick", "Place", "Wait", "Branch"
    QVariantMap parameters;
    QStringList nextNodes;     // Default successors
    QMap<QString, QStringList> conditionalNext;  // condition -> next nodes
};

struct WorkflowDefinition {
    QString id;
    QString name;
    QString startNodeId;
    QMap<QString, WorkflowNode> nodes;
};

class WorkflowEngine : public QObject {
    Q_OBJECT
public:
    enum class Status { Idle, Running, Paused, Completed, Error };

    explicit WorkflowEngine(QObject* parent = nullptr);

    void loadWorkflow(const WorkflowDefinition& definition);
    static WorkflowDefinition fromJson(const QJsonObject& json);

    void setWorkcellController(WorkcellController* controller);

    void start();
    void pause();
    void resume();
    void abort();

    QString currentNodeId() const;
    Status status() const;

signals:
    void nodeStarted(const QString& nodeId, const QString& nodeType);
    void nodeCompleted(const QString& nodeId, const QVariant& result);
    void nodeError(const QString& nodeId, const QString& error);
    void workflowFinished(bool success);
    void statusChanged(WorkflowEngine::Status status);

private:
    void executeNode(const QString& nodeId);
    void advanceToNext(const QString& currentNodeId, const QString& condition = QString());

    WorkflowDefinition m_definition;
    QString m_currentNodeId;
    Status m_status = Status::Idle;
    WorkcellController* m_controller = nullptr;
};

#endif // WORKFLOWENGINE_H
