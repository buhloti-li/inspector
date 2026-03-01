#include "workflowengine.h"
#include "../workcellcontroller.h"

#include <QJsonArray>

WorkflowEngine::WorkflowEngine(QObject* parent)
    : QObject(parent) {
}

void WorkflowEngine::loadWorkflow(const WorkflowDefinition& definition) {
    m_definition = definition;
    m_currentNodeId.clear();
    m_status = Status::Idle;
    emit statusChanged(m_status);
}

WorkflowDefinition WorkflowEngine::fromJson(const QJsonObject& json) {
    WorkflowDefinition def;
    def.id = json.value(QStringLiteral("id")).toString();
    def.name = json.value(QStringLiteral("name")).toString();
    def.startNodeId = json.value(QStringLiteral("startNodeId")).toString();

    QJsonObject nodesObj = json.value(QStringLiteral("nodes")).toObject();
    for (auto it = nodesObj.begin(); it != nodesObj.end(); ++it) {
        QJsonObject nodeObj = it.value().toObject();
        WorkflowNode node;
        node.id = it.key();
        node.type = nodeObj.value(QStringLiteral("type")).toString();
        node.parameters = nodeObj.value(QStringLiteral("parameters")).toObject().toVariantMap();

        QJsonArray nextArr = nodeObj.value(QStringLiteral("nextNodes")).toArray();
        for (const auto& v : nextArr)
            node.nextNodes.append(v.toString());

        QJsonObject condObj = nodeObj.value(QStringLiteral("conditionalNext")).toObject();
        for (auto cit = condObj.begin(); cit != condObj.end(); ++cit) {
            QStringList targets;
            QJsonArray arr = cit.value().toArray();
            for (const auto& v : arr)
                targets.append(v.toString());
            node.conditionalNext.insert(cit.key(), targets);
        }

        def.nodes.insert(node.id, node);
    }

    return def;
}

void WorkflowEngine::setWorkcellController(WorkcellController* controller) {
    m_controller = controller;
}

void WorkflowEngine::start() {
    if (m_definition.startNodeId.isEmpty() || m_definition.nodes.isEmpty()) {
        m_status = Status::Error;
        emit statusChanged(m_status);
        return;
    }

    m_status = Status::Running;
    emit statusChanged(m_status);
    executeNode(m_definition.startNodeId);
}

void WorkflowEngine::pause() {
    if (m_status == Status::Running) {
        m_status = Status::Paused;
        emit statusChanged(m_status);
    }
}

void WorkflowEngine::resume() {
    if (m_status == Status::Paused) {
        m_status = Status::Running;
        emit statusChanged(m_status);
        // Re-execute current node
        if (!m_currentNodeId.isEmpty())
            executeNode(m_currentNodeId);
    }
}

void WorkflowEngine::abort() {
    m_status = Status::Idle;
    m_currentNodeId.clear();
    emit statusChanged(m_status);
    emit workflowFinished(false);
}

QString WorkflowEngine::currentNodeId() const {
    return m_currentNodeId;
}

WorkflowEngine::Status WorkflowEngine::status() const {
    return m_status;
}

void WorkflowEngine::executeNode(const QString& nodeId) {
    if (m_status != Status::Running)
        return;

    auto it = m_definition.nodes.find(nodeId);
    if (it == m_definition.nodes.end()) {
        m_status = Status::Error;
        emit nodeError(nodeId, QStringLiteral("NodeNotFound"));
        emit statusChanged(m_status);
        return;
    }

    m_currentNodeId = nodeId;
    const WorkflowNode& node = it.value();
    emit nodeStarted(nodeId, node.type);

    // Dispatch based on node type
    // In a full implementation, each type would call the corresponding
    // subsystem (Vision, Robot, PLC, etc.) through the WorkcellController.
    // Here we provide the execution framework.

    QString condition;

    if (node.type == QStringLiteral("Capture")) {
        // Trigger camera capture via controller
        emit nodeCompleted(nodeId, QVariant());

    } else if (node.type == QStringLiteral("Detect")) {
        // Run vision detection
        emit nodeCompleted(nodeId, QVariant());

    } else if (node.type == QStringLiteral("Pick")) {
        // Execute pick
        if (m_controller) {
            m_controller->recordPickResult(true); // placeholder
        }
        emit nodeCompleted(nodeId, QVariant(true));

    } else if (node.type == QStringLiteral("Place")) {
        // Execute place
        emit nodeCompleted(nodeId, QVariant(true));

    } else if (node.type == QStringLiteral("Wait")) {
        // Wait for signal or timeout
        emit nodeCompleted(nodeId, QVariant());

    } else if (node.type == QStringLiteral("Branch")) {
        // Evaluate condition from parameters
        condition = node.parameters.value(QStringLiteral("condition")).toString();
        emit nodeCompleted(nodeId, QVariant(condition));

    } else {
        emit nodeError(nodeId, QStringLiteral("UnknownNodeType: ") + node.type);
        m_status = Status::Error;
        emit statusChanged(m_status);
        return;
    }

    advanceToNext(nodeId, condition);
}

void WorkflowEngine::advanceToNext(const QString& currentNodeId, const QString& condition) {
    if (m_status != Status::Running)
        return;

    auto it = m_definition.nodes.find(currentNodeId);
    if (it == m_definition.nodes.end())
        return;

    const WorkflowNode& node = it.value();

    // Check conditional branches first
    if (!condition.isEmpty() && node.conditionalNext.contains(condition)) {
        const QStringList& targets = node.conditionalNext.value(condition);
        if (!targets.isEmpty()) {
            executeNode(targets.first());
            return;
        }
    }

    // Default next
    if (!node.nextNodes.isEmpty()) {
        executeNode(node.nextNodes.first());
        return;
    }

    // No successors: workflow complete
    m_status = Status::Completed;
    emit statusChanged(m_status);
    emit workflowFinished(true);
}
