import QtQuick 2.5
import QtQuick.Controls 2.5
import net.asivery.AppLoad 1.0

Rectangle {
    anchors.fill: parent
    property var counter: 1

    // This is an endpoint. There can be multiple endpoints throughout one application
    // All endpoints will get all messages sent from the backend
    AppLoad {
        id: appload
        applicationID: "example"
        onMessageReceived: (type, contents) => {
            switch(type){
                case 101:
                    backendText.text = `Backend says: ${contents}`
                    break;
            }
        }
    }

    Text {
        id: text
        anchors.centerIn: parent
        width: parent.width
        horizontalAlignment: Text.AlignHCenter
        text: 'Hello, world!'
        font.pointSize: 48
    }

    Rectangle {
        id: button
        anchors.top: text.bottom
        anchors.topMargin: 20
        anchors.horizontalCenter: parent.horizontalCenter
        border.width: 2
        border.color: "black"
        width: 300
        height: 150

        Text {
            text: "Send 'Hello'"
            font.pointSize: 24
            anchors.centerIn: parent
        }

        MouseArea {
            anchors.fill: parent
            onClicked: () => appload.sendMessage(1, "Hello")
        }
    }

    Text {
        id: backendText
        anchors.top: button.bottom
        anchors.topMargin: 20
        width: parent.width
        horizontalAlignment: Text.AlignHCenter
        text: ''
        font.pointSize: 48
    }
}
