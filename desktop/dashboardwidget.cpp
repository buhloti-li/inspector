#include "dashboardwidget.h"
#include "mobile/workcellstatusprovider.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFrame>
#include <QFont>

DashboardWidget::DashboardWidget(WorkcellStatusProvider* provider, QWidget* parent)
    : QWidget(parent)
    , m_provider(provider)
{
    setupUi();

    connect(m_provider, &WorkcellStatusProvider::stateChanged,
            this, &DashboardWidget::onStateChanged);
    connect(m_provider, &WorkcellStatusProvider::cycleCountChanged,
            this, &DashboardWidget::onCycleCountChanged);
    connect(m_provider, &WorkcellStatusProvider::recipeChanged,
            this, &DashboardWidget::onRecipeChanged);
    connect(m_provider, &WorkcellStatusProvider::autoModeChanged,
            this, &DashboardWidget::onAutoModeChanged);
}

QWidget* DashboardWidget::createStatCard(const QString& title, QLabel** valueLabel, const QString& color)
{
    auto* card = new QFrame;
    card->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    card->setStyleSheet(QStringLiteral(
        "QFrame { background: white; border: 1px solid #e0e0e0; border-radius: 8px; padding: 12px; }"));

    auto* layout = new QVBoxLayout(card);
    layout->setAlignment(Qt::AlignCenter);

    *valueLabel = new QLabel("0");
    (*valueLabel)->setAlignment(Qt::AlignCenter);
    (*valueLabel)->setStyleSheet(QStringLiteral("QLabel { font-size: 36px; font-weight: bold; color: %1; border: none; }").arg(color));

    auto* titleLabel = new QLabel(title);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("QLabel { font-size: 13px; color: #666; border: none; }");

    layout->addWidget(*valueLabel);
    layout->addWidget(titleLabel);

    return card;
}

void DashboardWidget::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(24, 24, 24, 24);

    // State card
    m_stateCard = new QFrame;
    m_stateCard->setStyleSheet(
        "QFrame { background: #f5f5f5; border-radius: 12px; padding: 20px; }");
    auto* stateLayout = new QVBoxLayout(m_stateCard);
    stateLayout->setAlignment(Qt::AlignCenter);

    auto* stateTitle = new QLabel(QStringLiteral("\u5DE5\u4F5C\u7AD9\u72B6\u6001"));  // 工作站状态
    stateTitle->setAlignment(Qt::AlignCenter);
    stateTitle->setStyleSheet("QLabel { font-size: 14px; color: #666; }");

    m_stateLabel = new QLabel(QStringLiteral("\u672A\u77E5"));  // 未知
    m_stateLabel->setAlignment(Qt::AlignCenter);
    m_stateLabel->setStyleSheet("QLabel { font-size: 40px; font-weight: bold; color: #333; }");

    stateLayout->addWidget(stateTitle);
    stateLayout->addWidget(m_stateLabel);
    mainLayout->addWidget(m_stateCard);

    // Stats row
    auto* statsLayout = new QHBoxLayout;
    statsLayout->setSpacing(12);

    statsLayout->addWidget(createStatCard(QStringLiteral("\u603B\u5468\u671F"), &m_cycleCountLabel, "#1a73e8"));  // 总周期
    statsLayout->addWidget(createStatCard(QStringLiteral("\u6210\u529F"), &m_successCountLabel, "#4caf50"));       // 成功
    statsLayout->addWidget(createStatCard(QStringLiteral("\u5931\u8D25"), &m_failCountLabel, "#f44336"));           // 失败

    mainLayout->addLayout(statsLayout);

    // Success rate bar
    auto* rateGroup = new QFrame;
    rateGroup->setFrameStyle(QFrame::StyledPanel);
    rateGroup->setStyleSheet(
        "QFrame { background: white; border: 1px solid #e0e0e0; border-radius: 8px; padding: 12px; }");
    auto* rateLayout = new QHBoxLayout(rateGroup);

    auto* rateTitle = new QLabel(QStringLiteral("\u6210\u529F\u7387"));  // 成功率
    rateTitle->setStyleSheet("QLabel { font-size: 14px; color: #666; border: none; }");

    m_successRateBar = new QProgressBar;
    m_successRateBar->setRange(0, 1000);
    m_successRateBar->setValue(0);
    m_successRateBar->setTextVisible(false);
    m_successRateBar->setFixedHeight(20);
    m_successRateBar->setStyleSheet(
        "QProgressBar { border: 1px solid #e0e0e0; border-radius: 10px; background: #f0f0f0; }"
        "QProgressBar::chunk { background: #1a73e8; border-radius: 10px; }");

    m_successRateLabel = new QLabel("---");
    m_successRateLabel->setStyleSheet("QLabel { font-size: 20px; font-weight: bold; color: #1a73e8; border: none; }");
    m_successRateLabel->setMinimumWidth(80);
    m_successRateLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    rateLayout->addWidget(rateTitle);
    rateLayout->addWidget(m_successRateBar, 1);
    rateLayout->addWidget(m_successRateLabel);

    mainLayout->addWidget(rateGroup);

    // Recipe & Auto mode row
    auto* infoGroup = new QFrame;
    infoGroup->setFrameStyle(QFrame::StyledPanel);
    infoGroup->setStyleSheet(
        "QFrame { background: white; border: 1px solid #e0e0e0; border-radius: 8px; padding: 12px; }");
    auto* infoLayout = new QHBoxLayout(infoGroup);

    auto* recipeTitle = new QLabel(QStringLiteral("\u5F53\u524D\u914D\u65B9:"));  // 当前配方
    recipeTitle->setStyleSheet("QLabel { font-size: 14px; color: #666; border: none; }");
    m_recipeLabel = new QLabel("---");
    m_recipeLabel->setStyleSheet("QLabel { font-size: 16px; font-weight: bold; border: none; }");

    auto* modeTitle = new QLabel(QStringLiteral("\u8FD0\u884C\u6A21\u5F0F:"));  // 运行模式
    modeTitle->setStyleSheet("QLabel { font-size: 14px; color: #666; border: none; }");
    m_autoModeLabel = new QLabel(QStringLiteral("\u624B\u52A8"));  // 手动
    m_autoModeLabel->setStyleSheet("QLabel { font-size: 16px; font-weight: bold; color: #ff9800; border: none; }");

    infoLayout->addWidget(recipeTitle);
    infoLayout->addWidget(m_recipeLabel);
    infoLayout->addStretch();
    infoLayout->addWidget(modeTitle);
    infoLayout->addWidget(m_autoModeLabel);

    mainLayout->addWidget(infoGroup);
    mainLayout->addStretch();
}

