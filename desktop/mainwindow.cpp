#include "mainwindow.h"
#include "dashboardwidget.h"
#include "devicetablewidget.h"
#include "controlpanelwidget.h"
#include "alertwidget.h"
#include "connectiondialog.h"

#include "mobile/mobileclient.h"
#include "mobile/workcellstatusprovider.h"
#include "mobile/remotecommandservice.h"
#include "mobile/alertnotificationservice.h"
#include "mobile/devicelistmodel.h"

#include <QMenuBar>
#include <QStatusBar>
#include <QAction>
#include <QMessageBox>
#include <QApplication>

MainWindow::MainWindow(MobileClient* client,
                       WorkcellStatusProvider* statusProvider,
                       RemoteCommandService* commandService,
                       AlertNotificationService* alertService,
                       DeviceListModel* deviceModel,
                       QWidget* parent)
    : QMainWindow(parent)
    , m_client(client)
    , m_statusProvider(statusProvider)
    , m_commandService(commandService)
    , m_alertService(alertService)
    , m_deviceModel(deviceModel)
{
    setupUi();
    setupMenuBar();
    setupStatusBar();

    connect(m_client, &MobileClient::connected, this, &MainWindow::onConnected);
    connect(m_client, &MobileClient::disconnected, this, &MainWindow::onDisconnected);
    connect(m_statusProvider, &WorkcellStatusProvider::stateChanged, this, &MainWindow::onStateChanged);
    connect(m_alertService, &AlertNotificationService::alertReceived, this, &MainWindow::onAlertReceived);
}

void MainWindow::setupUi()
{
    setWindowTitle(QStringLiteral("\u5DE5\u4E1A\u76D1\u63A7\u7CFB\u7EDF"));  // 工业监控系统
    resize(1200, 800);
    setMinimumSize(900, 600);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabPosition(QTabWidget::North);
    m_tabWidget->setDocumentMode(true);

    m_dashboardWidget = new DashboardWidget(m_statusProvider, this);
    m_deviceTableWidget = new DeviceTableWidget(m_deviceModel, this);
    m_controlPanelWidget = new ControlPanelWidget(m_client, m_commandService, m_statusProvider, this);
    m_alertWidget = new AlertWidget(m_alertService, this);

    m_tabWidget->addTab(m_dashboardWidget, QStringLiteral("\u4EEA\u8868\u76D8"));     // 仪表盘
    m_tabWidget->addTab(m_deviceTableWidget, QStringLiteral("\u8BBE\u5907\u5217\u8868")); // 设备列表
    m_tabWidget->addTab(m_controlPanelWidget, QStringLiteral("\u8FDC\u7A0B\u63A7\u5236")); // 远程控制
    m_tabWidget->addTab(m_alertWidget, QStringLiteral("\u544A\u8B66\u901A\u77E5"));     // 告警通知

    setCentralWidget(m_tabWidget);
}

void MainWindow::setupMenuBar()
{
    // File menu
    auto* fileMenu = menuBar()->addMenu(QStringLiteral("\u6587\u4EF6(&F)"));  // 文件

    auto* connectAction = fileMenu->addAction(QStringLiteral("\u8FDE\u63A5\u670D\u52A1\u5668(&C)..."));  // 连接服务器
    connectAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_K));
    connect(connectAction, &QAction::triggered, this, &MainWindow::showConnectionDialog);

    fileMenu->addSeparator();

    auto* exitAction = fileMenu->addAction(QStringLiteral("\u9000\u51FA(&X)"));  // 退出
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);

    // View menu
    auto* viewMenu = menuBar()->addMenu(QStringLiteral("\u89C6\u56FE(&V)"));  // 视图

    auto* dashAction = viewMenu->addAction(QStringLiteral("\u4EEA\u8868\u76D8"));
    dashAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_1));
    connect(dashAction, &QAction::triggered, this, [this]() { m_tabWidget->setCurrentIndex(0); });

    auto* devAction = viewMenu->addAction(QStringLiteral("\u8BBE\u5907\u5217\u8868"));
    devAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_2));
    connect(devAction, &QAction::triggered, this, [this]() { m_tabWidget->setCurrentIndex(1); });

    auto* ctrlAction = viewMenu->addAction(QStringLiteral("\u8FDC\u7A0B\u63A7\u5236"));
    ctrlAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_3));
    connect(ctrlAction, &QAction::triggered, this, [this]() { m_tabWidget->setCurrentIndex(2); });

    auto* alertAction = viewMenu->addAction(QStringLiteral("\u544A\u8B66\u901A\u77E5"));
    alertAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_4));
    connect(alertAction, &QAction::triggered, this, [this]() { m_tabWidget->setCurrentIndex(3); });

    // Help menu
    auto* helpMenu = menuBar()->addMenu(QStringLiteral("\u5E2E\u52A9(&H)"));  // 帮助
    auto* aboutAction = helpMenu->addAction(QStringLiteral("\u5173\u4E8E(&A)..."));  // 关于
    connect(aboutAction, &QAction::triggered, this, [this]() {
        QMessageBox::about(this,
            QStringLiteral("\u5173\u4E8E\u5DE5\u4E1A\u76D1\u63A7\u7CFB\u7EDF"),
            QStringLiteral("\u5DE5\u4E1A\u76D1\u63A7\u7CFB\u7EDF v1.0.0\n\n"
                          "Windows \u684C\u9762\u5BA2\u6237\u7AEF\n"
                          "\u7528\u4E8E\u8FDC\u7A0B\u76D1\u63A7\u548C\u63A7\u5236\u5DE5\u4E1A\u5DE5\u4F5C\u7AD9"));
    });
}

