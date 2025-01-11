import QtQuick 2.5
import QtQuick.Window 2.2
import QtQuick.Controls 2.5
import net.asivery.AppLoad 1.0

Rectangle {
    id: root
    anchors.fill: parent
    color: "#f0f0f0"

    AppLoadCoordinator {
        id: coord

        onUnloading: () => {
            if(!coord.loaded) return;
            if(loader.item.unloading) loader.item.unloading();
        }
    }

    function close() {
        coord.close();
    }

    Loader {
        id: loader
        source: coord.applicationQMLRoot
        active: coord.loaded
        anchors.fill: parent

        Connections {
            target: loader.item
            function onClose() {
                root.close();
            }
        }
    }

    AppLoadLibrary {
        id: library
    }

    Rectangle {
        visible: !coord.loaded
        anchors.fill: parent
        Rectangle {
            id: header
            anchors.top: parent.top
            width: parent.width
            height: 100

            Rectangle {
                anchors.right: parent.right
                anchors.top: parent.top
                width: 100
                height: 100
                border.width: 2
                border.color: "black"
                Text {
                    anchors.fill: parent
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    text: "Reload"
                    font.pointSize: 20
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: () => {
                        library.reloadList();
                    }
                }
            }

            Text {
                anchors.fill: parent
                text: "AppLoad"
                font.pointSize: 40
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
        GridView {
            anchors.top: header.bottom
            anchors.right: parent.right
            anchors.topMargin: 20
            anchors.margins: 10
            width: parent.width - 20
            height: parent.height - 100
            id: gridView
            model: library.applications
            cellWidth: 200 + 20
            cellHeight: 200 + 20

            delegate: Rectangle {
                required property var modelData
                anchors.margins: 10
                width: gridView.cellWidth - 20
                height: gridView.cellHeight - 20

                Image {
                    anchors.top: parent.top
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.margins: 10
                    id: appIcon
                    source: modelData.icon
                    width: 150
                    height: 150
                }

                Text {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: appIcon.bottom
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                    anchors.topMargin: 5
                    text: modelData.name
                    font.pointSize: 24
                    elide: Text.ElideRight
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: () => {
                        coord.loadApplication(modelData.id);
                    }
                    onPressAndHold: () => {
                        // Create a new window
                        if(library.isFrontendRunningFor(modelData.id) && !modelData.canHaveMultipleFrontends) {
                            console.log("Cannot load multiple frontends for app. It doesn't support it.");
                            return;
                        }
                        if(windowArchetype.status !== Component.Ready) {
                            console.log("Window object not ready: " + windowArchetype.status);
                            console.log(windowArchetype.errorString());
                            return;
                        }
                        const win = windowArchetype.createObject(root, { x: 100, y: 100 });
                        if(win == null) {
                            console.log("Failed to instantiate a window object!");
                            return;
                        }

                        win.appName = modelData.name;
                        win.supportsScaling = modelData.supportsScaling;
                        win.closed.connect(() => win.destroy());
                        win.loadApplication(modelData.id);
                    }
                }
            }
        }
    }

    Component {
        id: windowArchetype

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

            // Frame for visual feedback during resizing
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

            // Draggable MouseArea to resize the rectangle
            MouseArea {
                id: resizeHandle
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                width: 100
                height: 100
                cursorShape: Qt.SizeFDiagCursor
                z: 10001

                // Variables to track resizing
                property real startX: 0
                property real startY: 0

                onPressed: {
                    // Store the initial position of the mouse when resizing starts
                    startX = mouse.x
                    startY = mouse.y
                    resizingFrame.visible = true
                }

                onReleased: {
                    // Finalize the resizing when released
                    resizingFrame.visible = false
                    // Calculate how much the mouse has moved
                    let deltaX = mouse.x - startX
                    let deltaY = mouse.y - startY

                    // Update the size of the rectangle based on the mouse movement
                    let width = Math.max(50, root.width + deltaX)  // Prevent going too small
                    let height = Math.max(50, root.height + deltaY)  // Prevent going too small
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
                    let width = Math.max(50, root.width + deltaX)  // Prevent going too small
                    let height = Math.max(50, root.height + deltaY)  // Prevent going too small

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
                        text: "C"
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
    }
}
