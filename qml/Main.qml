import QtQuick 2.12
import QtQuick.Window 2.12
import CodeViewer 1.0
import QtQuick.Controls 2.12


Window {
    id: mainWindow
    width: 640
    height: 480
    visible: true
    title: qsTr("Code Viewer")
    color: "lightblue"

    Item {
        id: canvasManager

        anchors.fill: parent

        function addNodeConnection(node, connection) {
            let n = impl.nodes[node.functionName];
            n.connections.push(connection);
        }

        function addNode(node) {
            impl.nodes[node.functionName] = { ref: node, connections: [] };
        }

        function getNode(functionName) {
            return impl.nodes[functionName];
        }

        function deleteNode(node) {
            delete impl.nodes[node.name];
        }

        function rerender() {
            impl.requestPaint();
        }

        Canvas {
            id: impl

            property var nodes: ({})

            anchors.fill: parent
            z: -1
            contextType: "2d"

            function arrow(context, fromx, fromy, tox, toy) {
                context.lineWidth = 2;
                context.beginPath();
                context.moveTo(fromx, fromy);
                context.lineTo(tox, toy);
                context.stroke();

            }

            onPaint: {
                const ctx = getContext("2d");
                ctx.clearRect(0, 0, parent.width, parent.height);
                for (let fromFN in impl.nodes) {
                    const from = impl.nodes[fromFN];
                    if (!from.connections) {
                        continue;
                    }
                    for (let toFN of from.connections) {
                        const to = impl.nodes[toFN];
                        arrow(ctx,
                              from.ref.x + from.ref.width / 2,
                              from.ref.y + from.ref.height / 2,
                              to.ref.x + to.ref.width / 2,
                              to.ref.y + to.ref.height / 2
                              );
                    }
                }
            }
        }
    }

    Component {
        id: graphNode

        GraphNode {}
    }

    function setupSlots(node) {
        node.linkActivated.connect((link) => {
                                       const props = {
                                           x: 0,
                                           y: 0,
                                           code: BackendApi.getFunctionCode(link),
                                           functionName: BackendApi.getFunctionName(link)
                                       };
                                       const it = canvasManager.getNode(props.functionName);
                                       if (!it) {
                                           let n = graphNode.createObject(mainWindow, props);
                                           canvasManager.addNode(n);
                                           setupSlots(n);
                                       }
                                       canvasManager.addNodeConnection(node, props.functionName);
                                       canvasManager.rerender();
                                   });
        node.xChanged.connect(() => canvasManager.rerender());
        node.yChanged.connect(() => canvasManager.rerender());
    }

    Component.onCompleted: {
        let node = graphNode.createObject(mainWindow, {
                                              x: 0,
                                              y: 0,
                                              code: BackendApi.getFunctionCode("c:@F@main#I#**C#"),
                                              functionName: "main()"
                                          });
        setupSlots(node);
        canvasManager.addNode(node);
    }
}

