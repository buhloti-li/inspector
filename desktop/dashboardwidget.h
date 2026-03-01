#pragma once

#include <QWidget>
#include <QLabel>
#include <QProgressBar>

class WorkcellStatusProvider;

class DashboardWidget : public QWidget {
    Q_OBJECT
public:
    explicit DashboardWidget(WorkcellStatusProvider* provider, QWidget* parent = nullptr);

private slots:
    void onStateChanged();
    void onCycleCountChanged(int total, int success, int fail);
    void onRecipeChanged(const QString& recipe);
    void onAutoModeChanged(bool autoMode);

private:
    void setupUi();
    QWidget* createStatCard(const QString& title, QLabel** valueLabel, const QString& color);

    WorkcellStatusProvider* m_provider;

    QLabel* m_stateLabel;
    QWidget* m_stateCard;
    QLabel* m_cycleCountLabel;
    QLabel* m_successCountLabel;
    QLabel* m_failCountLabel;
    QLabel* m_successRateLabel;
    QProgressBar* m_successRateBar;
    QLabel* m_recipeLabel;
    QLabel* m_autoModeLabel;
};
