import QtQuick
import QtQuick.Layouts
import Qt.labs.StyleKit
import ScootNorge

// Bottom sheet with operator, range, zone and appearance settings.
Popup {
    id: sheet

    required property MobilityService service
    required property var settings
    required property var styleObject   // the active StyleKit Style

    function isEnabled(key) {
        return Array.from(settings.enabledOperators).indexOf(key) >= 0
    }
    function setEnabled(key, on) {
        let list = Array.from(settings.enabledOperators).filter(k => k !== key)
        if (on)
            list.push(key)
        settings.enabledOperators = list
    }

    modal: true
    focus: true
    width: parent ? parent.width : 400
    x: 0
    y: parent ? parent.height - height : 0

    ColumnLayout {
        width: parent.width
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: qsTr("Filters")
                font.pixelSize: 20
                font.bold: true
            }
            Item { Layout.fillWidth: true }
            Button {
                flat: true
                text: qsTr("Done")
                onClicked: sheet.close()
            }
        }

        // --- Operators -----------------------------------------------------
        Label {
            text: qsTr("Operators")
            font.bold: true
        }
        Repeater {
            model: sheet.service.operators
            delegate: RowLayout {
                id: opRow
                required property var modelData
                Layout.fillWidth: true
                spacing: 10
                Rectangle {
                    width: 12
                    height: 12
                    radius: 6
                    color: opRow.modelData.color
                }
                Label {
                    Layout.fillWidth: true
                    text: opRow.modelData.name
                }
                Switch {
                    StyleVariation.variations: [opRow.modelData.key]
                    checked: sheet.isEnabled(opRow.modelData.key)
                    onToggled: {
                        sheet.setEnabled(opRow.modelData.key, checked)
                        checked = Qt.binding(() => sheet.isEnabled(opRow.modelData.key))
                    }
                }
            }
        }

        // --- Range -----------------------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            Label {
                text: qsTr("Minimum range")
                font.bold: true
            }
            Item { Layout.fillWidth: true }
            Label {
                text: sheet.settings.minRangeKm <= 0 ? qsTr("Any") : qsTr("≥ %1 km").arg(sheet.settings.minRangeKm)
            }
        }
        Slider {
            Layout.fillWidth: true
            from: 0
            to: 25
            stepSize: 1
            value: sheet.settings.minRangeKm
            onMoved: sheet.settings.minRangeKm = value
        }
        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            StyleVariation.variations: ["caption"]
            font.pixelSize: 12
            text: qsTr("Hides scooters that can't make the trip. Scooters without range data are kept.")
        }

        // --- Zones -------------------------------------------------------------
        Label {
            text: qsTr("Show zones on the map")
            font.bold: true
        }
        CheckBox {
            text: qsTr("No-riding zones")
            checked: sheet.settings.showNoRide
            onToggled: sheet.settings.showNoRide = checked
        }
        CheckBox {
            text: qsTr("No-parking zones")
            checked: sheet.settings.showNoParking
            onToggled: sheet.settings.showNoParking = checked
        }
        CheckBox {
            text: qsTr("Slow zones")
            checked: sheet.settings.showSlow
            onToggled: sheet.settings.showSlow = checked
        }

        // --- Appearance ------------------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            Label {
                Layout.fillWidth: true
                text: qsTr("Theme")
                font.bold: true
            }
            ComboBox {
                model: sheet.styleObject.themeNames
                Component.onCompleted: currentIndex = Math.max(0, find(sheet.settings.theme))
                onActivated: sheet.settings.theme = currentText
            }
        }

        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            StyleVariation.variations: ["caption"]
            font.pixelSize: 11
            text: qsTr("Live data: Entur Mobility API (NLOD licence). Feeds: %1")
                  .arg(sheet.service.systems.length > 0 ? sheet.service.systems.join(", ") : qsTr("none"))
        }
    }
}
