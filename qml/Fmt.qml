pragma Singleton
import QtQuick

// Small formatting helpers shared by all pages.
QtObject {
    function distance(m) {
        if (m === undefined || m === null || m < 0)
            return "–"
        return m < 1000 ? Math.round(m) + " m" : (m / 1000).toFixed(1) + " km"
    }

    function money(v, currency) {
        if (v === undefined || v === null || v < 0)
            return qsTr("n/a")
        const unit = (!currency || currency === "NOK") ? "kr" : currency
        const rounded = Math.round(v * 10) / 10
        return rounded.toLocaleString(Qt.locale(), "f", rounded % 1 === 0 ? 0 : 1) + " " + unit
    }

    function range(km) {
        return km === undefined || km < 0 ? qsTr("range n/a") : qsTr("%1 km range").arg(km.toFixed(km < 10 ? 1 : 0))
    }

    function batteryVariations(b) {
        if (b === undefined || b < 0)
            return []
        return b < 25 ? ["batteryLow"] : (b < 55 ? ["batteryMid"] : ["batteryHigh"])
    }

    function walkMinutes(m) {
        return m === undefined || m < 0 ? "" : qsTr("%1 min walk").arg(Math.max(1, Math.round(m / 80)))
    }
}
