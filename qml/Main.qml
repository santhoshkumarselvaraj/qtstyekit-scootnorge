import QtQuick
import QtQuick.Layouts
import QtCore
import QtPositioning
import Qt.labs.StyleKit
import ScootNorge

ApplicationWindow {
    id: app
    width: 420
    height: 860
    visible: true
    title: qsTr("ScootNorge")

    // StyleKit: one Style object styles every control in the app.
    StyleKit.style: scootStyle
    ScootStyle {
        id: scootStyle
        themeName: settings.theme
    }

    readonly property bool darkMode: scootStyle.themeName === "Dark"
        || (scootStyle.themeName === "System" && Qt.styleHints.colorScheme === Qt.ColorScheme.Dark)

    // Colours for the few non-control items we draw ourselves (map markers, badges).
    readonly property QtObject ui: QtObject {
        readonly property color accent: scootStyle.accent
        readonly property color surface: app.darkMode ? "#1A1F26" : "#FFFFFF"
        readonly property color ink: app.darkMode ? "#F2F4F7" : "#101828"
        readonly property color subtle: app.darkMode ? "#98A2B3" : "#667085"
        readonly property color divider: app.darkMode ? "#2A313C" : "#EAECF0"
        readonly property color best: "#FFC53D"
    }

    Settings {
        id: settings
        property string city: "oslo"
        property string theme: "System"
        property var enabledOperators: ["voi", "bolt", "ryde"]
        property real minRangeKm: 0
        property int tripMinutes: 10
        property bool showNoRide: true
        property bool showNoParking: true
        property bool showSlow: false
    }

    MobilityService {
        id: service
        city: settings.city
    }

    // Filters -> models
    Binding { target: service.vehicles; property: "enabledOperators"; value: settings.enabledOperators }
    Binding { target: service.vehicles; property: "minRangeKm"; value: settings.minRangeKm }
    Binding { target: service.vehicles; property: "tripMinutes"; value: settings.tripMinutes }
    Binding { target: service.zones; property: "enabledOperators"; value: settings.enabledOperators }
    Binding { target: service.zones; property: "showNoRide"; value: settings.showNoRide }
    Binding { target: service.zones; property: "showNoParking"; value: settings.showNoParking }
    Binding { target: service.zones; property: "showSlow"; value: settings.showSlow }

    // Location (Android asks the user at first start)
    LocationPermission {
        id: locationPermission
        accuracy: LocationPermission.Precise
        availability: LocationPermission.WhenInUse
    }
    PositionSource {
        id: positionSource
        active: locationPermission.status === Qt.PermissionStatus.Granted
        updateInterval: 5000
    }
    Component.onCompleted: {
        if (locationPermission.status === Qt.PermissionStatus.Undetermined)
            locationPermission.request()
    }

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            spacing: 6

            ComboBox {
                id: cityBox
                Layout.preferredWidth: 150
                model: service.cities
                textRole: "name"
                valueRole: "key"
                Component.onCompleted: currentIndex = Math.max(0, indexOfValue(settings.city))
                onActivated: settings.city = currentValue
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0
                Label {
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    font.bold: true
                    text: service.loading && service.vehicles.totalCount === 0
                          ? qsTr("Loading scooters…")
                          : qsTr("%1 scooters").arg(service.vehicles.totalCount)
                }
                Label {
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    font.pixelSize: 11
                    StyleVariation.variations: ["caption"]
                    text: service.error.length > 0
                          ? service.error
                          : (isNaN(service.lastUpdated) ? "" : qsTr("Updated %1").arg(
                                 service.lastUpdated.toLocaleTimeString(Qt.locale(), Locale.ShortFormat)))
                }
            }

            ToolButton {
                text: "↻"
                font.pixelSize: 20
                enabled: !service.loading
                onClicked: service.refresh()
            }
            ToolButton {
                text: "☰"
                font.pixelSize: 18
                onClicked: filterSheet.open()
            }
        }
    }

    ProgressBar {
        z: 10
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        indeterminate: true
        visible: service.loading
    }

    StackLayout {
        anchors.fill: parent
        currentIndex: tabBar.currentIndex

        MapPage {
            id: mapPage
            service: service
            settings: settings
            ui: app.ui
            userPosition: positionSource.position
            onOpenFilters: filterSheet.open()
        }
        NearbyPage {
            service: service
            ui: app.ui
            onVehicleChosen: vehicle => {
                mapPage.showVehicle(vehicle)
                tabBar.currentIndex = 0
            }
        }
        ComparePage {
            service: service
            ui: app.ui
            tripMinutes: settings.tripMinutes
            onTripMinutesEdited: minutes => settings.tripMinutes = minutes
        }
    }

    footer: TabBar {
        id: tabBar
        TabButton { text: qsTr("Map") }
        TabButton { text: qsTr("Nearby") }
        TabButton { text: qsTr("Compare") }
    }

    FilterSheet {
        id: filterSheet
        service: service
        settings: settings
        styleObject: scootStyle
    }
}
