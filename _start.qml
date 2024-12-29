import QtQuick 2.5
import QtQuick.Window 2.2
import QtQuick.Controls 2.5

Window {
    visible: true
    title: qsTr("AppLoad - PC emulator")
    width: 1620 / 4
    height: 2160 / 4
    id: window

    Rectangle {
        anchors.fill: parent
        color: "#f0f0f0"

        Loader {
            id: loader
            source: "./appload.qml"
            active: true

            width: 1620
            height: 2160

            transform: Scale {
                origin.x: loader.width / 2
                origin.y: loader.height / 2
                xScale: Math.min(window.width / loader.width, window.height / loader.height)
                yScale: Math.min(window.width / loader.width, window.height / loader.height)
            }

            anchors.centerIn: parent
        }
    }

    Button {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 20
        text: "Close"
        onClicked: () => loader.item.close()
    }
}