void MainWindow::setupStatusBar()
{
    m_connectionLabel = new QLabel(QStringLiteral("\u672A\u8FDE\u63A5"));  // 未连接
    m_connectionLabel->setStyleSheet("QLabel { color: #999; padding: 0 8px; }");

    m_stateLabel = new QLabel(QStringLiteral("\u72B6\u6001: \u672A\u77E5"));  // 状态: 未知
    m_stateLabel->setStyleSheet("QLabel { padding: 0 8px; }");

    statusBar()->addWidget(m_connectionLabel);
    statusBar()->addWidget(m_stateLabel);
}

void MainWindow::showConnectionDialog()
{
    ConnectionDialog dialog(m_client, m_statusProvider, this);
    dialog.exec();
}

void MainWindow::onConnected()
{
    updateConnectionIndicator(true);
    m_controlPanelWidget->setEnabled(true);
}

void MainWindow::onDisconnected()
{
    updateConnectionIndicator(false);
    m_controlPanelWidget->setEnabled(false);
}

void MainWindow::onStateChanged()
{
    auto state = m_statusProvider->workcellState();
    QString stateText;
    QString color;
    switch (state) {
    case WorkcellStatusProvider::WorkcellState::Idle:
        stateText = QStringLiteral("\u7A7A\u95F2"); color = "#333"; break;
    case WorkcellStatusProvider::WorkcellState::Running:
        stateText = QStringLiteral("\u8FD0\u884C\u4E2D"); color = "#2e7d32"; break;
    case WorkcellStatusProvider::WorkcellState::Stopped:
        stateText = QStringLiteral("\u5DF2\u505C\u6B62"); color = "#1565c0"; break;
    case WorkcellStatusProvider::WorkcellState::Fault:
        stateText = QStringLiteral("\u6545\u969C"); color = "#c62828"; break;
    case WorkcellStatusProvider::WorkcellState::Degraded:
        stateText = QStringLiteral("\u964D\u7EA7"); color = "#e65100"; break;
    default:
        stateText = QStringLiteral("\u672A\u77E5"); color = "#999"; break;
    }
    m_stateLabel->setText(QStringLiteral("\u72B6\u6001: %1").arg(stateText));
    m_stateLabel->setStyleSheet(QStringLiteral("QLabel { color: %1; padding: 0 8px; font-weight: bold; }").arg(color));
}

void MainWindow::updateConnectionIndicator(bool connected)
{
    if (connected) {
        m_connectionLabel->setText(
            QStringLiteral("\u2022 \u5DF2\u8FDE\u63A5 %1:%2")
                .arg(m_client->serverHost())
                .arg(m_client->serverPort()));
        m_connectionLabel->setStyleSheet("QLabel { color: #2e7d32; padding: 0 8px; font-weight: bold; }");
    } else {
        m_connectionLabel->setText(QStringLiteral("\u2022 \u672A\u8FDE\u63A5"));
        m_connectionLabel->setStyleSheet("QLabel { color: #999; padding: 0 8px; }");
    }
}

void MainWindow::onAlertReceived()
{
    int count = m_alertService->unacknowledgedCount();
    if (count > 0) {
        m_tabWidget->setTabText(3, QStringLiteral("\u544A\u8B66\u901A\u77E5 (%1)").arg(count));
    } else {
        m_tabWidget->setTabText(3, QStringLiteral("\u544A\u8B66\u901A\u77E5"));
    }
}
