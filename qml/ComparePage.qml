import QtQuick
import QtQuick.Layouts
import Qt.labs.StyleKit
import ScootNorge

// "Which operator is cheapest for my trip, right here?" - the thing Ruter doesn't answer.
Item {
    id: page

    required property MobilityService service
    required property var ui
    property int tripMinutes: 10

    signal tripMinutesEdited(int minutes)

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth

        ColumnLayout {
            width: parent.width
            spacing: 14

            Item { implicitHeight: 4 }

            ColumnLayout {
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                spacing: 2
                Label {
                    text: qsTr("Compare prices")
                    font.pixelSize: 22
                    font.bold: true
                }
                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    StyleVariation.variations: ["caption"]
                    text: qsTr("Cheapest available scooter per operator within %1 of the map centre.")
                          .arg(Fmt.distance(page.service.vehicles.compareRadiusMeters))
                }
            }

            Frame {
                Layout.fillWidth: true
                Layout.leftMargin: 12
                Layout.rightMargin: 12

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

                    RowLayout {
                        Layout.fillWidth: true
                        Label {
                            text: qsTr("Trip length")
                            font.bold: true
                        }
                        Item { Layout.fillWidth: true }
                        Label {
                            text: qsTr("%1 min").arg(page.tripMinutes)
                            font.bold: true
                        }
                    }
                    Slider {
                        Layout.fillWidth: true
                        from: 1
                        to: 60
                        stepSize: 1
                        value: page.tripMinutes
                        onMoved: page.tripMinutesEdited(Math.round(value))
                    }

                    Label {
                        text: qsTr("Search radius")
                        font.bold: true
                    }
                    RowLayout {
                        spacing: 8
                        Repeater {
                            model: [250, 500, 1000]
                            delegate: Button {
                                required property int modelData
                                text: Fmt.distance(modelData)
                                checked: page.service.vehicles.compareRadiusMeters === modelData
                                flat: !checked
                                onClicked: page.service.vehicles.compareRadiusMeters = modelData
                            }
                        }
                    }
                }
            }

            Repeater {
                model: page.service.vehicles.comparison

                delegate: Frame {
                    id: offer
                    required property var modelData
                    Layout.fillWidth: true
                    Layout.leftMargin: 12
                    Layout.rightMargin: 12
                    StyleVariation.variations: modelData.best ? ["best"] : []

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 6

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            Rectangle {
                                width: 14
                                height: 14
                                radius: 7
                                color: offer.modelData.color
                            }
                            Label {
                                text: offer.modelData.name
                                font.pixelSize: 17
                                font.bold: true
                            }
                            Rectangle {
                                visible: offer.modelData.best === true
                                radius: 8
                                color: page.ui.best
                                implicitWidth: bestLabel.implicitWidth + 12
                                implicitHeight: bestLabel.implicitHeight + 4
                                Text {
                                    id: bestLabel
                                    anchors.centerIn: parent
                                    text: qsTr("Best price")
                                    font.pixelSize: 11
                                    font.bold: true
                                    color: "#3D2C00"
                                }
                            }
                            Item { Layout.fillWidth: true }
                            Label {
                                visible: offer.modelData.available
                                text: "≈ " + Fmt.money(offer.modelData.estimate, offer.modelData.currency)
                                font.pixelSize: 20
                                font.bold: true
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            StyleVariation.variations: ["caption"]
                            text: !offer.modelData.available
                                  ? qsTr("No scooters available in this city right now.")
                                  : offer.modelData.nearbyCount > 0
                                    ? qsTr("%1 nearby · best one is %2 away (%3)")
                                          .arg(offer.modelData.nearbyCount)
                                          .arg(Fmt.distance(offer.modelData.distance))
                                          .arg(Fmt.walkMinutes(offer.modelData.distance))
                                    : qsTr("None within range · nearest is %1 away")
                                          .arg(Fmt.distance(offer.modelData.nearestDistance))
                        }
                        Label {
                            visible: offer.modelData.available
                            StyleVariation.variations: ["caption"]
                            text: offer.modelData.unlockFee >= 0
                                  ? qsTr("Unlock %1 + %2/min")
                                        .arg(Fmt.money(offer.modelData.unlockFee, offer.modelData.currency))
                                        .arg(Fmt.money(offer.modelData.perMinute, offer.modelData.currency))
                                  : qsTr("No published pricing")
                        }
                        Button {
                            Layout.fillWidth: true
                            visible: offer.modelData.available
                            text: qsTr("Ride with %1").arg(offer.modelData.name)
                            StyleVariation.variations: [offer.modelData.key]
                            onClicked: page.service.vehicles.openInOperatorApp(offer.modelData.vehicleId)
                        }
                    }
                }
            }

            Label {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 16
                wrapMode: Text.WordWrap
                StyleVariation.variations: ["caption"]
                font.pixelSize: 11
                text: qsTr("Estimates use each operator's published GBFS list prices via Entur. "
                           + "Ride passes, discounts and surge pricing are not included - "
                           + "the operator's app shows the final price before you unlock.")
            }
        }
    }
}
