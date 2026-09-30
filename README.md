# ScootNorge

A Qt 6.12 / QML app that shows live **Voi, Bolt and Ryde** e-scooters on one map,
styled entirely with **Qt Labs StyleKit** (`import Qt.labs.StyleKit`).

Data comes from Entur's open Mobility API (GBFS 3.0, NLOD licence). No API key is
needed, only an `ET-Client-Name` header. Booking happens in the operator's own
app, which the app opens through a deep link.

## Features

| Feature | Where | Ruter app |
|---|---|---|
| Voi + Bolt + Ryde on one map, with per-operator filter chips | Map | Yes (Oslo/Akershus only) |
| 8 Norwegian cities (Oslo, Bergen, Trondheim, Stavanger, ...) | City picker | No |
| **Minimum-range filter** (hide scooters that can't make the trip) | Filters | No |
| Battery-coloured markers, battery bar and range per scooter | Map, Nearby | Partly |
| **Price comparison** per operator for *N* minutes within 250 m, 500 m or 1 km | Compare | No |
| "Cheapest nearby" highlight (gold ring) and ★ jump button | Map | No |
| **No-riding / no-parking / slow-zone overlays** with legend | Map | No |
| Walking directions to the chosen scooter | Vehicle card | No |
| Light / Dark / System theme (StyleKit themes) | Filters | Yes |
| In-app unlock and payment | Not possible (needs an operator agreement) | Yes |

## Project layout

```
CMakeLists.txt
src/
  gbfs.h/.cpp            GBFS parsers (vehicle_status, vehicle_types, pricing, geofencing)
  mobilityservice.*      Entur client: discovers systems per city, refreshes every 60 s
  vehiclemodel.*         Filtered, distance-sorted model + price comparison + deep links
  zonemodel.*            Restricted zones near the map centre
  operators.h            Voi/Bolt/Ryde metadata (colour, Play Store package)
qml/
  ScootStyle.qml         The StyleKit Style (geometry + light/dark Themes)
  OperatorVariation.qml  StyleVariation: solid operator-branded controls
  OperatorChipVariation.qml  StyleVariation: checkable map filter chips
  Main.qml, MapPage.qml, NearbyPage.qml, ComparePage.qml, FilterSheet.qml,
  VehicleCard.qml, ZoneLegend.qml, Fmt.qml
```

### How StyleKit is used

- `StyleKit.style: ScootStyle {}` on the `ApplicationWindow` styles every control.
  There is no `import QtQuick.Controls`: the controls come from `Qt.labs.StyleKit`.
- **Geometry** (radii, sizes, shadows) is set once in the `Style`. **Colours** live in
  the `light` and `dark` `Theme`s. `themeName` switches between System, Light and Dark at runtime.
- **Instance variations** handle the branding: `StyleVariation.variations: ["voi"]`
  gives a Voi-coloured button or switch, `["boltChip"]` gives a Bolt filter chip, and
  `["batteryLow"]` gives a red battery bar.
- Colour variations are defined **inside each Theme** on purpose. StyleKit resolves
  properties from the theme before the style, so a style-level variation could not
  override a theme colour.

## Build

Requirements: Qt **6.12+** with Qt Positioning and Qt Location, CMake 3.21+.

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=~/Qt/6.12/gcc_64 -DET_CLIENT_NAME="yourname-scootnorge"
cmake --build build
./build/appScootNorge
```

## Notes and limits

- Set your own `ET_CLIENT_NAME` (format `company-application`). Entur rate-limits anonymous clients.
- Map tiles come from `tile.openstreetmap.org`, which is fine for personal use under the
  [OSM tile policy](https://operations.osmfoundation.org/policies/tiles/). A published app should use its own tile provider.
- Prices are **list prices** from GBFS `system_pricing_plans`. Passes and discounts are not included.
- Only the outer ring of each geofencing polygon is drawn, and the map shows at most 400 zones.
- `Qt.labs` modules may change between Qt versions.
- Voi, Bolt and Ryde are trademarks of their owners. This app uses only their public open data.
