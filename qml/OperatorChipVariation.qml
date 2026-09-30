import QtQuick
import Qt.labs.StyleKit

// Checkable filter "chips" on top of the map: neutral when off, branded when on.
StyleVariation {
    property color brand: "gray"
    property color surface: "white"
    property color ink: "black"

    button {
        padding: 8
        background {
            width: 72
            height: 34
            radius: 17
            color: surface
            border.width: 1
            border.color: Qt.alpha(ink, 0.15)
        }
        text.color: ink
        hovered.background.color: Qt.tint(surface, Qt.alpha(brand, 0.15))
        checked.background.color: brand
        checked.background.border.color: brand
        checked.text.color: "white"
    }
}