void DashboardWidget::onStateChanged()
{
    auto state = m_provider->workcellState();
    QString text;
    QString bgColor;
    QString textColor;

    switch (state) {
    case WorkcellStatusProvider::WorkcellState::Idle:
        text = QStringLiteral("\u7A7A\u95F2");
        bgColor = "#f5f5f5"; textColor = "#333";
        break;
    case WorkcellStatusProvider::WorkcellState::Running:
        text = QStringLiteral("\u8FD0\u884C\u4E2D");
        bgColor = "#e8f5e9"; textColor = "#2e7d32";
        break;
    case WorkcellStatusProvider::WorkcellState::Stopped:
        text = QStringLiteral("\u5DF2\u505C\u6B62");
        bgColor = "#e3f2fd"; textColor = "#1565c0";
        break;
    case WorkcellStatusProvider::WorkcellState::Fault:
        text = QStringLiteral("\u6545\u969C");
        bgColor = "#ffebee"; textColor = "#c62828";
        break;
    case WorkcellStatusProvider::WorkcellState::Degraded:
        text = QStringLiteral("\u964D\u7EA7");
        bgColor = "#fff3e0"; textColor = "#e65100";
        break;
    default:
        text = QStringLiteral("\u672A\u77E5");
        bgColor = "#f5f5f5"; textColor = "#999";
        break;
    }

    m_stateLabel->setText(text);
    m_stateLabel->setStyleSheet(
        QStringLiteral("QLabel { font-size: 40px; font-weight: bold; color: %1; }").arg(textColor));
    m_stateCard->setStyleSheet(
        QStringLiteral("QFrame { background: %1; border-radius: 12px; padding: 20px; }").arg(bgColor));
}

void DashboardWidget::onCycleCountChanged(int total, int success, int fail)
{
    m_cycleCountLabel->setText(QString::number(total));
    m_successCountLabel->setText(QString::number(success));
    m_failCountLabel->setText(QString::number(fail));

    if (total > 0) {
        double rate = static_cast<double>(success) * 100.0 / total;
        m_successRateLabel->setText(QStringLiteral("%1%").arg(rate, 0, 'f', 1));
        m_successRateBar->setValue(static_cast<int>(rate * 10));
    } else {
        m_successRateLabel->setText("---");
        m_successRateBar->setValue(0);
    }
}

void DashboardWidget::onRecipeChanged(const QString& recipe)
{
    m_recipeLabel->setText(recipe.isEmpty() ? "---" : recipe);
}

void DashboardWidget::onAutoModeChanged(bool autoMode)
{
    if (autoMode) {
        m_autoModeLabel->setText(QStringLiteral("\u81EA\u52A8"));  // 自动
        m_autoModeLabel->setStyleSheet("QLabel { font-size: 16px; font-weight: bold; color: #4caf50; border: none; }");
    } else {
        m_autoModeLabel->setText(QStringLiteral("\u624B\u52A8"));  // 手动
        m_autoModeLabel->setStyleSheet("QLabel { font-size: 16px; font-weight: bold; color: #ff9800; border: none; }");
    }
}
