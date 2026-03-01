import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: devicePage

    property var devices: []

    Connections {
        target: deviceModel
        function onDeviceListUpdated() {
            devicePage.refreshDevices()
        }
        function onDeviceStatusChanged(devId, oldS, newS) {
            devicePage.refreshDevices()
        }
    }

    function refreshDevices() {
        var count = deviceModel.deviceCount()
        var list = []
        for (var i = 0; i < count; i++) {
            var d = deviceModel.deviceAt(i)
            list.push({
                deviceId: d.deviceId,
                deviceType: d.deviceType,
                status: d.status
            })
        }
        devices = list
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        Label {
            text: "设备列表"
            font.pixelSize: 22
            font.bold: true
        }

        // Filter row
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                text: "全部"
                flat: true
                font.pixelSize: 12
                onClicked: devicePage.refreshDevices()
            }
            Button {
                text: "相机"
                flat: true
                font.pixelSize: 12
                onClicked: {
                    var all = deviceModel.devicesByType("camera")
                    // simplified filter
                    devicePage.refreshDevices()
                }
            }
            Button {
                text: "机器人"
                flat: true
                font.pixelSize: 12
            }
            Button {
                text: "PLC"
                flat: true
                font.pixelSize: 12
            }
        }

        // Device list
        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 8
            model: devicePage.devices

            delegate: Rectangle {
                width: ListView.view.width
                height: 72
                radius: 10
                color: "white"
                border.color: "#e0e0e0"

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 12

                    // Device type icon placeholder
                    Rectangle {
                        width: 48; height: 48; radius: 8
                        color: {
                            switch (modelData.deviceType) {
                                case "camera": return "#e3f2fd"
                                case "robot": return "#e8f5e9"
                                case "plc": return "#fff3e0"
                                default: return "#f5f5f5"
                            }
                        }
                        Label {
                            anchors.centerIn: parent
                            text: {
                                switch (modelData.deviceType) {
                                    case "camera": return "CAM"
                                    case "robot": return "ROB"
                                    case "plc": return "PLC"
                                    default: return "DEV"
                                }
                            }
                            font.pixelSize: 12
                            font.bold: true
                            color: "#333"
                        }
                    }

                    ColumnLayout {
                        spacing: 4
                        Label {
                            text: modelData.deviceId
                            font.pixelSize: 16
                            font.bold: true
                        }
                        Label {
                            text: modelData.deviceType
                            font.pixelSize: 12
                            color: "#666"
                        }
                    }

                    Item { Layout.fillWidth: true }

                    // Status badge
                    Rectangle {
                        width: statusLabel.width + 16
                        height: 28
                        radius: 14
                        color: {
                            switch (modelData.status) {
                                case "Ready": return "#e8f5e9"
                                case "Busy": return "#fff3e0"
                                case "Error": return "#ffebee"
                                case "Disconnected": return "#f5f5f5"
                                default: return "#f5f5f5"
                            }
                        }
                        Label {
                            id: statusLabel
                            anchors.centerIn: parent
                            text: {
                                switch (modelData.status) {
                                    case "Ready": return "就绪"
                                    case "Busy": return "忙碌"
                                    case "Error": return "错误"
                                    case "Disconnected": return "断开"
                                    default: return modelData.status
                                }
                            }
                            font.pixelSize: 12
                            color: {
                                switch (modelData.status) {
                                    case "Ready": return "#2e7d32"
                                    case "Busy": return "#e65100"
                                    case "Error": return "#c62828"
                                    default: return "#666"
                                }
                            }
                        }
                    }
                }
            }

            // Empty state
            Label {
                anchors.centerIn: parent
                text: "暂无设备\n请先连接服务器"
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: 16
                color: "#999"
                visible: devicePage.devices.length === 0
            }
        }
    }
}
