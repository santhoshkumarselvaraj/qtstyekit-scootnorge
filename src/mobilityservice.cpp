#include "mobilityservice.h"
#include "operators.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>

#include <algorithm>

#ifndef ET_CLIENT_NAME
#define ET_CLIENT_NAME "personal-scootnorge"
#endif

namespace {

const QString kGbfsBase = QStringLiteral("https://api.entur.io/mobility/v2/gbfs/v3/");

struct City { const char *key; const char *name; double lat; double lon; };

const City kCities[] = {
    { "oslo",         "Oslo",         59.9139, 10.7522 },
    { "asker",        "Asker",        59.8333, 10.4372 },
    { "bergen",       "Bergen",       60.3913,  5.3221 },
    { "trondheim",    "Trondheim",    63.4305, 10.3951 },
    { "stavanger",    "Stavanger",    58.9690,  5.7331 },
    { "kristiansand", "Kristiansand", 58.1467,  7.9956 },
    { "drammen",      "Drammen",      59.7439, 10.2045 },
    { "fredrikstad",  "Fredrikstad",  59.2181, 10.9298 },
    { "tromso",       "Tromsø",       69.6492, 18.9553 },
};

} // namespace

MobilityService::MobilityService(QObject *parent)
    : QObject(parent)
    , m_vehicles(new VehicleModel(this))
    , m_zones(new ZoneModel(this))
{
    m_nam.setTransferTimeout(20000);

    m_refreshTimer.setInterval(60 * 1000);
    connect(&m_refreshTimer, &QTimer::timeout, this, &MobilityService::refresh);

    m_publishTimer.setSingleShot(true);
    m_publishTimer.setInterval(100);
    connect(&m_publishTimer, &QTimer::timeout, this, &MobilityService::publish);

    // Start after QML has applied its initial property values (city etc.).
    QTimer::singleShot(0, this, [this] {
        m_started = true;
        reload();
        m_refreshTimer.start();
    });
}

void MobilityService::setCity(const QString &city)
{
    const QString key = city.toLower();
    if (key == m_city)
        return;
    m_city = key;
    emit cityChanged();
    if (m_started)
        reload();
}

QVariantList MobilityService::cities() const
{
    QVariantList list;
    for (const City &c : kCities) {
        list.append(QVariantMap{
            { QStringLiteral("key"), QString::fromLatin1(c.key) },
            { QStringLiteral("name"), QString::fromUtf8(c.name) },
            { QStringLiteral("center"), QVariant::fromValue(QGeoCoordinate(c.lat, c.lon)) },
        });
    }
    return list;
}

QGeoCoordinate MobilityService::cityCenter() const
{
    for (const City &c : kCities)
        if (m_city == QLatin1String(c.key))
            return QGeoCoordinate(c.lat, c.lon);
    return QGeoCoordinate(kCities[0].lat, kCities[0].lon);
}

QVariantList MobilityService::operators() const
{
    QVariantList list;
    for (const OperatorInfo &op : knownOperators()) {
        list.append(QVariantMap{
            { QStringLiteral("key"), op.key },
            { QStringLiteral("name"), op.name },
            { QStringLiteral("color"), op.color },
        });
    }
    return list;
}

QStringList MobilityService::systems() const
{
    QStringList ids;
    for (const System &s : m_systemList)
        ids.append(s.id);
    return ids;
}

void MobilityService::setRefreshIntervalSec(int s)
{
    s = std::max(15, s); // be nice to Entur - GBFS feeds typically have ttl >= 15 s
    if (s == refreshIntervalSec())
        return;
    m_refreshTimer.setInterval(s * 1000);
    emit refreshIntervalSecChanged();
}

void MobilityService::setError(const QString &e)
{
    if (e == m_error)
        return;
    m_error = e;
    emit errorChanged();
}

void MobilityService::get(const QUrl &url, JsonHandler onOk)
{
    QNetworkRequest req(url);
    req.setRawHeader("ET-Client-Name", QByteArrayLiteral(ET_CLIENT_NAME));
    req.setHeader(QNetworkRequest::UserAgentHeader, QByteArrayLiteral("ScootNorge/0.1 (Qt)"));

    QNetworkReply *reply = m_nam.get(req);
    const int generation = m_generation;
    if (m_pending++ == 0)
        emit loadingChanged();

    connect(reply, &QNetworkReply::finished, this, [this, reply, generation, url, onOk] {
        reply->deleteLater();
        if (--m_pending == 0)
            emit loadingChanged();
        if (generation != m_generation)
            return; // city changed while the request was in flight
        if (reply->error() != QNetworkReply::NoError) {
            setError(tr("Could not load %1: %2").arg(url.fileName(), reply->errorString()));
            return;
        }
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);
        if (!doc.isObject()) {
            setError(tr("Unexpected data from %1").arg(url.toString()));
            return;
        }
        onOk(doc.object());
    });
}

void MobilityService::reload()
{
    ++m_generation;
    m_systemList.clear();
    emit systemsChanged();
    setError({});
    m_zonesDirty = true;
    publish(); // clears the map immediately

    get(QUrl(kGbfsBase + QStringLiteral("manifest.json")),
        [this](const QJsonObject &manifest) { discoverFromManifest(manifest); });
}

