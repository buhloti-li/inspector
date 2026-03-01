#ifndef RECOVERYMANAGER_H
#define RECOVERYMANAGER_H

#include <QObject>
#include <QMap>
#include <QVector>
#include <QString>

class WorkcellController;

struct RecoveryStrategy {
    QString errorCode;

    enum Action {
        Retry,
        SkipObject,
        DegradeSpeed,
        Alert,
        EStop
    };

    QVector<Action> actionSequence;
    int maxRetries = 3;
};

class RecoveryManager : public QObject {
    Q_OBJECT
public:
    explicit RecoveryManager(QObject* parent = nullptr);

    void registerStrategy(const RecoveryStrategy& strategy);
    void removeStrategy(const QString& errorCode);
    bool hasStrategy(const QString& errorCode) const;

    // Attempt recovery for the given error. Returns true if recovery succeeded.
    bool handleError(const QString& errorCode, WorkcellController* controller);

    // Register default strategies for common errors
    void registerDefaults();

signals:
    void recoveryAttempted(const QString& errorCode, const QString& action, bool success);
    void alertTriggered(const QString& errorCode, const QString& message);

private:
    static QString actionName(RecoveryStrategy::Action action);

    QMap<QString, RecoveryStrategy> m_strategies;
    QMap<QString, int> m_retryCounts;  // errorCode -> current retry count
};

#endif // RECOVERYMANAGER_H
