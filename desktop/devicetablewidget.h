#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QLabel>

class DeviceListModel;

class DeviceTableWidget : public QWidget {
    Q_OBJECT
public:
    explicit DeviceTableWidget(DeviceListModel* model, QWidget* parent = nullptr);

private slots:
    void refreshTable();
    void onFilterChanged();

private:
    void setupUi();
    QString translateType(const QString& type) const;
    QString translateStatus(const QString& status) const;

    DeviceListModel* m_model;
    QTableWidget* m_table;
    QComboBox* m_typeFilter;
    QComboBox* m_statusFilter;
    QLabel* m_countLabel;
};
