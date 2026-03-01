#include "devicetablewidget.h"
#include "mobile/devicelistmodel.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>

DeviceTableWidget::DeviceTableWidget(DeviceListModel* model, QWidget* parent)
    : QWidget(parent)
    , m_model(model)
{
    setupUi();

    connect(m_model, &DeviceListModel::deviceListUpdated, this, &DeviceTableWidget::refreshTable);
    connect(m_model, &DeviceListModel::deviceStatusChanged, this, &DeviceTableWidget::refreshTable);
    connect(m_model, &DeviceListModel::deviceAdded, this, &DeviceTableWidget::refreshTable);
    connect(m_model, &DeviceListModel::deviceRemoved, this, &DeviceTableWidget::refreshTable);
}

void DeviceTableWidget::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(16, 16, 16, 16);

    // Toolbar
    auto* toolbar = new QHBoxLayout;

    auto* titleLabel = new QLabel(QStringLiteral("\u8BBE\u5907\u5217\u8868"));  // 设备列表
    titleLabel->setStyleSheet("QLabel { font-size: 18px; font-weight: bold; }");
    toolbar->addWidget(titleLabel);

    toolbar->addSpacing(24);

    // Type filter
    auto* typeLabel = new QLabel(QStringLiteral("\u7C7B\u578B:"));  // 类型
    toolbar->addWidget(typeLabel);

    m_typeFilter = new QComboBox;
    m_typeFilter->addItem(QStringLiteral("\u5168\u90E8"), "");          // 全部
    m_typeFilter->addItem(QStringLiteral("\u76F8\u673A"), "camera");    // 相机
    m_typeFilter->addItem(QStringLiteral("\u673A\u5668\u4EBA"), "robot"); // 机器人
    m_typeFilter->addItem("PLC", "plc");
    m_typeFilter->addItem(QStringLiteral("\u4F20\u611F\u5668"), "sensor"); // 传感器
    m_typeFilter->addItem(QStringLiteral("\u5939\u722A"), "gripper");     // 夹爪
    connect(m_typeFilter, &QComboBox::currentIndexChanged, this, &DeviceTableWidget::onFilterChanged);
    toolbar->addWidget(m_typeFilter);

    toolbar->addSpacing(16);

    // Status filter
    auto* statusLabel = new QLabel(QStringLiteral("\u72B6\u6001:"));  // 状态
    toolbar->addWidget(statusLabel);

    m_statusFilter = new QComboBox;
    m_statusFilter->addItem(QStringLiteral("\u5168\u90E8"), "");                // 全部
    m_statusFilter->addItem(QStringLiteral("\u5C31\u7EEA"), "Ready");           // 就绪
    m_statusFilter->addItem(QStringLiteral("\u5FD9\u788C"), "Busy");            // 忙碌
    m_statusFilter->addItem(QStringLiteral("\u9519\u8BEF"), "Error");           // 错误
    m_statusFilter->addItem(QStringLiteral("\u65AD\u5F00"), "Disconnected");    // 断开
    connect(m_statusFilter, &QComboBox::currentIndexChanged, this, &DeviceTableWidget::onFilterChanged);
    toolbar->addWidget(m_statusFilter);

    toolbar->addStretch();

    m_countLabel = new QLabel;
    m_countLabel->setStyleSheet("QLabel { color: #666; }");
    toolbar->addWidget(m_countLabel);

    mainLayout->addLayout(toolbar);

    // Table
    m_table = new QTableWidget;
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("\u8BBE\u5907 ID"),    // 设备 ID
        QStringLiteral("\u7C7B\u578B"),        // 类型
        QStringLiteral("\u72B6\u6001"),        // 状态
        QStringLiteral("\u5C5E\u6027")         // 属性
    });
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->setSelectionBehavior(QTableWidget::SelectRows);
    m_table->setSelectionMode(QTableWidget::SingleSelection);
    m_table->setEditTriggers(QTableWidget::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->setStyleSheet(
        "QTableWidget { border: 1px solid #e0e0e0; gridline-color: #f0f0f0; }"
        "QTableWidget::item { padding: 8px; }"
        "QHeaderView::section { background: #f5f5f5; padding: 8px; border: none; "
        "  border-bottom: 2px solid #e0e0e0; font-weight: bold; }");

    mainLayout->addWidget(m_table, 1);
}

void DeviceTableWidget::refreshTable()
{
    onFilterChanged();
}

void DeviceTableWidget::onFilterChanged()
{
    QString typeFilter = m_typeFilter->currentData().toString();
    QString statusFilter = m_statusFilter->currentData().toString();

    QVector<DeviceListModel::DeviceInfo> devices;
    if (!typeFilter.isEmpty()) {
        devices = m_model->devicesByType(typeFilter);
    } else if (!statusFilter.isEmpty()) {
        devices = m_model->devicesByStatus(statusFilter);
    } else {
        devices = m_model->allDevices();
    }

    // Apply second filter if both set
    if (!typeFilter.isEmpty() && !statusFilter.isEmpty()) {
        QVector<DeviceListModel::DeviceInfo> filtered;
        for (const auto& d : devices) {
            if (d.status == statusFilter)
                filtered.append(d);
        }
        devices = filtered;
    }

    m_table->setRowCount(devices.size());
    for (int i = 0; i < devices.size(); ++i) {
        const auto& dev = devices[i];

        auto* idItem = new QTableWidgetItem(dev.deviceId);
        idItem->setFont(QFont(idItem->font().family(), -1, QFont::Bold));
        m_table->setItem(i, 0, idItem);

        m_table->setItem(i, 1, new QTableWidgetItem(translateType(dev.deviceType)));

        auto* statusItem = new QTableWidgetItem(translateStatus(dev.status));
        if (dev.status == "Ready")
            statusItem->setForeground(QColor("#2e7d32"));
        else if (dev.status == "Busy")
            statusItem->setForeground(QColor("#e65100"));
        else if (dev.status == "Error")
            statusItem->setForeground(QColor("#c62828"));
        else
            statusItem->setForeground(QColor("#666"));
        statusItem->setFont(QFont(statusItem->font().family(), -1, QFont::Bold));
        m_table->setItem(i, 2, statusItem);

        // Properties as key=value pairs
        QStringList props;
        for (auto it = dev.properties.begin(); it != dev.properties.end(); ++it) {
            props << QStringLiteral("%1=%2").arg(it.key(), it.value().toString());
        }
        m_table->setItem(i, 3, new QTableWidgetItem(props.join(", ")));
    }

    m_countLabel->setText(QStringLiteral("\u5171 %1 \u53F0\u8BBE\u5907").arg(devices.size()));  // 共 N 台设备
}

QString DeviceTableWidget::translateType(const QString& type) const
{
    if (type == "camera") return QStringLiteral("\u76F8\u673A");       // 相机
    if (type == "robot") return QStringLiteral("\u673A\u5668\u4EBA");  // 机器人
    if (type == "plc") return "PLC";
    if (type == "sensor") return QStringLiteral("\u4F20\u611F\u5668"); // 传感器
    if (type == "gripper") return QStringLiteral("\u5939\u722A");      // 夹爪
    return type;
}

QString DeviceTableWidget::translateStatus(const QString& status) const
{
    if (status == "Ready") return QStringLiteral("\u5C31\u7EEA");           // 就绪
    if (status == "Busy") return QStringLiteral("\u5FD9\u788C");            // 忙碌
    if (status == "Error") return QStringLiteral("\u9519\u8BEF");           // 错误
    if (status == "Disconnected") return QStringLiteral("\u65AD\u5F00");    // 断开
    return status;
}
