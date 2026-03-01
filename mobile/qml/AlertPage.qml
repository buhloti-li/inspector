import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: alertPage

    property var alerts: []

    function refreshAlerts() {
        var allAlerts = alertService.allAlerts()
        var list = []
        for (var i = allAlerts.length - 1; i >= 0; i--) {
            var a = allAlerts[i]
            list.push({
                id: a.id,
                severity: a.severity,
                source: a.source,
                message: a.message,
                timestamp: a.timestamp,
                acknowledged: a.acknowledged
            })
        }
        alerts = list
    }

    Connections {
        target: alertService
        function onAlertReceived(id, sev, msg) { alertPage.refreshAlerts() }
        function onAlertAcknowledged(id) { alertPage.refreshAlerts() }
        function onAllAlertsAcknowledged() { alertPage.refreshAlerts() }
        function onAlertsCleared() { alertPage.refreshAlerts() }
    }

    Component.onCompleted: refreshAlerts()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        RowLayout {
            Layout.fillWidth: true

            Label {
                text: "告警通知"
                font.pixelSize: 22
                font.bold: true
            }

            Item { Layout.fillWidth: true }

            Button {
                text: "全部确认"
                flat: true
                font.pixelSize: 13
                enabled: alertService.unacknowledgedCount() > 0
                onClicked: alertService.acknowledgeAll()
            }

            Button {
                text: "清除"
                flat: true
                font.pixelSize: 13
                onClicked: alertService.clearHistory()
            }
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 8
            model: alertPage.alerts

            delegate: Rectangle {
                width: ListView.view.width
                height: alertContent.height + 24
                radius: 10
                color: modelData.acknowledged ? "#fafafa" : "white"
                border.color: {
                    switch (modelData.severity) {
                        case 3: return "#f44336" // Critical
                        case 2: return "#ff9800" // Error
                        case 1: return "#ffc107" // Warning
                        default: return "#e0e0e0" // Info
                    }
                }
                border.width: modelData.acknowledged ? 1 : 2
                opacity: modelData.acknowledged ? 0.7 : 1.0

                ColumnLayout {
                    id: alertContent
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 12
                    spacing: 4

                    RowLayout {
                        Layout.fillWidth: true

                        // Severity badge
                        Rectangle {
                            width: sevLabel.width + 12
                            height: 22
                            radius: 4
                            color: {
                                switch (modelData.severity) {
                                    case 3: return "#ffebee"
                                    case 2: return "#fff3e0"
                                    case 1: return "#fff8e1"
                                    default: return "#e3f2fd"
                                }
                            }
                            Label {
                                id: sevLabel
                                anchors.centerIn: parent
                                text: {
                                    switch (modelData.severity) {
                                        case 3: return "严重"
                                        case 2: return "错误"
                                        case 1: return "警告"
                                        default: return "信息"
                                    }
                                }
                                font.pixelSize: 11
                                font.bold: true
                                color: {
                                    switch (modelData.severity) {
                                        case 3: return "#c62828"
                                        case 2: return "#e65100"
                                        case 1: return "#f9a825"
                                        default: return "#1565c0"
                                    }
                                }
                            }
                        }

                        Label {
                            text: modelData.source
                            font.pixelSize: 12
                            color: "#666"
                        }

                        Item { Layout.fillWidth: true }

                        Label {
                            text: modelData.acknowledged ? "已确认" : ""
                            font.pixelSize: 11
                            color: "#999"
                        }
                    }

                    Label {
                        text: modelData.message
                        font.pixelSize: 14
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (!modelData.acknowledged)
                            alertService.acknowledgeAlert(modelData.id)
                    }
                }
            }

            // Empty state
            Label {
                anchors.centerIn: parent
                text: "暂无告警"
                font.pixelSize: 18
                color: "#999"
                visible: alertPage.alerts.length === 0
            }
        }
    }
}
