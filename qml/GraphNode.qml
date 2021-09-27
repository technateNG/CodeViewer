import QtQuick 2.12

Item {
    id: root

    property string code
    property string functionName

    width: childrenRect.width
    height: childrenRect.height

    signal linkActivated(string link)

    Rectangle {
        id: fieldRect

        color: "white"
        border.color: "black"
        border.width: 2
        width: childrenRect.width
        height: childrenRect.height

        Column {
            Rectangle {
                id: functionNameRect

                height: functionNameText.implicitHeight
                width: parent.width
                border.color: "black"
                border.width: 2
                color: "lightgrey"

                Text {
                    id: functionNameText

                    anchors.left: parent.left
                    anchors.leftMargin: 2
                    color: "white"
                    text: root.functionName
                    font.bold: true
                }
            }

            Item {
                height: sourceCodeText.implicitHeight
                width: sourceCodeText.implicitWidth

                Text {
                    id: sourceCodeText
                    font.pointSize: 10
                    anchors.centerIn: parent
                    textFormat: Text.RichText
                    text: code
                }
            }
        }
    }

    DragHandler {}

    TapHandler {
        onTapped: {
            let link = sourceCodeText.hoveredLink;
            if (link) {
                root.linkActivated(link);
            }
        }
    }
}
