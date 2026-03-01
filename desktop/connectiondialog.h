#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>

class MobileClient;
class WorkcellStatusProvider;

class ConnectionDialog : public QDialog {
    Q_OBJECT
public:
    explicit ConnectionDialog(MobileClient* client,
                              WorkcellStatusProvider* statusProvider,
                              QWidget* parent = nullptr);

private slots:
    void onConnectClicked();
    void onDisconnectClicked();
    void onConnectionStateChanged();

private:
    void setupUi();
    void updateState();

    MobileClient* m_client;
    WorkcellStatusProvider* m_statusProvider;

    QLineEdit* m_hostEdit;
    QSpinBox* m_portSpin;
    QSpinBox* m_maxRetrySpin;
    QPushButton* m_connectBtn;
    QPushButton* m_disconnectBtn;
    QLabel* m_statusLabel;
};
