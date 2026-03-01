#include "controlpanelwidget.h"
#include "mobile/mobileclient.h"
#include "mobile/remotecommandservice.h"
#include "mobile/workcellstatusprovider.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QGridLayout>
#include <QDateTime>

ControlPanelWidget::ControlPanelWidget(MobileClient* client,
                                       RemoteCommandService* commandService,
                                       WorkcellStatusProvider* statusProvider,
                                       QWidget* parent)
    : QWidget(parent)
    , m_client(client)
    , m_commandService(commandService)
    , m_statusProvider(statusProvider)
{
    setupUi();

    connect(m_commandService, &RemoteCommandService::commandAcknowledged,
            this, &ControlPanelWidget::onCommandAcknowledged);
    connect(m_commandService, &RemoteCommandService::commandRejected,
            this, &ControlPanelWidget::onCommandRejected);
    connect(m_statusProvider, &WorkcellStatusProvider::stateChanged,
            this, &ControlPanelWidget::onStateChanged);
}

void ControlPanelWidget::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(24, 24, 24, 24);

    // Connection status
    m_statusDisplay = new QLabel(QStringLiteral("\u672A\u8FDE\u63A5\u670D\u52A1\u5668"));  // 未连接服务器
    m_statusDisplay->setAlignment(Qt::AlignCenter);
    m_statusDisplay->setFixedHeight(48);
    m_statusDisplay->setStyleSheet(
        "QLabel { background: #f5f5f5; border-radius: 8px; font-size: 15px; color: #666; }");
    mainLayout->addWidget(m_statusDisplay);

    // Main control buttons
    auto* controlGroup = new QGroupBox(QStringLiteral("\u5468\u671F\u63A7\u5236"));  // 周期控制
    auto* controlGrid = new QGridLayout(controlGroup);
    controlGrid->setSpacing(12);

    m_startBtn = new QPushButton(QStringLiteral("\u542F\u52A8\u5468\u671F"));  // 启动周期
    m_startBtn->setMinimumHeight(64);
    m_startBtn->setStyleSheet(
        "QPushButton { background: #4caf50; color: white; border: none; border-radius: 8px; "
        "  font-size: 18px; font-weight: bold; }"
        "QPushButton:hover { background: #43a047; }"
        "QPushButton:pressed { background: #388e3c; }"
        "QPushButton:disabled { background: #ccc; }");
    connect(m_startBtn, &QPushButton::clicked, this, &ControlPanelWidget::onStartClicked);

    m_stopBtn = new QPushButton(QStringLiteral("\u505C\u6B62"));  // 停止
    m_stopBtn->setMinimumHeight(64);
    m_stopBtn->setStyleSheet(
        "QPushButton { background: #1e88e5; color: white; border: none; border-radius: 8px; "
        "  font-size: 18px; font-weight: bold; }"
        "QPushButton:hover { background: #1976d2; }"
        "QPushButton:pressed { background: #1565c0; }"
        "QPushButton:disabled { background: #ccc; }");
    connect(m_stopBtn, &QPushButton::clicked, this, &ControlPanelWidget::onStopClicked);

    controlGrid->addWidget(m_startBtn, 0, 0);
    controlGrid->addWidget(m_stopBtn, 0, 1);

    mainLayout->addWidget(controlGroup);

    // Emergency stop
    m_eStopBtn = new QPushButton(QStringLiteral("\u7D27 \u6025 \u505C \u6B62"));  // 紧 急 停 止
    m_eStopBtn->setMinimumHeight(80);
    m_eStopBtn->setStyleSheet(
        "QPushButton { background: #d32f2f; color: white; border: 3px solid #b71c1c; "
        "  border-radius: 12px; font-size: 24px; font-weight: bold; }"
        "QPushButton:hover { background: #c62828; }"
        "QPushButton:pressed { background: #b71c1c; }"
        "QPushButton:disabled { background: #ccc; border-color: #aaa; }");
    connect(m_eStopBtn, &QPushButton::clicked, this, &ControlPanelWidget::onEmergencyStopClicked);
    mainLayout->addWidget(m_eStopBtn);

    // Speed control + auto mode in a row
    auto* settingsLayout = new QHBoxLayout;
    settingsLayout->setSpacing(16);

    // Speed group
    auto* speedGroup = new QGroupBox(QStringLiteral("\u8FD0\u884C\u901F\u5EA6"));  // 运行速度
    auto* speedLayout = new QHBoxLayout(speedGroup);

    m_speedSlider = new QSlider(Qt::Horizontal);
    m_speedSlider->setRange(10, 100);
    m_speedSlider->setValue(100);
    m_speedSlider->setSingleStep(5);
    m_speedSlider->setTickInterval(10);
    m_speedSlider->setTickPosition(QSlider::TicksBelow);
    connect(m_speedSlider, &QSlider::valueChanged, this, &ControlPanelWidget::onSpeedChanged);

    m_speedLabel = new QLabel("100%");
    m_speedLabel->setMinimumWidth(50);
    m_speedLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_speedLabel->setStyleSheet("QLabel { font-size: 18px; font-weight: bold; color: #1a73e8; }");

    speedLayout->addWidget(m_speedSlider, 1);
    speedLayout->addWidget(m_speedLabel);
    settingsLayout->addWidget(speedGroup, 2);

    // Auto mode + Recipe group
    auto* modeGroup = new QGroupBox(QStringLiteral("\u6A21\u5F0F\u8BBE\u7F6E"));  // 模式设置
    auto* modeLayout = new QVBoxLayout(modeGroup);

    m_autoModeCheck = new QCheckBox(QStringLiteral("\u81EA\u52A8\u6A21\u5F0F"));  // 自动模式
    m_autoModeCheck->setStyleSheet("QCheckBox { font-size: 14px; }");
    connect(m_autoModeCheck, &QCheckBox::toggled, this, &ControlPanelWidget::onAutoModeToggled);
    modeLayout->addWidget(m_autoModeCheck);

    auto* recipeLayout = new QHBoxLayout;
    m_recipeInput = new QLineEdit;
    m_recipeInput->setPlaceholderText(QStringLiteral("\u914D\u65B9\u540D\u79F0"));  // 配方名称
    m_loadRecipeBtn = new QPushButton(QStringLiteral("\u52A0\u8F7D"));  // 加载
    m_loadRecipeBtn->setStyleSheet(
        "QPushButton { background: #1a73e8; color: white; border: none; border-radius: 4px; "
        "  padding: 6px 16px; font-weight: bold; }"
        "QPushButton:hover { background: #1565c0; }");
    connect(m_loadRecipeBtn, &QPushButton::clicked, this, &ControlPanelWidget::onLoadRecipeClicked);
    recipeLayout->addWidget(m_recipeInput, 1);
    recipeLayout->addWidget(m_loadRecipeBtn);
    modeLayout->addLayout(recipeLayout);

    settingsLayout->addWidget(modeGroup, 1);
    mainLayout->addLayout(settingsLayout);

    // Command log
    auto* logGroup = new QGroupBox(QStringLiteral("\u547D\u4EE4\u65E5\u5FD7"));  // 命令日志
    auto* logLayout = new QVBoxLayout(logGroup);

    m_commandLog = new QTextEdit;
    m_commandLog->setReadOnly(true);
    m_commandLog->setMaximumHeight(150);
    m_commandLog->setStyleSheet(
        "QTextEdit { background: #fafafa; border: 1px solid #e0e0e0; font-family: monospace; font-size: 12px; }");
    logLayout->addWidget(m_commandLog);

    mainLayout->addWidget(logGroup);
    mainLayout->addStretch();
}

