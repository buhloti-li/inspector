import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: dashboardPage

    property int cycleCount: 0
    property int successCount: 0
    property int failCount: 0
    property string recipe: "---"
    property bool autoMode: false

    Connections {
        target: statusProvider
        function onCycleCountChanged(total, success, fail) {
            dashboardPage.cycleCount = total
            dashboardPage.successCount = success
            dashboardPage.failCount = fail
        }
        function onRecipeChanged(r) { dashboardPage.recipe = r }
        function onAutoModeChanged(a) { dashboardPage.autoMode = a }
    }

    ScrollView {
        anchors.fill: parent
        anchors.margins: 16

        ColumnLayout {
            width: dashboardPage.width - 32
            spacing: 16

            // Status card
            Rectangle {
                Layout.fillWidth: true
                height: 120
                radius: 12
                color: {
                    switch (root.currentState) {
                        case "运行中": return "#e8f5e9"
                        case "故障": return "#ffebee"
                        case "降级": return "#fff3e0"
                        case "已停止": return "#e3f2fd"
                        default: return "#f5f5f5"
                    }
                }

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 8

                    Label {
                        text: "工作站状态"
                        font.pixelSize: 14
                        color: "#666"
                        Layout.alignment: Qt.AlignHCenter
                    }
                    Label {
                        text: root.currentState
                        font.pixelSize: 32
                        font.bold: true
                        color: {
                            switch (root.currentState) {
                                case "运行中": return "#2e7d32"
                                case "故障": return "#c62828"
                                case "降级": return "#e65100"
                                case "已停止": return "#1565c0"
                                default: return "#333"
                            }
                        }
                        Layout.alignment: Qt.AlignHCenter
                    }
                }
            }

            // Stats row
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                // Total cycles
                Rectangle {
                    Layout.fillWidth: true
                    height: 90
                    radius: 10
                    color: "white"
                    border.color: "#e0e0e0"

                    ColumnLayout {
                        anchors.centerIn: parent
                        Label {
                            text: dashboardPage.cycleCount
                            font.pixelSize: 28
                            font.bold: true
                            color: "#1a73e8"
                            Layout.alignment: Qt.AlignHCenter
                        }
                        Label {
                            text: "总周期"
                            font.pixelSize: 12
                            color: "#666"
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }
                }

                // Success
                Rectangle {
                    Layout.fillWidth: true
                    height: 90
                    radius: 10
                    color: "white"
                    border.color: "#e0e0e0"

                    ColumnLayout {
                        anchors.centerIn: parent
                        Label {
                            text: dashboardPage.successCount
                            font.pixelSize: 28
                            font.bold: true
                            color: "#4caf50"
                            Layout.alignment: Qt.AlignHCenter
                        }
                        Label {
                            text: "成功"
                            font.pixelSize: 12
                            color: "#666"
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }
                }

                // Fail
                Rectangle {
                    Layout.fillWidth: true
                    height: 90
                    radius: 10
                    color: "white"
                    border.color: "#e0e0e0"

                    ColumnLayout {
                        anchors.centerIn: parent
                        Label {
                            text: dashboardPage.failCount
                            font.pixelSize: 28
                            font.bold: true
                            color: "#f44336"
                            Layout.alignment: Qt.AlignHCenter
                        }
                        Label {
                            text: "失败"
                            font.pixelSize: 12
                            color: "#666"
                            Layout.alignment: Qt.AlignHCenter
                        }
                    }
                }
            }

            // Recipe & Auto mode
            Rectangle {
                Layout.fillWidth: true
                height: 80
                radius: 10
                color: "white"
                border.color: "#e0e0e0"

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 16

                    ColumnLayout {
                        Label {
                            text: "当前配方"
                            font.pixelSize: 12
                            color: "#666"
                        }
                        Label {
                            text: dashboardPage.recipe
                            font.pixelSize: 16
                            font.bold: true
                        }
                    }

                    Item { Layout.fillWidth: true }

                    ColumnLayout {
                        Label {
                            text: "运行模式"
                            font.pixelSize: 12
                            color: "#666"
                            Layout.alignment: Qt.AlignRight
                        }
                        Label {
                            text: dashboardPage.autoMode ? "自动" : "手动"
                            font.pixelSize: 16
                            font.bold: true
                            color: dashboardPage.autoMode ? "#4caf50" : "#ff9800"
                            Layout.alignment: Qt.AlignRight
                        }
                    }
                }
            }

            // Success rate
            Rectangle {
                Layout.fillWidth: true
                height: 60
                radius: 10
                color: "white"
                border.color: "#e0e0e0"

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 16

                    Label {
                        text: "成功率"
                        font.pixelSize: 14
                        color: "#666"
                    }
                    Item { Layout.fillWidth: true }
                    Label {
                        text: dashboardPage.cycleCount > 0
                              ? (dashboardPage.successCount * 100.0
                                 / dashboardPage.cycleCount).toFixed(1) + "%"
                              : "---"
                        font.pixelSize: 20
                        font.bold: true
                        color: "#1a73e8"
                    }
                }
            }
        }
    }
}
