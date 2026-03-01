#pragma once

#include <QWidget>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QCheckBox>
#include <QLineEdit>
#include <QTextEdit>

class MobileClient;
class RemoteCommandService;
class WorkcellStatusProvider;

class ControlPanelWidget : public QWidget {
    Q_OBJECT
public:
    explicit ControlPanelWidget(MobileClient* client,
                                RemoteCommandService* commandService,
                                WorkcellStatusProvider* statusProvider,
                                QWidget* parent = nullptr);

private slots:
    void onStartClicked();
    void onStopClicked();
    void onEmergencyStopClicked();
    void onSpeedChanged(int value);
    void onAutoModeToggled(bool checked);
    void onLoadRecipeClicked();
    void onCommandAcknowledged(int requestId, const QString& message);
    void onCommandRejected(int requestId, const QString& reason);
    void onStateChanged();

private:
    void setupUi();
    void appendLog(const QString& message);

    MobileClient* m_client;
    RemoteCommandService* m_commandService;
    WorkcellStatusProvider* m_statusProvider;

    QPushButton* m_startBtn;
    QPushButton* m_stopBtn;
    QPushButton* m_eStopBtn;
    QSlider* m_speedSlider;
    QLabel* m_speedLabel;
    QCheckBox* m_autoModeCheck;
    QLineEdit* m_recipeInput;
    QPushButton* m_loadRecipeBtn;
    QLabel* m_statusDisplay;
    QTextEdit* m_commandLog;
};