void ControlPanelWidget::onStartClicked()
{
    int id = m_commandService->startCycle();
    appendLog(QStringLiteral("[%1] \u53D1\u9001: startCycle (ID=%2)")
                  .arg(QDateTime::currentDateTime().toString("HH:mm:ss"))
                  .arg(id));
}

void ControlPanelWidget::onStopClicked()
{
    int id = m_commandService->stopCycle();
    appendLog(QStringLiteral("[%1] \u53D1\u9001: stopCycle (ID=%2)")
                  .arg(QDateTime::currentDateTime().toString("HH:mm:ss"))
                  .arg(id));
}

void ControlPanelWidget::onEmergencyStopClicked()
{
    int id = m_commandService->emergencyStop();
    appendLog(QStringLiteral("[%1] \u53D1\u9001: emergencyStop (ID=%2)")
                  .arg(QDateTime::currentDateTime().toString("HH:mm:ss"))
                  .arg(id));
}

void ControlPanelWidget::onSpeedChanged(int value)
{
    m_speedLabel->setText(QStringLiteral("%1%").arg(value));
    if (m_client->connectionState() == MobileClient::ConnectionState::Connected) {
        m_commandService->setSpeed(static_cast<double>(value));
    }
}

void ControlPanelWidget::onAutoModeToggled(bool checked)
{
    if (m_client->connectionState() == MobileClient::ConnectionState::Connected) {
        int id = m_commandService->setAutoMode(checked);
        appendLog(QStringLiteral("[%1] \u53D1\u9001: setAutoMode(%2) (ID=%3)")
                      .arg(QDateTime::currentDateTime().toString("HH:mm:ss"))
                      .arg(checked ? "true" : "false")
                      .arg(id));
    }
}

