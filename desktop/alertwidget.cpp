#include "alertwidget.h"
#include "mobile/alertnotificationservice.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>

AlertWidget::AlertWidget(AlertNotificationService* service, QWidget* parent)
    : QWidget(parent)
    , m_service(service)
{
    setupUi();

    connect(m_service, &AlertNotificationService::alertReceived, this, &AlertWidget::refreshAlerts);
    connect(m_service, &AlertNotificationService::alertAcknowledged, this, &AlertWidget::refreshAlerts);
    connect(m_service, &AlertNotificationService::allAlertsAcknowledged, this, &AlertWidget::refreshAlerts);
    connect(m_service, &AlertNotificationService::alertsCleared, this, &AlertWidget::refreshAlerts);
}

void AlertWidget::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(16, 16, 16, 16);

    // Toolbar
    auto* toolbar = new QHBoxLayout;

    auto* titleLabel = new QLabel(QStringLiteral("\u544A\u8B66\u901A\u77E5"));  // 告警通知
    titleLabel->setStyleSheet("QLabel { font-size: 18px; font-weight: bold; }");
    toolbar->addWidget(titleLabel);

    toolbar->addSpacing(16);

    // Severity filter
    auto* filterLabel = new QLabel(QStringLiteral("\u7B5B\u9009:"));  // 筛选
    toolbar->addWidget(filterLabel);

    m_severityFilter = new QComboBox;
    m_severityFilter->addItem(QStringLiteral("\u5168\u90E8"), -1);        // 全部
    m_severityFilter->addItem(QStringLiteral("\u4FE1\u606F"), 0);         // 信息
    m_severityFilter->addItem(QStringLiteral("\u8B66\u544A"), 1);         // 警告
    m_severityFilter->addItem(QStringLiteral("\u9519\u8BEF"), 2);         // 错误
    m_severityFilter->addItem(QStringLiteral("\u4E25\u91CD"), 3);         // 严重
    connect(m_severityFilter, &QComboBox::currentIndexChanged, this, &AlertWidget::onFilterChanged);
    toolbar->addWidget(m_severityFilter);

    toolbar->addStretch();

    m_countLabel = new QLabel;
    m_countLabel->setStyleSheet("QLabel { color: #666; }");
    toolbar->addWidget(m_countLabel);

    toolbar->addSpacing(16);

    m_ackBtn = new QPushButton(QStringLiteral("\u786E\u8BA4\u9009\u4E2D"));  // 确认选中
    m_ackBtn->setStyleSheet(
        "QPushButton { background: #1a73e8; color: white; border: none; border-radius: 4px; "
        "  padding: 6px 12px; font-weight: bold; }"
        "QPushButton:hover { background: #1565c0; }");
    connect(m_ackBtn, &QPushButton::clicked, this, &AlertWidget::onAcknowledgeSelected);
    toolbar->addWidget(m_ackBtn);

    m_ackAllBtn = new QPushButton(QStringLiteral("\u5168\u90E8\u786E\u8BA4"));  // 全部确认
    m_ackAllBtn->setStyleSheet(
        "QPushButton { background: #ff9800; color: white; border: none; border-radius: 4px; "
        "  padding: 6px 12px; font-weight: bold; }"
        "QPushButton:hover { background: #f57c00; }");
    connect(m_ackAllBtn, &QPushButton::clicked, this, &AlertWidget::onAcknowledgeAll);
    toolbar->addWidget(m_ackAllBtn);

    m_clearBtn = new QPushButton(QStringLiteral("\u6E05\u9664"));  // 清除
    m_clearBtn->setStyleSheet(
        "QPushButton { background: #f44336; color: white; border: none; border-radius: 4px; "
        "  padding: 6px 12px; font-weight: bold; }"
        "QPushButton:hover { background: #e53935; }");
    connect(m_clearBtn, &QPushButton::clicked, this, &AlertWidget::onClearHistory);
    toolbar->addWidget(m_clearBtn);

    mainLayout->addLayout(toolbar);

    // Alert table
    m_table = new QTableWidget;
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels({
        "ID",
        QStringLiteral("\u7EA7\u522B"),        // 级别
        QStringLiteral("\u6765\u6E90"),        // 来源
        QStringLiteral("\u6D88\u606F"),        // 消息
        QStringLiteral("\u65F6\u95F4"),        // 时间
        QStringLiteral("\u72B6\u6001")         // 状态
    });

    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);

    m_table->setSelectionBehavior(QTableWidget::SelectRows);
    m_table->setSelectionMode(QTableWidget::ExtendedSelection);
    m_table->setEditTriggers(QTableWidget::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->setStyleSheet(
        "QTableWidget { border: 1px solid #e0e0e0; gridline-color: #f0f0f0; }"
        "QTableWidget::item { padding: 6px; }"
        "QHeaderView::section { background: #f5f5f5; padding: 8px; border: none; "
        "  border-bottom: 2px solid #e0e0e0; font-weight: bold; }");

    mainLayout->addWidget(m_table, 1);
}

