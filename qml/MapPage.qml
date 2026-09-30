import QtQuick
import QtQuick.Layouts
import QtLocation
import QtPositioning
import Qt.labs.StyleKit
import ScootNorge

Item {
    id: page

    required property MobilityService service
    required property var settings
    required property var ui
    property var userPosition: null
    property var selected: null

    signal openFilters()

    readonly property bool hasUser: userPosition !== null
                                    && userPosition.latitudeValid && userPosition.longitudeValid

    function isEnabled(key) {
        return Array.from(settings.enabledOperators).indexOf(key) >= 0
    }
    function setEnabled(key, on) {
        let list = Array.from(settings.enabledOperators).filter(k => k !== key)
        if (on)
            list.push(key)
        settings.enabledOperators = list
    }
    function showVehicle(v) {
        selected = v
        mapView.map.center = v.geo
        if (mapView.map.zoomLevel < 16)
            mapView.map.zoomLevel = 16
    }
    function showCheapest() {
        const model = service.vehicles
        for (let i = 0; i < model.count; ++i) {
            const v = model.get(i)
            if (v.cheapest) {
                showVehicle(v)
                return
            }
        }
    }
    function centerOnUser() {
        if (hasUser) {
            mapView.map.center = userPosition.coordinate
            if (mapView.map.zoomLevel < 16)
                mapView.map.zoomLevel = 16
        }
    }
    function applyMapType() {
        // With a custom tile host, the OSM plugin exposes it as the CustomMap type.
        const types = mapView.map.supportedMapTypes
        for (let i = 0; i < types.length; ++i) {
            if (types[i].style === MapType.CustomMap) {
                mapView.map.activeMapType = types[i]
                return
            }
        }
    }
    function updateFocus() {
        const c = mapView.map.center
        service.vehicles.focus = c
        service.zones.focus = c
        // Show zones for roughly what is on screen.
        service.zones.radiusMeters = Math.min(8000, Math.max(800, 2500 * Math.pow(2, 15 - mapView.map.zoomLevel)))
    }

    Connections {
        target: page.service
        function onCityChanged() {
            page.selected = null
            mapView.map.center = page.service.cityCenter
        }
    }

    Plugin {
        id: osmPlugin
        name: "osm"
        // OSM tile usage policy: identify the app, use the standard tile server.
        PluginParameter { name: "osm.useragent"; value: "ScootNorge/0.1 (personal Qt app)" }
        PluginParameter { name: "osm.mapping.custom.host"; value: "https://tile.openstreetmap.org/" }
        PluginParameter { name: "osm.mapping.providersrepository.disabled"; value: true }
    }

    MapView {
        id: mapView
        anchors.fill: parent
        map.plugin: osmPlugin
        map.zoomLevel: 15
        Component.onCompleted: {
            map.center = page.service.cityCenter
            page.applyMapType()
            page.updateFocus()
            map.addMapItemView(zoneView)
            map.addMapItemView(vehicleView)
            map.addMapItem(userMarker)
        }
    }

    Connections {
        target: mapView.map
        function onCenterChanged() { focusTimer.restart() }
        function onZoomLevelChanged() { focusTimer.restart() }
        function onSupportedMapTypesChanged() { page.applyMapType() }
    }
    Timer {
        id: focusTimer
        interval: 150
        onTriggered: page.updateFocus()
    }

    // ---- Restricted zones (no ride / no parking / slow) -----------------------
    MapItemView {
        id: zoneView
        model: page.service.zones
        delegate: MapPolygon {
            required property var polygon
            required property color fillColor
            required property color borderColor
            path: polygon
            color: fillColor
            border.color: borderColor
            border.width: 1
            z: 1
        }
    }

    // ---- Scooters ------------------------------------------------------------
    MapItemView {
        id: vehicleView
        model: page.service.vehicles
        delegate: MapQuickItem {
            id: marker
            required property int index
            required property string vehicleId
            required property var geo
            required property color operatorColor
            required property int battery
            required property bool cheapest
            readonly property bool isSelected: page.selected !== null
                                               && page.selected.vehicleId === vehicleId

            coordinate: geo
            anchorPoint.x: dot.width / 2
            anchorPoint.y: dot.height / 2
            z: isSelected ? 4 : (cheapest ? 3 : 2)

            sourceItem: Rectangle {
                id: dot
                width: marker.isSelected ? 40 : (mapView.map.zoomLevel >= 15.5 ? 30 : 18)
                height: width
                radius: width / 2
                color: marker.operatorColor
                border.width: marker.cheapest ? 3 : 2
                border.color: marker.cheapest ? page.ui.best : "white"

                Behavior on width { NumberAnimation { duration: 120 } }

                Text {
                    anchors.centerIn: parent
                    visible: dot.width >= 30 && marker.battery >= 0
                    text: marker.battery
                    color: "white"
                    font.pixelSize: 11
                    font.bold: true
                }
                TapHandler {
                    onTapped: page.selected = page.service.vehicles.get(marker.index)
                }
            }
        }
    }

    // ---- You are here ---------------------------------------------------------
    MapQuickItem {
        id: userMarker
        visible: page.hasUser
        coordinate: page.hasUser ? page.userPosition.coordinate : QtPositioning.coordinate()
        anchorPoint.x: 11
        anchorPoint.y: 11
        z: 5
        sourceItem: Rectangle {
            width: 22
            height: 22
            radius: 11
            color: "#2F80ED"
            border.color: "white"
            border.width: 3
        }
    }

    // ---- Operator filter chips ----------------------------------------------
    Flow {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 12
        spacing: 8

        Repeater {
            model: page.service.operators
            delegate: Button {
                required property var modelData
                text: modelData.name
                checked: page.isEnabled(modelData.key)
                StyleVariation.variations: [modelData.key + "Chip"]
                onClicked: page.setEnabled(modelData.key, !checked)
            }
        }
    }

    // ---- Floating actions -----------------------------------------------------
    ColumnLayout {
        anchors.right: parent.right
        anchors.rightMargin: 12
        anchors.bottom: card.visible ? card.top : legend.top
        anchors.bottomMargin: 12
        spacing: 10

        Button {
            text: "★"
            font.pixelSize: 20
            Accessible.name: qsTr("Cheapest scooter nearby")
            StyleVariation.variations: ["fab", "fabNeutral"]
            onClicked: page.showCheapest()
        }
        Button {
            text: "◎"
            font.pixelSize: 22
            enabled: page.hasUser
            StyleVariation.variations: ["fab"]
            onClicked: page.centerOnUser()
        }
    }

    ZoneLegend {
        id: legend
        anchors.left: parent.left
        anchors.bottom: card.visible ? card.top : parent.bottom
        anchors.margins: 12
        visible: page.service.zones.count > 0
        zones: page.service.zones
    }

    VehicleCard {
        id: card
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 12
        visible: page.selected !== null
        vehicle: page.selected
        service: page.service
        tripMinutes: page.settings.tripMinutes
        onCloseRequested: page.selected = null
    }
}