void ControlPanelWidget::onLoadRecipeClicked()
{
    QString recipe = m_recipeInput->text().trimmed();
    if (recipe.isEmpty()) return;

    int id = m_commandService->loadRecipe(recipe);
    appendLog(QStringLiteral("[%1] \u53D1\u9001: loadRecipe(\"%2\") (ID=%3)")
                  .arg(QDateTime::currentDateTime().toString("HH:mm:ss"))
                  .arg(recipe)
                  .arg(id));
}

void ControlPanelWidget::onCommandAcknowledged(int requestId, const QString& message)
{
    appendLog(QStringLiteral("[%1] \u2713 ID=%2 \u5DF2\u786E\u8BA4: %3")
                  .arg(QDateTime::currentDateTime().toString("HH:mm:ss"))
                  .arg(requestId)
                  .arg(message));
}

void ControlPanelWidget::onCommandRejected(int requestId, const QString& reason)
{
    appendLog(QStringLiteral("[%1] \u2717 ID=%2 \u88AB\u62D2\u7EDD: %3")
                  .arg(QDateTime::currentDateTime().toString("HH:mm:ss"))
                  .arg(requestId)
                  .arg(reason));
}

void ControlPanelWidget::onStateChanged()
{
    auto state = m_statusProvider->workcellState();
    bool connected = (m_client->connectionState() == MobileClient::ConnectionState::Connected);
    QString text;
    QString bgColor;
    QString textColor;

    if (!connected) {
        text = QStringLiteral("\u672A\u8FDE\u63A5\u670D\u52A1\u5668");
        bgColor = "#f5f5f5"; textColor = "#666";
    } else {
        switch (state) {
        case WorkcellStatusProvider::WorkcellState::Idle:
            text = QStringLiteral("\u5DF2\u8FDE\u63A5 - \u7A7A\u95F2");
            bgColor = "#f5f5f5"; textColor = "#333"; break;
        case WorkcellStatusProvider::WorkcellState::Running:
            text = QStringLiteral("\u5DF2\u8FDE\u63A5 - \u8FD0\u884C\u4E2D");
            bgColor = "#e8f5e9"; textColor = "#2e7d32"; break;
        case WorkcellStatusProvider::WorkcellState::Stopped:
            text = QStringLiteral("\u5DF2\u8FDE\u63A5 - \u5DF2\u505C\u6B62");
            bgColor = "#e3f2fd"; textColor = "#1565c0"; break;
        case WorkcellStatusProvider::WorkcellState::Fault:
            text = QStringLiteral("\u5DF2\u8FDE\u63A5 - \u6545\u969C");
            bgColor = "#ffebee"; textColor = "#c62828"; break;
        case WorkcellStatusProvider::WorkcellState::Degraded:
            text = QStringLiteral("\u5DF2\u8FDE\u63A5 - \u964D\u7EA7");
            bgColor = "#fff3e0"; textColor = "#e65100"; break;
        default:
            text = QStringLiteral("\u5DF2\u8FDE\u63A5 - \u672A\u77E5");
            bgColor = "#f5f5f5"; textColor = "#999"; break;
        }
    }

    m_statusDisplay->setText(text);
    m_statusDisplay->setStyleSheet(
        QStringLiteral("QLabel { background: %1; border-radius: 8px; font-size: 15px; "
                       "color: %2; font-weight: bold; }").arg(bgColor, textColor));
}

void ControlPanelWidget::appendLog(const QString& message)
{
    m_commandLog->append(message);
}
