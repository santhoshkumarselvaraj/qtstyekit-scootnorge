import QtQuick
import QtQuick.Layouts
import Qt.labs.StyleKit
import ScootNorge

// Details for the tapped scooter + hand-off to the operator's own app.
Frame {
    id: card

    property var vehicle: null
    property MobilityService service
    property int tripMinutes: 10

    signal closeRequested()

    readonly property bool valid: vehicle !== null && vehicle !== undefined

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Rectangle {
                width: 14
                height: 14
                radius: 7
                color: card.valid ? card.vehicle.operatorColor : "gray"
            }
            Label {
                text: card.valid ? card.vehicle.operatorName : ""
                font.pixelSize: 18
                font.bold: true
            }
            Rectangle {
                visible: card.valid && card.vehicle.cheapest
                radius: 8
                color: "#FFC53D"
                implicitWidth: bestText.implicitWidth + 12
                implicitHeight: bestText.implicitHeight + 4
                Text {
                    id: bestText
                    anchors.centerIn: parent
                    text: qsTr("Cheapest nearby")
                    font.pixelSize: 11
                    font.bold: true
                    color: "#3D2C00"
                }
            }
            Item { Layout.fillWidth: true }
            Button {
                flat: true
                text: "✕"
                onClicked: card.closeRequested()
            }
        }

        Label {
            StyleVariation.variations: ["caption"]
            text: card.valid
                  ? qsTr("%1 from map centre · %2").arg(Fmt.distance(card.vehicle.distance))
                                                   .arg(Fmt.walkMinutes(card.vehicle.distance))
                  : ""
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            ProgressBar {
                Layout.preferredWidth: 110
                from: 0
                to: 100
                value: card.valid ? Math.max(0, card.vehicle.battery) : 0
                StyleVariation.variations: card.valid ? Fmt.batteryVariations(card.vehicle.battery) : []
            }
            Label {
                Layout.fillWidth: true
                text: card.valid
                      ? (card.vehicle.battery >= 0 ? card.vehicle.battery + " %" : qsTr("battery n/a"))
                        + " · " + Fmt.range(card.vehicle.rangeKm)
                      : ""
            }
        }

        ColumnLayout {
            spacing: 2
            Label {
                font.pixelSize: 16
                font.bold: true
                text: card.valid
                      ? qsTr("≈ %1 for %2 min").arg(Fmt.money(card.vehicle.estimate, card.vehicle.currency))
                                                .arg(card.tripMinutes)
                      : ""
            }
            Label {
                StyleVariation.variations: ["caption"]
                text: card.valid && card.vehicle.unlockFee >= 0
                      ? qsTr("Unlock %1 + %2/min · list price, passes not included")
                            .arg(Fmt.money(card.vehicle.unlockFee, card.vehicle.currency))
                            .arg(Fmt.money(card.vehicle.perMinute, card.vehicle.currency))
                      : qsTr("Operator hasn't published pricing for this scooter")
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Button {
                Layout.fillWidth: true
                text: card.valid ? qsTr("Ride with %1").arg(card.vehicle.operatorName) : ""
                StyleVariation.variations: card.valid ? [card.vehicle.operatorKey] : []
                onClicked: card.service.vehicles.openInOperatorApp(card.vehicle.vehicleId)
            }
            Button {
                flat: true
                text: qsTr("Walk there")
                onClicked: card.service.vehicles.openWalkingDirections(card.vehicle.vehicleId)
            }
        }
    }
}
