import QtQuick 2.12
import QtQuick.Controls 2.12

Item {
    id: root

    property string code
    property string functionName
    property int padding: 4
    property int borderSize: 2
    property string borderColor: "black"
    property string functionNameSectionColor: "lightgrey"
    property string sourceCodeSectionColor: "white"

    width: sourceCodeText.implicitWidth
    height: sourceCodeText.implicitHeight + functionNameText.implicitHeight

    signal linkActivated(string link)

    Rectangle {
        height: root.height
        width: root.width
        clip: true

        Rectangle {
            id: functionNameSection
            width: parent.width
            height: functionNameText.implicitHeight
            color: root.functionNameSectionColor
            anchors {
                top: parent.top
                left: parent.left
                right: parent.right
            }

            Text {
                id: functionNameText
                text: functionName
                font.bold: true
                color: "white"

                padding: root.padding
            }
        }

        Rectangle {
            id: sourceCodeSection
            width: parent.width
            height: parent.height - functionNameSection.height
            clip: true
            color: root.sourceCodeSectionColor
            anchors {
                top: functionNameSection.bottom
                left: parent.left
                right: parent.right
            }

            Text {
                id: sourceCodeText
                text: code
                textFormat: Text.RichText
                x: -hbar.position * width
                y: -vbar.position * height
                padding: root.padding
            }

            MouseArea {
                anchors.fill: parent
                drag {
                    target: root
                    smoothed: true
                }

                onClicked: {
                    let link = sourceCodeText.hoveredLink;
                    if (link) {
                        root.linkActivated(link);
                    }
                }
            }

            ScrollBar {
                id: vbar
                hoverEnabled: true
                active: hovered || pressed
                orientation: Qt.Vertical
                size: parent.height / sourceCodeText.height
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.bottom: parent.bottom
            }

            ScrollBar {
                id: hbar
                hoverEnabled: true
                active: hovered || pressed
                orientation: Qt.Horizontal
                size: parent.width / sourceCodeText.width
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
            }
        }

        // Bottom border
        Rectangle {
            width: parent.width
            height: root.borderSize
            z: 1
            color: root.borderColor
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right

            MouseArea {
                anchors.fill: parent
                drag {
                    target: parent
                    axis: Drag.YAxis
                }
                cursorShape: Qt.SizeVerCursor
                onMouseYChanged: {
                    if (drag.active) {
                        root.height += mouseY;
                    }
                }
            }
        }

        // Top border
        Rectangle {
            width: parent.width
            height: root.borderSize
            z: 1
            color: root.borderColor
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            MouseArea {
                anchors.fill: parent
                drag {
                    target: parent
                    axis: Drag.YAxis
                }
                cursorShape: Qt.SizeVerCursor
                onMouseYChanged: {
                    if (drag.active) {
                        root.height -= mouseY;
                        root.y +=  mouseY;
                    }
                }
            }
        }

        // Left border
        Rectangle {
            width: root.borderSize
            height: parent.height
            z: 1
            color: root.borderColor
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.bottom: parent.bottom

            MouseArea {
                anchors.fill: parent
                drag {
                    target: parent
                    axis: Drag.XAxis
                }
                cursorShape: Qt.SizeHorCursor
                onMouseYChanged: {
                    if (drag.active) {
                        root.width -= mouseX;
                        root.x +=  mouseX;
                    }
                }
            }
        }

        // Right border
        Rectangle {
            width: root.borderSize
            height: parent.height
            z: 1
            color: root.borderColor
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.bottom: parent.bottom

            MouseArea {
                anchors.fill: parent
                drag {
                    target: parent
                    axis: Drag.XAxis
                }
                cursorShape: Qt.SizeHorCursor
                onMouseYChanged: {
                    if (drag.active) {
                        root.width += mouseX;
                    }
                }
            }
        }

    }
}
