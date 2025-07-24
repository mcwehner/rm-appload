import QtQuick 2.5
import QtQuick.Window 2.2
import QtQuick.Controls 2.5
import net.asivery.AppLoad 1.0

FocusScope {
    id: root
    Drag.active: topbarDragHandle.drag.active
    implicitWidth: 500
    implicitHeight: 700
    width: 1620 / 3
    height: 2160 / 3 + topbar.height
    z: 100

    property alias appName: _appName.text
    property bool supportsScaling: false
    signal closed
    function loadApplication(appId) {
        coord.loadApplication(appId);
    }

    /*
        marker-rebuild!!
    */

    /* Frame for visual feedback during resizing*/
    Rectangle {
        id: resizingFrame
        width: root.width
        height: root.height
        border.color: "gray"
        border.width: 2
        visible: false
        color: "transparent"
        z: 102
    }

    MouseArea {
        id: resizeHandle
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        width: 100
        height: 100
        cursorShape: Qt.SizeFDiagCursor
        z: 10001

        property real startX: 0
        property real startY: 0

        onPressed: {
            startX = mouse.x
            startY = mouse.y
            resizingFrame.visible = true
        }

        onReleased: {
            resizingFrame.visible = false
            let deltaX = mouse.x - startX
            let deltaY = mouse.y - startY

            let width = Math.max(50, root.width + deltaX)
            let height = Math.max(50, root.height + deltaY)
            startX = mouse.x
            startY = mouse.y

            if(!supportsScaling) {
                let scale = Math.min(height / 2160, width / 1620);

                width = 1620 * scale
                height = 2160 * scale + topbar.height
            }

            root.width = width;
            root.height = height;
        }

        onPositionChanged: {
            let deltaX = mouse.x - startX
            let deltaY = mouse.y - startY
            let width = Math.max(50, root.width + deltaX)
            let height = Math.max(50, root.height + deltaY)

            if(supportsScaling){
                resizingFrame.width = width
                resizingFrame.height = height
            } else {
                let scale = Math.min(height / 2160, width / 1620);

                resizingFrame.width = 1620 * scale
                resizingFrame.height = 2160 * scale + topbar.height
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "transparent"
        border.width: 2
        border.color: "black"
        z: 101
    }

    Rectangle {
        id: topbar
        width: parent.width
        height: 75
        color: root.focus ? "#A0A0A0" : "#707070"
        Text {
            id: _appName
            anchors.fill: parent
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.pointSize: 20
            text: "<AppLoad Window>"
        }

        MouseArea {
            id: topbarDragHandle
            anchors.fill: parent
            drag.target: root

            onClicked: () => {
                root.forceActiveFocus();
            }
        }

        Rectangle {
            width: parent.height
            height: parent.height
            anchors.right: parent.right
            border.width: 2
            border.color: "black"
            color: parent.color

            Text {
                anchors.fill: parent
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text: "X"
                font.pointSize: 20
            }

            MouseArea {
                anchors.fill: parent
                onClicked: () => {
                    coord.close();
                    root.closed();
                }

                onPressAndHold: () => {
                    coord.terminate();
                    root.closed();
                }
            }
        }

        Rectangle {
            width: parent.width
            height: 2
            anchors.bottom: parent.bottom
            border.width: 2
            border.color: "black"
        }
    }
    Rectangle {
        anchors.top: topbar.bottom
        height: parent.height - topbar.height
        width: parent.width
        clip: true

        AppLoadCoordinator {
            id: coord

            onUnloading: () => {
                if(!coord.loaded) return;
                let unloadingFunction;
                if(supportsScaling) {
                    unloadingFunction = loader.item.unloading;
                } else {
                    unloadingFunction = loaderScaled.item.unloading;
                }
                if(unloadingFunction) unloadingFunction();
                root.closed();
            }
        }

        Loader {
            id: loaderScaled
            source: coord.applicationQMLRoot
            visible: !supportsScaling
            active: coord.loaded && !supportsScaling
            anchors.centerIn: parent

            width: 1620
            height: 2160

            transform: Scale {
                origin.x: loaderScaled.width / 2
                origin.y: loaderScaled.height / 2
                xScale: Math.min(root.width / loaderScaled.width, root.height / loaderScaled.height)
                yScale: Math.min(root.width / loaderScaled.width, root.height / loaderScaled.height)
            }

            Connections {
                target: loaderScaled.item
                function onClose() {
                    coord.close();
                }
            }
        }
        Loader {
            id: loader
            source: coord.applicationQMLRoot
            visible: supportsScaling
            active: coord.loaded && supportsScaling
            anchors.fill: parent

            Connections {
                target: loader.item
                onClose: () => {
                    coord.close();
                }
            }
        }
    }
}
