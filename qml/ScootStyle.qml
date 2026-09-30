import QtQuick
import Qt.labs.StyleKit

// The whole look of ScootNorge lives here. Geometry is shared in the Style,
// colours live in the light/dark Themes, and operator branding is done with
// StyleVariations that screens opt into, e.g.
//     StyleVariation.variations: ["voi"]          -> solid Voi button / switch
//     StyleVariation.variations: ["boltChip"]     -> map filter chip
//     StyleVariation.variations: ["batteryLow"]   -> red battery bar
Style {
    id: style

    readonly property color accent: "#0FA38F"
    readonly property color voiColor: "#F26961"
    readonly property color boltColor: "#34D186"
    readonly property color rydeColor: "#3D8BFD"
    readonly property color lowColor: "#E53935"
    readonly property color midColor: "#FB8C00"
    readonly property color highColor: "#43A047"

    // ---------------------------------------------------------------- geometry
    control {
        padding: 8
        background {
            radius: 12
            height: 40
            border.width: 0
        }
        indicator.radius: 6
        handle.radius: 255
    }

    abstractButton {
        background {
            width: 44
            height: 42
            radius: 21
        }
        pressed.background.scale: 0.96
    }

    button {
        padding: 10
        background.shadow {
            opacity: 0.22
            verticalOffset: 2
            horizontalOffset: 0
        }
    }

    flatButton {
        background.visible: false
        background.shadow.opacity: 0
        hovered.background.visible: true
        pressed.background.visible: true
    }

    toolButton {
        background {
            visible: false
            radius: 20
            width: 40
            height: 40
            shadow.opacity: 0
        }
        hovered.background.visible: true
        pressed.background.visible: true
    }

    switchControl {
        background.visible: false
        indicator {
            width: 44
            height: 26
            radius: 13
            border.width: 0
            foreground {
                margins: 2
                radius: 11
                visible: false
            }
        }
        checked.indicator.foreground.visible: true
        handle {
            leftMargin: 3
            rightMargin: 3
            width: 20
            height: 20
            radius: 255
            color: "white"
            shadow {
                opacity: 0.3
                verticalOffset: 1
                horizontalOffset: 0
            }
        }
    }

    checkBox {
        background.visible: false
        indicator {
            width: 22
            height: 22
            radius: 6
            border.width: 2
            foreground {
                margins: 4
                radius: 3
                visible: false
            }
        }
        checked.indicator.foreground.visible: true
    }

    slider {
        background.visible: false
        indicator {
            fillWidth: true
            height: 6
            radius: 3
            foreground.radius: 3
        }
        handle {
            width: 24
            height: 24
            radius: 255
            color: "white"
            border.width: 2
        }
    }

    progressBar {
        background.visible: false
        indicator {
            width: 120
            height: 8
            radius: 4
            foreground.radius: 4
        }
    }

    itemDelegate {
        padding: 12
        text.alignment: Qt.AlignVCenter | Qt.AlignLeft
        background {
            radius: 0
            border.width: 0
        }
    }

    toolBar {
        padding: 6
        background {
            radius: 0
            height: 56
            border.width: 0
        }
    }

    tabBar {
        padding: 0
        background {
            radius: 0
            border.width: 0
        }
    }

    tabButton {
        background {
            radius: 0
            height: 52
            shadow.opacity: 0
        }
        pressed.background.scale: 1.0
    }

    frame {
        padding: 14
        background {
            radius: 18
            border.width: 1
            shadow {
                opacity: 0.16
                verticalOffset: 4
                horizontalOffset: 0
            }
        }
    }

    popup {
        padding: 18
        background {
            radius: 24
            border.width: 0
        }
    }

    label {
        background.visible: false
    }

    // Round floating action buttons on the map.
    StyleVariation {
        name: "fab"
        button {
            padding: 0
            background {
                width: 48
                height: 48
                radius: 24
            }
        }
    }

    // ------------------------------------------------------------------ themes
    light: Theme {
        applicationWindow.background.color: "#F3F5F7"

        control {
            text.color: "#101828"
            background.color: "#FFFFFF"
            background.border.color: "#D0D5DD"
            hovered.background.color: "#EEF2F6"
            indicator.color: "#D0D5DD"
            indicator.border.color: "#98A2B3"
            indicator.foreground.color: style.accent
            handle.border.color: style.accent
        }
        button {
            background.color: style.accent
            background.shadow.color: "#101828"
            text.color: "white"
            hovered.background.color: Qt.darker(style.accent, 1.08)
            pressed.background.color: Qt.darker(style.accent, 1.18)
        }
        flatButton {
            text.color: style.accent
            hovered.background.color: Qt.alpha(style.accent, 0.10)
            pressed.background.color: Qt.alpha(style.accent, 0.18)
        }
        toolButton {
            text.color: "#101828"
            hovered.background.color: "#EEF2F6"
            pressed.background.color: "#E4E7EC"
        }
        label.text.color: "#101828"
        toolBar.background.color: "#FFFFFF"
        tabBar.background.color: "#FFFFFF"
        tabButton {
            background.color: "#FFFFFF"
            text.color: "#667085"
            hovered.background.color: "#F9FAFB"
            checked.text.color: style.accent
            checked.background.color: "#E7F6F3"
        }
        frame {
            background.color: "#FFFFFF"
            background.border.color: "#EAECF0"
            background.shadow.color: "#101828"
        }
        popup.background.color: "#FFFFFF"
        itemDelegate {
            background.color: "#FFFFFF"
            text.color: "#101828"
            hovered.background.color: "#F2F4F7"
            pressed.background.color: "#EAECF0"
        }
        progressBar.indicator.color: "#EAECF0"

        StyleVariation { name: "caption"; label.text.color: "#667085" }
        StyleVariation { name: "best"; frame.background.border.color: style.accent; frame.background.border.width: 2 }
        OperatorVariation { name: "voi"; brand: style.voiColor }
        OperatorVariation { name: "bolt"; brand: style.boltColor }
        OperatorVariation { name: "ryde"; brand: style.rydeColor }
        OperatorChipVariation { name: "voiChip"; brand: style.voiColor; surface: "#FFFFFF"; ink: "#101828" }
        OperatorChipVariation { name: "boltChip"; brand: style.boltColor; surface: "#FFFFFF"; ink: "#101828" }
        OperatorChipVariation { name: "rydeChip"; brand: style.rydeColor; surface: "#FFFFFF"; ink: "#101828" }
        StyleVariation { name: "batteryLow"; progressBar.indicator.foreground.color: style.lowColor }
        StyleVariation { name: "batteryMid"; progressBar.indicator.foreground.color: style.midColor }
        StyleVariation { name: "batteryHigh"; progressBar.indicator.foreground.color: style.highColor }
        StyleVariation {
            name: "fabNeutral"
            button {
                background.color: "#FFFFFF"
                text.color: "#101828"
                hovered.background.color: "#F2F4F7"
            }
        }
    }

    dark: Theme {
        applicationWindow.background.color: "#0E1116"

        control {
            text.color: "#F2F4F7"
            background.color: "#1A1F26"
            background.border.color: "#344054"
            hovered.background.color: "#242A33"
            indicator.color: "#344054"
            indicator.border.color: "#667085"
            indicator.foreground.color: style.accent
            handle.border.color: style.accent
        }
        button {
            background.color: style.accent
            background.shadow.color: "black"
            text.color: "white"
            hovered.background.color: Qt.lighter(style.accent, 1.1)
            pressed.background.color: Qt.darker(style.accent, 1.1)
        }
        flatButton {
            text.color: Qt.lighter(style.accent, 1.3)
            hovered.background.color: Qt.alpha(style.accent, 0.18)
            pressed.background.color: Qt.alpha(style.accent, 0.28)
        }
        toolButton {
            text.color: "#F2F4F7"
            hovered.background.color: "#242A33"
            pressed.background.color: "#2E3540"
        }
        label.text.color: "#F2F4F7"
        toolBar.background.color: "#151A21"
        tabBar.background.color: "#151A21"
        tabButton {
            background.color: "#151A21"
            text.color: "#98A2B3"
            hovered.background.color: "#1C222B"
            checked.text.color: Qt.lighter(style.accent, 1.3)
            checked.background.color: "#10302B"
        }
        frame {
            background.color: "#1A1F26"
            background.border.color: "#2A313C"
            background.shadow.color: "black"
        }
        popup.background.color: "#1A1F26"
        itemDelegate {
            background.color: "#151A21"
            text.color: "#F2F4F7"
            hovered.background.color: "#1C222B"
            pressed.background.color: "#242A33"
        }
        progressBar.indicator.color: "#2A313C"

        StyleVariation { name: "caption"; label.text.color: "#98A2B3" }
        StyleVariation { name: "best"; frame.background.border.color: style.accent; frame.background.border.width: 2 }
        OperatorVariation { name: "voi"; brand: style.voiColor }
        OperatorVariation { name: "bolt"; brand: style.boltColor }
        OperatorVariation { name: "ryde"; brand: style.rydeColor }
        OperatorChipVariation { name: "voiChip"; brand: style.voiColor; surface: "#1A1F26"; ink: "#F2F4F7" }
        OperatorChipVariation { name: "boltChip"; brand: style.boltColor; surface: "#1A1F26"; ink: "#F2F4F7" }
        OperatorChipVariation { name: "rydeChip"; brand: style.rydeColor; surface: "#1A1F26"; ink: "#F2F4F7" }
        StyleVariation { name: "batteryLow"; progressBar.indicator.foreground.color: style.lowColor }
        StyleVariation { name: "batteryMid"; progressBar.indicator.foreground.color: style.midColor }
        StyleVariation { name: "batteryHigh"; progressBar.indicator.foreground.color: style.highColor }
        StyleVariation {
            name: "fabNeutral"
            button {
                background.color: "#1A1F26"
                text.color: "#F2F4F7"
                hovered.background.color: "#242A33"
            }
        }
    }
}
