import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: settingsPage

    property string hostValue: "192.168.1.100"
    property string portValue: "8080"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 16

        Label {
            text: "连接设置"
            font.pixelSize: 22
            font.bold: true
        }

        // Connection status card
        Rectangle {
            Layout.fillWidth: true
            height: 50
            radius: 10
            color: root.isConnected ? "#e8f5e9" : "#f5f5f5"
            border.color: root.isConnected ? "#4caf50" : "#e0e0e0"

            Label {
                anchors.centerIn: parent
                text: root.isConnected
                      ? "已连接到 " + mobileClient.serverHost()
                        + ":" + mobileClient.serverPort()
                      : "未连接"
                font.pixelSize: 14
                color: root.isConnected ? "#2e7d32" : "#666"
            }
        }

        // Server settings
        Rectangle {
            Layout.fillWidth: true
            height: serverCol.height + 32
            radius: 10
            color: "white"
            border.color: "#e0e0e0"

            ColumnLayout {
                id: serverCol
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 16
                spacing: 12

                Label {
                    text: "服务器地址"
                    font.pixelSize: 14
                    color: "#666"
                }

                TextField {
                    Layout.fillWidth: true
                    placeholderText: "IP 地址"
                    text: settingsPage.hostValue
                    onTextChanged: settingsPage.hostValue = text
                    inputMethodHints: Qt.ImhPreferNumbers
                }

                Label {
                    text: "端口"
                    font.pixelSize: 14
                    color: "#666"
                }

                TextField {
                    Layout.fillWidth: true
                    placeholderText: "端口号"
                    text: settingsPage.portValue
                    onTextChanged: settingsPage.portValue = text
                    inputMethodHints: Qt.ImhDigitsOnly
                    validator: IntValidator { bottom: 1; top: 65535 }
                }
            }
        }

        // Connect/Disconnect button
        Button {
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            text: root.isConnected ? "断开连接" : "连接服务器"
            font.pixelSize: 16
            background: Rectangle {
                radius: 10
                color: root.isConnected
                       ? (parent.pressed ? "#c62828" : "#f44336")
                       : (parent.pressed ? "#1565c0" : "#1a73e8")
            }
            contentItem: Label {
                text: parent.text
                font: parent.font
                color: "white"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            onClicked: {
                if (root.isConnected) {
                    mobileClient.disconnectFromServer()
                    statusProvider.stopPolling()
                } else {
                    var port = parseInt(settingsPage.portValue)
                    if (mobileClient.connectToServer(settingsPage.hostValue, port)) {
                        statusProvider.startPolling(1000)
                    }
                }
            }
        }

        // Reconnect settings
        Rectangle {
            Layout.fillWidth: true
            height: reconCol.height + 32
            radius: 10
            color: "white"
            border.color: "#e0e0e0"

            ColumnLayout {
                id: reconCol
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 16
                spacing: 8

                Label {
                    text: "重连设置"
                    font.pixelSize: 14
                    font.bold: true
                }

                RowLayout {
                    Layout.fillWidth: true
                    Label {
                        text: "最大重试次数"
                        font.pixelSize: 13
                    }
                    Item { Layout.fillWidth: true }
                    SpinBox {
                        from: 1; to: 20
                        value: mobileClient.maxReconnectAttempts()
                        onValueModified: mobileClient.setMaxReconnectAttempts(value)
                    }
                }
            }
        }

        // App info
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
                    text: "版本"
                    font.pixelSize: 14
                    color: "#666"
                }
                Item { Layout.fillWidth: true }
                Label {
                    text: "1.0.0"
                    font.pixelSize: 14
                    color: "#333"
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}
