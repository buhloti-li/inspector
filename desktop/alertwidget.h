#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>

class AlertNotificationService;

class AlertWidget : public QWidget {
    Q_OBJECT
public:
    explicit AlertWidget(AlertNotificationService* service, QWidget* parent = nullptr);

private slots:
    void refreshAlerts();
    void onAcknowledgeSelected();
    void onAcknowledgeAll();
    void onClearHistory();
    void onFilterChanged();

private:
    void setupUi();

    AlertNotificationService* m_service;
    QTableWidget* m_table;
    QComboBox* m_severityFilter;
    QPushButton* m_ackBtn;
    QPushButton* m_ackAllBtn;
    QPushButton* m_clearBtn;
    QLabel* m_countLabel;
};
