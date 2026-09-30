#include "gbfs.h"

#include <QtMath>
#include <algorithm>

namespace Gbfs {

double PricingPlan::estimate(double minutes) const
{
    double total = price;
    for (const PriceSegment &s : perMin) {
        if (minutes <= s.start)
            continue;
        const double stop = s.end >= 0 ? std::min(minutes, s.end) : minutes;
        const double span = stop - s.start;
        if (span <= 0)
            continue;
        const double interval = s.interval > 0 ? s.interval : 1;
        total += std::ceil(span / interval) * s.rate;
    }
    return total;
}

double PricingPlan::perMinuteRate() const
{
    if (perMin.isEmpty())
        return 0;
    const PriceSegment &s = perMin.first();
    return s.interval > 0 ? s.rate / s.interval : s.rate;
}

// GBFS v3 uses [{ "text": "...", "language": "en" }], v2.3 plain strings.
QString localized(const QJsonValue &v)
{
    if (v.isString())
        return v.toString();
    if (!v.isArray())
        return {};
    const QJsonArray arr = v.toArray();
    QString first;
    for (const QJsonValue &item : arr) {
        const QJsonObject o = item.toObject();
        const QString text = o.value(QLatin1String("text")).toString();
        if (first.isEmpty())
            first = text;
        if (o.value(QLatin1String("language")).toString().startsWith(QLatin1String("en")))
            return text;
    }
    return first;
}

double number(const QJsonValue &v, double fallback)
{
    if (v.isDouble())
        return v.toDouble();
    if (v.isString()) {
        bool ok = false;
        const double d = v.toString().toDouble(&ok);
        return ok ? d : fallback;
    }
    return fallback;
}

static QJsonObject dataOf(const QJsonObject &root)
{
    return root.value(QLatin1String("data")).toObject();
}

QHash<QString, QUrl> parseFeeds(const QJsonObject &root)
{
    QHash<QString, QUrl> feeds;
    const QJsonObject data = dataOf(root);
    QJsonArray list = data.value(QLatin1String("feeds")).toArray(); // v3
    if (list.isEmpty()) {
        // v2.3: data.<language>.feeds - prefer English, else take the first language.
        QJsonObject lang = data.value(QLatin1String("en")).toObject();
        if (lang.isEmpty() && !data.isEmpty())
            lang = data.begin().value().toObject();
        list = lang.value(QLatin1String("feeds")).toArray();
    }
    for (const QJsonValue &f : list) {
        const QJsonObject o = f.toObject();
        feeds.insert(o.value(QLatin1String("name")).toString(),
                     QUrl(o.value(QLatin1String("url")).toString()));
    }
    return feeds;
}

QList<VehicleType> parseVehicleTypes(const QJsonObject &root)
{
    QList<VehicleType> out;
    const QJsonArray arr = dataOf(root).value(QLatin1String("vehicle_types")).toArray();
    for (const QJsonValue &v : arr) {
        const QJsonObject o = v.toObject();
        VehicleType t;
        t.id = o.value(QLatin1String("vehicle_type_id")).toString();
        t.formFactor = o.value(QLatin1String("form_factor")).toString();
        t.maxRangeMeters = number(o.value(QLatin1String("max_range_meters")));
        t.defaultPlanId = o.value(QLatin1String("default_pricing_plan_id")).toString();
        out.append(t);
    }
    return out;
}

QList<PricingPlan> parsePricingPlans(const QJsonObject &root)
{
    QList<PricingPlan> out;
    const QJsonArray arr = dataOf(root).value(QLatin1String("plans")).toArray();
    for (const QJsonValue &v : arr) {
        const QJsonObject o = v.toObject();
        PricingPlan p;
        p.id = o.value(QLatin1String("plan_id")).toString();
        p.currency = o.value(QLatin1String("currency")).toString(QStringLiteral("NOK"));
        p.price = number(o.value(QLatin1String("price")), 0);
        const QJsonArray segs = o.value(QLatin1String("per_min_pricing")).toArray();
        for (const QJsonValue &sv : segs) {
            const QJsonObject so = sv.toObject();
            PriceSegment s;
            s.start = number(so.value(QLatin1String("start")), 0);
            s.rate = number(so.value(QLatin1String("rate")), 0);
            s.interval = number(so.value(QLatin1String("interval")), 1);
            s.end = number(so.value(QLatin1String("end")), -1);
            p.perMin.append(s);
        }
        p.valid = true;
        out.append(p);
    }
    return out;
}

QList<Vehicle> parseVehicles(const QJsonObject &root)
{
    QList<Vehicle> out;
    const QJsonObject data = dataOf(root);
    QJsonArray arr = data.value(QLatin1String("vehicles")).toArray(); // v3 vehicle_status
    if (arr.isEmpty())
        arr = data.value(QLatin1String("bikes")).toArray();          // v2.3 free_bike_status
    out.reserve(arr.size());
    for (const QJsonValue &v : arr) {
        const QJsonObject o = v.toObject();
        Vehicle x;
        x.id = o.value(QLatin1String("vehicle_id")).toString();
        if (x.id.isEmpty())
            x.id = o.value(QLatin1String("bike_id")).toString();
        x.lat = number(o.value(QLatin1String("lat")), 0);
        x.lon = number(o.value(QLatin1String("lon")), 0);
        x.reserved = o.value(QLatin1String("is_reserved")).toBool();
        x.disabled = o.value(QLatin1String("is_disabled")).toBool();
        x.rangeMeters = number(o.value(QLatin1String("current_range_meters")));
        const double fuel = number(o.value(QLatin1String("current_fuel_percent")));
        x.fuelPercent = fuel < 0 ? -1 : (fuel <= 1.0 ? fuel * 100.0 : fuel);
        x.typeId = o.value(QLatin1String("vehicle_type_id")).toString();
        x.planId = o.value(QLatin1String("pricing_plan_id")).toString();
        const QJsonObject uris = o.value(QLatin1String("rental_uris")).toObject();
        x.androidUri = QUrl(uris.value(QLatin1String("android")).toString());
        x.webUri = QUrl(uris.value(QLatin1String("web")).toString());
        if (!x.id.isEmpty() && (x.lat != 0 || x.lon != 0))
            out.append(x);
    }
    return out;
}

static void appendPolygon(const QJsonArray &polygon, const Zone &proto, QList<Zone> &out)
{
    // polygon = [ outerRing, hole1, ... ]; ring = [[lon, lat], ...]
    const QJsonArray ring = polygon.at(0).toArray();
    if (ring.size() < 3)
        return;
    Zone z = proto;
    double sumLat = 0, sumLon = 0;
    QList<QGeoCoordinate> coords;
    coords.reserve(ring.size());
    for (const QJsonValue &p : ring) {
        const QJsonArray ll = p.toArray();
        const QGeoCoordinate c(ll.at(1).toDouble(), ll.at(0).toDouble());
        coords.append(c);
        sumLat += c.latitude();
        sumLon += c.longitude();
    }
    z.center = QGeoCoordinate(sumLat / coords.size(), sumLon / coords.size());
    for (const QGeoCoordinate &c : std::as_const(coords)) {
        z.radiusMeters = std::max(z.radiusMeters, z.center.distanceTo(c));
        z.path.append(QVariant::fromValue(c));
    }
    out.append(z);
}

QList<Zone> parseZones(const QJsonObject &root)
{
    QList<Zone> out;
    const QJsonObject fc = dataOf(root).value(QLatin1String("geofencing_zones")).toObject();
    const QJsonArray features = fc.value(QLatin1String("features")).toArray();
    for (const QJsonValue &fv : features) {
        const QJsonObject f = fv.toObject();
        const QJsonObject props = f.value(QLatin1String("properties")).toObject();

        bool noRide = false, noParking = false;
        double maxSpeed = -1;
        const QJsonArray rules = props.value(QLatin1String("rules")).toArray();
        for (const QJsonValue &rv : rules) {
            const QJsonObject r = rv.toObject();
            if (r.contains(QLatin1String("ride_through_allowed"))
                && !r.value(QLatin1String("ride_through_allowed")).toBool(true))
                noRide = true;
            // v3: ride_end_allowed, v2.3: ride_allowed
            const QJsonValue end = r.contains(QLatin1String("ride_end_allowed"))
                    ? r.value(QLatin1String("ride_end_allowed"))
                    : r.value(QLatin1String("ride_allowed"));
            if (end.isBool() && !end.toBool())
                noParking = true;
            const double speed = number(r.value(QLatin1String("maximum_speed_kph")));
            if (speed >= 0)
                maxSpeed = maxSpeed < 0 ? speed : std::min(maxSpeed, speed);
        }
        if (!noRide && !noParking && maxSpeed < 0)
            continue; // plain operating area - nothing to warn about

        Zone proto;
        proto.kind = noRide ? NoRide : (noParking ? NoParking : SlowZone);
        proto.name = localized(props.value(QLatin1String("name")));
        proto.maxSpeedKph = maxSpeed;

        const QJsonObject geom = f.value(QLatin1String("geometry")).toObject();
        const QString type = geom.value(QLatin1String("type")).toString();
        const QJsonArray coords = geom.value(QLatin1String("coordinates")).toArray();
        if (type == QLatin1String("MultiPolygon")) {
            for (const QJsonValue &poly : coords)
                appendPolygon(poly.toArray(), proto, out);
        } else if (type == QLatin1String("Polygon")) {
            appendPolygon(coords, proto, out);
        }
    }
    return out;
}

} // namespace Gbfs
