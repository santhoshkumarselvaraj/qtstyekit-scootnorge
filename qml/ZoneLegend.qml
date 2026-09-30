import QtQuick
import QtQuick.Layouts
import Qt.labs.StyleKit
import ScootNorge

// Small key for the coloured zone overlays.
Frame {
    id: legend

    property ZoneModel zones

    padding: 8

    ColumnLayout {
        spacing: 4

        Repeater {
            model: [
                { kind: ZoneModel.NoRide, label: qsTr("No riding"), shown: legend.zones.showNoRide },
                { kind: ZoneModel.NoParking, label: qsTr("No parking"), shown: legend.zones.showNoParking },
                { kind: ZoneModel.Slow, label: qsTr("Slow zone"), shown: legend.zones.showSlow },
            ]
            delegate: RowLayout {
                required property var modelData
                visible: modelData.shown
                spacing: 6
                Rectangle {
                    width: 14
                    height: 14
                    radius: 3
                    color: legend.zones.fillColorFor(modelData.kind)
                    border.color: legend.zones.borderColorFor(modelData.kind)
                    border.width: 1
                }
                Label {
                    text: modelData.label
                    font.pixelSize: 12
                }
            }
        }
    }
}
