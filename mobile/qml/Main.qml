import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    visible: true
    width: 390
    height: 844
    title: qsTr("工业监控")
    color: "#f0f2f5"

    // Connection state tracking
    property bool isConnected: false
    property string currentState: "Unknown"
    property int alertBadge: 0

    Connections {
        target: mobileClient
        function onConnected() { root.isConnected = true }
        function onDisconnected() { root.isConnected = false }
    }

    Connections {
        target: statusProvider
        function onStateChanged(newState) {
            switch (newState) {
                case 1: root.currentState = "空闲"; break
                case 2: root.currentState = "运行中"; break
                case 3: root.currentState = "已停止"; break
                case 4: root.currentState = "故障"; break
                case 5: root.currentState = "降级"; break
                default: root.currentState = "未知"; break
            }
        }
    }

    Connections {
        target: alertService
        function onAlertReceived(id, severity, msg) {
            root.alertBadge = alertService.unacknowledgedCount()
        }
    }

    header: ToolBar {
        background: Rectangle { color: "#1a73e8" }
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16

            Label {
                text: "工业监控"
                color: "white"
                font.pixelSize: 20
                font.bold: true
            }

            Item { Layout.fillWidth: true }

            // Connection indicator
            Rectangle {
                width: 12; height: 12; radius: 6
                color: root.isConnected ? "#4caf50" : "#f44336"
            }
        }
    }

    // Main content with swipe navigation
    SwipeView {
        id: swipeView
        anchors.fill: parent
        currentIndex: tabBar.currentIndex

        DashboardPage {}
        DeviceListPage {}
        ControlPage {}
        AlertPage {}
        SettingsPage {}
    }

    footer: TabBar {
        id: tabBar
        currentIndex: swipeView.currentIndex
        background: Rectangle {
            color: "white"
            Rectangle { width: parent.width; height: 1; color: "#e0e0e0" }
        }

        TabButton {
            text: "仪表盘"
            icon.source: ""
            font.pixelSize: 11
        }
        TabButton {
            text: "设备"
            font.pixelSize: 11
        }
        TabButton {
            text: "控制"
            font.pixelSize: 11
        }
        TabButton {
            text: root.alertBadge > 0
                  ? "告警(" + root.alertBadge + ")" : "告警"
            font.pixelSize: 11
        }
        TabButton {
            text: "设置"
            font.pixelSize: 11
        }
    }
}
