import QtQuick
import Qt.labs.StyleKit

// Solid operator-branded controls: "Open in Voi" buttons, operator switches, etc.
// Instantiated once per operator inside each Theme (theme variations take
// precedence over the theme's own colours, see "StyleKit Property Resolution").
StyleVariation {
    property color brand: "gray"

    button {
        background.color: brand
        text.color: "white"
        hovered.background.color: Qt.darker(brand, 1.08)
        pressed.background.color: Qt.darker(brand, 1.18)
        checked.background.color: brand
        checked.text.color: "white"
    }
    switchControl {
        indicator.foreground.color: brand
    }
    checkBox {
        indicator.foreground.color: brand
    }
    slider {
        indicator.foreground.color: brand
        handle.border.color: brand
    }
}