void AlertWidget::refreshAlerts()
{
    onFilterChanged();
}

void AlertWidget::onFilterChanged()
{
    int filterVal = m_severityFilter->currentData().toInt();

    QVector<AlertNotificationService::Alert> alerts;
    if (filterVal < 0) {
        alerts = m_service->allAlerts();
    } else {
        alerts = m_service->alertsBySeverity(static_cast<AlertNotificationService::Severity>(filterVal));
    }

    m_table->setRowCount(alerts.size());

    // Show newest first
    for (int i = 0; i < alerts.size(); ++i) {
        const auto& alert = alerts[alerts.size() - 1 - i];

        m_table->setItem(i, 0, new QTableWidgetItem(QString::number(alert.id)));

        // Severity
        QString sevText;
        QColor sevColor;
        switch (alert.severity) {
        case AlertNotificationService::Severity::Info:
            sevText = QStringLiteral("\u4FE1\u606F"); sevColor = QColor("#1565c0"); break;
        case AlertNotificationService::Severity::Warning:
            sevText = QStringLiteral("\u8B66\u544A"); sevColor = QColor("#f9a825"); break;
        case AlertNotificationService::Severity::Error:
            sevText = QStringLiteral("\u9519\u8BEF"); sevColor = QColor("#e65100"); break;
        case AlertNotificationService::Severity::Critical:
            sevText = QStringLiteral("\u4E25\u91CD"); sevColor = QColor("#c62828"); break;
        }
        auto* sevItem = new QTableWidgetItem(sevText);
        sevItem->setForeground(sevColor);
        sevItem->setFont(QFont(sevItem->font().family(), -1, QFont::Bold));
        m_table->setItem(i, 1, sevItem);

        m_table->setItem(i, 2, new QTableWidgetItem(alert.source));
        m_table->setItem(i, 3, new QTableWidgetItem(alert.message));
        m_table->setItem(i, 4, new QTableWidgetItem(
            alert.timestamp.toString("yyyy-MM-dd HH:mm:ss")));

        auto* ackItem = new QTableWidgetItem(
            alert.acknowledged ? QStringLiteral("\u5DF2\u786E\u8BA4") : QStringLiteral("\u672A\u786E\u8BA4"));
        ackItem->setForeground(alert.acknowledged ? QColor("#999") : QColor("#d32f2f"));
        m_table->setItem(i, 5, ackItem);

        // Gray out acknowledged rows
        if (alert.acknowledged) {
            for (int col = 0; col < m_table->columnCount(); ++col) {
                if (auto* item = m_table->item(i, col)) {
                    QColor fg = item->foreground().color();
                    fg.setAlpha(128);
                    item->setForeground(fg);
                }
            }
        }

        // Store alert ID in first column's data
        m_table->item(i, 0)->setData(Qt::UserRole, alert.id);
    }

    int unack = m_service->unacknowledgedCount();
    m_countLabel->setText(QStringLiteral("\u5171 %1 \u6761, \u672A\u786E\u8BA4 %2 \u6761")
                              .arg(alerts.size()).arg(unack));
    m_ackAllBtn->setEnabled(unack > 0);
}

void AlertWidget::onAcknowledgeSelected()
{
    auto selected = m_table->selectedItems();
    QSet<int> alertIds;
    for (auto* item : selected) {
        int row = item->row();
        auto* idItem = m_table->item(row, 0);
        if (idItem)
            alertIds.insert(idItem->data(Qt::UserRole).toInt());
    }
    for (int id : alertIds) {
        m_service->acknowledgeAlert(id);
    }
}

void AlertWidget::onAcknowledgeAll()
{
    m_service->acknowledgeAll();
}

void AlertWidget::onClearHistory()
{
    m_service->clearHistory();
}