void MobilityService::discoverFromManifest(const QJsonObject &manifest)
{
    const QJsonArray datasets = manifest.value(QLatin1String("data")).toObject()
                                        .value(QLatin1String("datasets")).toArray();
    const auto &ops = knownOperators();

    for (int opIdx = 0; opIdx < ops.size(); ++opIdx) {
        const QString exact = ops[opIdx].prefix + m_city;
        bool found = false;
        for (const QJsonValue &dv : datasets) {
            const QJsonObject d = dv.toObject();
            const QString id = d.value(QLatin1String("system_id")).toString();
            if (id != exact && !(id.startsWith(ops[opIdx].prefix) && id.contains(m_city)))
                continue;
            QUrl url;
            const QJsonArray versions = d.value(QLatin1String("versions")).toArray();
            for (const QJsonValue &vv : versions) {
                const QJsonObject v = vv.toObject();
                url = QUrl(v.value(QLatin1String("url")).toString());
                if (v.value(QLatin1String("version")).toString().startsWith(QLatin1Char('3')))
                    break; // prefer GBFS 3.x
            }
            if (url.isValid()) {
                addSystem(id, opIdx, url);
                found = true;
            }
        }
        // Manifest missing or renamed? Try the conventional id directly.
        if (!found && datasets.isEmpty())
            addSystem(exact, opIdx, QUrl(kGbfsBase + exact + QStringLiteral("/gbfs")));
    }

    if (m_systemList.isEmpty())
        setError(tr("No Voi, Bolt or Ryde scooters are published for this city right now."));
}

void MobilityService::addSystem(const QString &systemId, int op, const QUrl &gbfsUrl)
{
    System s;
    s.id = systemId;
    s.op = op;
    m_systemList.append(s);
    emit systemsChanged();

    const int index = int(m_systemList.size()) - 1;
    get(gbfsUrl, [this, index](const QJsonObject &root) {
        m_systemList[index].feeds = Gbfs::parseFeeds(root);
        loadSystemFeeds(index);
    });
}

void MobilityService::loadSystemFeeds(int index)
{
    const System &s = m_systemList.at(index);

    if (const QUrl u = s.feeds.value(QStringLiteral("vehicle_types")); u.isValid()) {
        get(u, [this, index](const QJsonObject &root) {
            auto &types = m_systemList[index].types;
            types.clear();
            for (const Gbfs::VehicleType &t : Gbfs::parseVehicleTypes(root))
                types.insert(t.id, t);
            schedulePublish();
        });
    }
    if (const QUrl u = s.feeds.value(QStringLiteral("system_pricing_plans")); u.isValid()) {
        get(u, [this, index](const QJsonObject &root) {
            auto &plans = m_systemList[index].plans;
            plans.clear();
            for (const Gbfs::PricingPlan &p : Gbfs::parsePricingPlans(root))
                plans.insert(p.id, p);
            schedulePublish();
        });
    }
    if (const QUrl u = s.feeds.value(QStringLiteral("geofencing_zones")); u.isValid()) {
        get(u, [this, index](const QJsonObject &root) {
            m_systemList[index].zones = Gbfs::parseZones(root);
            m_zonesDirty = true;
            schedulePublish();
        });
    }
    fetchVehicles(index);
}

void MobilityService::fetchVehicles(int index)
{
    const System &s = m_systemList.at(index);
    QUrl u = s.feeds.value(QStringLiteral("vehicle_status"));      // GBFS 3.x
    if (!u.isValid())
        u = s.feeds.value(QStringLiteral("free_bike_status"));     // GBFS 2.x
    if (!u.isValid())
        return;
    get(u, [this, index](const QJsonObject &root) {
        m_systemList[index].vehicles = Gbfs::parseVehicles(root);
        m_lastUpdated = QDateTime::currentDateTime();
        emit lastUpdatedChanged();
        setError({});
        schedulePublish();
    });
}

void MobilityService::refresh()
{
    if (m_systemList.isEmpty()) {
        reload();
        return;
    }
    for (int i = 0; i < m_systemList.size(); ++i)
        fetchVehicles(i);
}

void MobilityService::schedulePublish()
{
    m_publishTimer.start();
}

void MobilityService::publish()
{
    m_publishTimer.stop();

    QList<VehicleRecord> records;
    for (const System &s : std::as_const(m_systemList)) {
        for (const Gbfs::Vehicle &v : s.vehicles) {
            if (v.reserved || v.disabled)
                continue;
            const auto typeIt = s.types.constFind(v.typeId);
            const Gbfs::VehicleType *type = typeIt != s.types.cend() ? &typeIt.value() : nullptr;
            if (type && !type->formFactor.isEmpty()
                && !type->formFactor.startsWith(QLatin1String("scooter")))
                continue; // e-bikes, mopeds, cars... - this app is about kick scooters

            VehicleRecord r;
            r.id = s.id + QLatin1Char(':') + v.id;
            r.op = s.op;
            r.coord = QGeoCoordinate(v.lat, v.lon);
            r.battery = v.fuelPercent >= 0 ? qRound(v.fuelPercent) : -1;
            if (v.rangeMeters >= 0)
                r.rangeKm = v.rangeMeters / 1000.0;
            else if (v.fuelPercent >= 0 && type && type->maxRangeMeters > 0)
                r.rangeKm = v.fuelPercent / 100.0 * type->maxRangeMeters / 1000.0;

            QString planId = v.planId;
            if (planId.isEmpty() && type)
                planId = type->defaultPlanId;
            if (const auto p = s.plans.constFind(planId); p != s.plans.cend())
                r.plan = p.value();
            else if (s.plans.size() == 1)
                r.plan = s.plans.cbegin().value();

            r.androidUri = v.androidUri;
            r.webUri = v.webUri;
            records.append(std::move(r));
        }
    }
    m_vehicles->setRecords(std::move(records));

    if (m_zonesDirty) {
        m_zonesDirty = false;
        QList<ZoneRecord> zones;
        for (const System &s : std::as_const(m_systemList))
            for (const Gbfs::Zone &z : s.zones)
                zones.append(ZoneRecord{ s.op, z });
        m_zones->setRecords(std::move(zones));
    }
}
