#include "connectiondialog.h"
#include "mobile/mobileclient.h"
#include "mobile/workcellstatusprovider.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QDialogButtonBox>

ConnectionDialog::ConnectionDialog(MobileClient* client,
                                   WorkcellStatusProvider* statusProvider,
                                   QWidget* parent)
    : QDialog(parent)
    , m_client(client)
    , m_statusProvider(statusProvider)
{
    setupUi();
    updateState();

    connect(m_client, &MobileClient::connectionStateChanged,
            this, &ConnectionDialog::onConnectionStateChanged);
}

void ConnectionDialog::setupUi()
{
    setWindowTitle(QStringLiteral("\u8FDE\u63A5\u670D\u52A1\u5668"));  // 连接服务器
    setMinimumWidth(400);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);

    // Status
    m_statusLabel = new QLabel;
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setFixedHeight(40);
    m_statusLabel->setStyleSheet(
        "QLabel { background: #f5f5f5; border-radius: 6px; font-size: 14px; }");
    mainLayout->addWidget(m_statusLabel);

    // Server settings
    auto* serverGroup = new QGroupBox(QStringLiteral("\u670D\u52A1\u5668\u8BBE\u7F6E"));  // 服务器设置
    auto* formLayout = new QFormLayout(serverGroup);

    m_hostEdit = new QLineEdit("192.168.1.100");
    m_hostEdit->setPlaceholderText(QStringLiteral("IP \u5730\u5740"));  // IP 地址
    if (!m_client->serverHost().isEmpty())
        m_hostEdit->setText(m_client->serverHost());
    formLayout->addRow(QStringLiteral("\u670D\u52A1\u5668\u5730\u5740:"), m_hostEdit);

    m_portSpin = new QSpinBox;
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(m_client->serverPort() > 0 ? m_client->serverPort() : 9600);
    formLayout->addRow(QStringLiteral("\u7AEF\u53E3:"), m_portSpin);

    mainLayout->addWidget(serverGroup);

    // Reconnect settings
    auto* reconnGroup = new QGroupBox(QStringLiteral("\u91CD\u8FDE\u8BBE\u7F6E"));  // 重连设置
    auto* reconnLayout = new QFormLayout(reconnGroup);

    m_maxRetrySpin = new QSpinBox;
    m_maxRetrySpin->setRange(1, 20);
    m_maxRetrySpin->setValue(m_client->maxReconnectAttempts());
    connect(m_maxRetrySpin, &QSpinBox::valueChanged, m_client, &MobileClient::setMaxReconnectAttempts);
    reconnLayout->addRow(QStringLiteral("\u6700\u5927\u91CD\u8BD5\u6B21\u6570:"), m_maxRetrySpin);

    mainLayout->addWidget(reconnGroup);

    // Buttons
    auto* btnLayout = new QHBoxLayout;

    m_connectBtn = new QPushButton(QStringLiteral("\u8FDE\u63A5"));  // 连接
    m_connectBtn->setStyleSheet(
        "QPushButton { background: #1a73e8; color: white; border: none; border-radius: 6px; "
        "  padding: 10px 24px; font-size: 14px; font-weight: bold; }"
        "QPushButton:hover { background: #1565c0; }"
        "QPushButton:disabled { background: #ccc; }");
    connect(m_connectBtn, &QPushButton::clicked, this, &ConnectionDialog::onConnectClicked);

    m_disconnectBtn = new QPushButton(QStringLiteral("\u65AD\u5F00\u8FDE\u63A5"));  // 断开连接
    m_disconnectBtn->setStyleSheet(
        "QPushButton { background: #f44336; color: white; border: none; border-radius: 6px; "
        "  padding: 10px 24px; font-size: 14px; font-weight: bold; }"
        "QPushButton:hover { background: #e53935; }"
        "QPushButton:disabled { background: #ccc; }");
    connect(m_disconnectBtn, &QPushButton::clicked, this, &ConnectionDialog::onDisconnectClicked);

    auto* closeBtn = new QPushButton(QStringLiteral("\u5173\u95ED"));  // 关闭
    closeBtn->setStyleSheet(
        "QPushButton { border: 1px solid #ccc; border-radius: 6px; "
        "  padding: 10px 24px; font-size: 14px; }"
        "QPushButton:hover { background: #f0f0f0; }");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);

    btnLayout->addWidget(m_connectBtn);
    btnLayout->addWidget(m_disconnectBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(closeBtn);

    mainLayout->addLayout(btnLayout);
}

void ConnectionDialog::onConnectClicked()
{
    QString host = m_hostEdit->text().trimmed();
    int port = m_portSpin->value();

    if (host.isEmpty()) return;

    if (m_client->connectToServer(host, port)) {
        m_statusProvider->startPolling(1000);
    }
}

void ConnectionDialog::onDisconnectClicked()
{
    m_statusProvider->stopPolling();
    m_client->disconnectFromServer();
}

void ConnectionDialog::onConnectionStateChanged()
{
    updateState();
}

void ConnectionDialog::updateState()
{
    auto state = m_client->connectionState();
    bool connected = (state == MobileClient::ConnectionState::Connected);
    bool connecting = (state == MobileClient::ConnectionState::Connecting
                       || state == MobileClient::ConnectionState::Reconnecting);

    m_connectBtn->setEnabled(!connected && !connecting);
    m_disconnectBtn->setEnabled(connected || connecting);
    m_hostEdit->setEnabled(!connected && !connecting);
    m_portSpin->setEnabled(!connected && !connecting);

    switch (state) {
    case MobileClient::ConnectionState::Connected:
        m_statusLabel->setText(
            QStringLiteral("\u2022 \u5DF2\u8FDE\u63A5\u5230 %1:%2")
                .arg(m_client->serverHost()).arg(m_client->serverPort()));
        m_statusLabel->setStyleSheet(
            "QLabel { background: #e8f5e9; border-radius: 6px; font-size: 14px; "
            "color: #2e7d32; font-weight: bold; }");
        break;
    case MobileClient::ConnectionState::Connecting:
        m_statusLabel->setText(QStringLiteral("\u6B63\u5728\u8FDE\u63A5..."));
        m_statusLabel->setStyleSheet(
            "QLabel { background: #fff3e0; border-radius: 6px; font-size: 14px; color: #e65100; }");
        break;
    case MobileClient::ConnectionState::Reconnecting:
        m_statusLabel->setText(
            QStringLiteral("\u6B63\u5728\u91CD\u8FDE... (\u7B2C %1 \u6B21)")
                .arg(m_client->reconnectAttempts()));
        m_statusLabel->setStyleSheet(
            "QLabel { background: #fff3e0; border-radius: 6px; font-size: 14px; color: #e65100; }");
        break;
    case MobileClient::ConnectionState::Error:
        m_statusLabel->setText(
            QStringLiteral("\u8FDE\u63A5\u9519\u8BEF: %1").arg(m_client->lastError()));
        m_statusLabel->setStyleSheet(
            "QLabel { background: #ffebee; border-radius: 6px; font-size: 14px; color: #c62828; }");
        break;
    default:
        m_statusLabel->setText(QStringLiteral("\u672A\u8FDE\u63A5"));
        m_statusLabel->setStyleSheet(
            "QLabel { background: #f5f5f5; border-radius: 6px; font-size: 14px; color: #666; }");
        break;
    }
}
