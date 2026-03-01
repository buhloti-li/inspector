import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: controlPage

    property double speedValue: 100.0

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 16

        Label {
            text: "远程控制"
            font.pixelSize: 22
            font.bold: true
        }

        // Status display
        Rectangle {
            Layout.fillWidth: true
            height: 60
            radius: 10
            color: root.isConnected ? "#e8f5e9" : "#ffebee"

            Label {
                anchors.centerIn: parent
                text: root.isConnected
                      ? "已连接 - " + root.currentState
                      : "未连接服务器"
                font.pixelSize: 16
                color: root.isConnected ? "#2e7d32" : "#c62828"
            }
        }

        // Main controls
        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: 12
            rowSpacing: 12

            // Start button
            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: 80
                text: "启动周期"
                font.pixelSize: 18
                enabled: root.isConnected
                background: Rectangle {
                    radius: 12
                    color: parent.enabled
                           ? (parent.pressed ? "#388e3c" : "#4caf50")
                           : "#ccc"
                }
                contentItem: Label {
                    text: parent.text
                    font: parent.font
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: commandService.startCycle()
            }

            // Stop button
            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: 80
                text: "停止"
                font.pixelSize: 18
                enabled: root.isConnected
                background: Rectangle {
                    radius: 12
                    color: parent.enabled
                           ? (parent.pressed ? "#1565c0" : "#1e88e5")
                           : "#ccc"
                }
                contentItem: Label {
                    text: parent.text
                    font: parent.font
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: commandService.stopCycle()
            }
        }

        // Emergency stop
        Button {
            Layout.fillWidth: true
            Layout.preferredHeight: 100
            text: "紧 急 停 止"
            font.pixelSize: 24
            font.bold: true
            enabled: root.isConnected
            background: Rectangle {
                radius: 16
                color: parent.enabled
                       ? (parent.pressed ? "#b71c1c" : "#d32f2f")
                       : "#ccc"
                border.color: parent.enabled ? "#b71c1c" : "#aaa"
                border.width: 3
            }
            contentItem: Label {
                text: parent.text
                font: parent.font
                color: "white"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            onClicked: commandService.emergencyStop()
        }

        // Speed control
        Rectangle {
            Layout.fillWidth: true
            height: 100
            radius: 10
            color: "white"
            border.color: "#e0e0e0"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16

                RowLayout {
                    Label {
                        text: "运行速度"
                        font.pixelSize: 14
                        color: "#666"
                    }
                    Item { Layout.fillWidth: true }
                    Label {
                        text: controlPage.speedValue.toFixed(0) + "%"
                        font.pixelSize: 18
                        font.bold: true
                        color: "#1a73e8"
                    }
                }

                Slider {
                    Layout.fillWidth: true
                    from: 10
                    to: 100
                    stepSize: 5
                    value: controlPage.speedValue
                    onMoved: {
                        controlPage.speedValue = value
                        if (root.isConnected)
                            commandService.setSpeed(value)
                    }
                }
            }
        }

        // Auto mode toggle
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
                    text: "自动模式"
                    font.pixelSize: 16
                }

                Item { Layout.fillWidth: true }

                Switch {
                    enabled: root.isConnected
                    onToggled: {
                        commandService.setAutoMode(checked)
                    }
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}
