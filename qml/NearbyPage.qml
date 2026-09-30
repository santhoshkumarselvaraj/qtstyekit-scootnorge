import QtQuick
import QtQuick.Layouts
import Qt.labs.StyleKit
import ScootNorge

// All matching scooters as a list, closest first - handy when the map is crowded.
Item {
    id: page

    required property MobilityService service
    required property var ui

    signal vehicleChosen(var vehicle)

    ListView {
        id: list
        anchors.fill: parent
        clip: true
        model: page.service.vehicles

        header: Pane {
            width: ListView.view.width
            padding: 14
            ColumnLayout {
                width: parent.width
                spacing: 2
                Label {
                    text: qsTr("Closest to the map centre")
                    font.pixelSize: 18
                    font.bold: true
                }
                Label {
                    StyleVariation.variations: ["caption"]
                    text: page.service.vehicles.count < page.service.vehicles.totalCount
                          ? qsTr("Showing the nearest %1 of %2").arg(page.service.vehicles.count)
                                                                 .arg(page.service.vehicles.totalCount)
                          : qsTr("%1 scooters").arg(page.service.vehicles.count)
                }
            }
        }

        delegate: ItemDelegate {
            id: row
            required property int index
            required property string operatorName
            required property color operatorColor
            required property int battery
            required property real rangeKm
            required property real distance
            required property real estimate
            required property string currency
            required property bool cheapest

            width: ListView.view.width
            onClicked: page.vehicleChosen(page.service.vehicles.get(row.index))

            contentItem: RowLayout {
                spacing: 12

                Rectangle {
                    Layout.preferredWidth: 36
                    Layout.preferredHeight: 36
                    radius: 18
                    color: row.operatorColor
                    border.width: row.cheapest ? 3 : 0
                    border.color: page.ui.best
                    Text {
                        anchors.centerIn: parent
                        text: row.operatorName.charAt(0)
                        color: "white"
                        font.bold: true
                        font.pixelSize: 16
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    RowLayout {
                        Label {
                            text: row.operatorName
                            font.bold: true
                        }
                        Label {
                            StyleVariation.variations: ["caption"]
                            text: Fmt.distance(row.distance) + " · " + Fmt.walkMinutes(row.distance)
                        }
                    }
                    RowLayout {
                        spacing: 8
                        ProgressBar {
                            Layout.preferredWidth: 70
                            from: 0
                            to: 100
                            value: Math.max(0, row.battery)
                            StyleVariation.variations: Fmt.batteryVariations(row.battery)
                        }
                        Label {
                            StyleVariation.variations: ["caption"]
                            font.pixelSize: 12
                            text: (row.battery >= 0 ? row.battery + " % · " : "") + Fmt.range(row.rangeKm)
                        }
                    }
                }

                Label {
                    font.bold: true
                    text: Fmt.money(row.estimate, row.currency)
                }
            }
        }

        Label {
            anchors.centerIn: parent
            visible: list.count === 0 && !page.service.loading
            StyleVariation.variations: ["caption"]
            text: qsTr("No scooters match your filters here.")
        }
    }
}
