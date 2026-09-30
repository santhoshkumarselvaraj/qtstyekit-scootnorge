#include "vehiclemodel.h"
#include "operators.h"

#include <QDesktopServices>
#include <QUrlQuery>
#include <algorithm>

VehicleModel::VehicleModel(QObject *parent)
    : QAbstractListModel(parent)
{
    for (const OperatorInfo &op : knownOperators())
        m_enabled.append(op.key);

    // Map panning changes `focus` many times per second - coalesce rebuilds.
    m_rebuildTimer.setSingleShot(true);
    m_rebuildTimer.setInterval(120);
    connect(&m_rebuildTimer, &QTimer::timeout, this, &VehicleModel::rebuild);
}

int VehicleModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_rows.size());
}

QHash<int, QByteArray> VehicleModel::roleNames() const
{
    return {
        { VehicleIdRole, "vehicleId" },
        { OperatorKeyRole, "operatorKey" },
        { OperatorNameRole, "operatorName" },
        { OperatorColorRole, "operatorColor" },
        { GeoRole, "geo" },
        { BatteryRole, "battery" },
        { RangeKmRole, "rangeKm" },
        { DistanceRole, "distance" },
        { UnlockFeeRole, "unlockFee" },
        { PerMinuteRole, "perMinute" },
        { EstimateRole, "estimate" },
        { CurrencyRole, "currency" },
        { CheapestRole, "cheapest" },
    };
}

QVariant VehicleModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_rows.size())
        return {};
    const Row &row = m_rows.at(index.row());
    const VehicleRecord &r = m_all.at(row.record);
    const OperatorInfo &op = knownOperators().at(r.op);

    switch (role) {
    case VehicleIdRole: return r.id;
    case OperatorKeyRole: return op.key;
    case OperatorNameRole: return op.name;
    case OperatorColorRole: return op.color;
    case GeoRole: return QVariant::fromValue(r.coord);
    case BatteryRole: return r.battery;
    case RangeKmRole: return r.rangeKm;
    case DistanceRole: return row.distance;
    case UnlockFeeRole: return r.plan.valid ? r.plan.price : -1.0;
    case PerMinuteRole: return r.plan.valid ? r.plan.perMinuteRate() : -1.0;
    case EstimateRole: return row.estimate;
    case CurrencyRole: return r.plan.currency;
    case CheapestRole:
        return m_cheapest >= 0 && row.estimate >= 0 && row.estimate <= m_cheapest + 0.005
                && row.distance <= m_compareRadius;
    }
    return {};
}

void VehicleModel::setFocus(const QGeoCoordinate &c)
{
    if (c == m_focus)
        return;
    m_focus = c;
    emit focusChanged();
    scheduleRebuild();
}

void VehicleModel::setEnabledOperators(const QStringList &ops)
{
    if (ops == m_enabled)
        return;
    m_enabled = ops;
    emit enabledOperatorsChanged();
    scheduleRebuild();
}

void VehicleModel::setMinRangeKm(double km)
{
    if (qFuzzyCompare(km + 1, m_minRangeKm + 1))
        return;
    m_minRangeKm = km;
    emit minRangeKmChanged();
    scheduleRebuild();
}

void VehicleModel::setTripMinutes(int m)
{
    if (m == m_tripMinutes || m <= 0)
        return;
    m_tripMinutes = m;
    emit tripMinutesChanged();
    scheduleRebuild();
}

void VehicleModel::setMaxResults(int n)
{
    if (n == m_maxResults || n <= 0)
        return;
    m_maxResults = n;
    emit maxResultsChanged();
    scheduleRebuild();
}

void VehicleModel::setCompareRadiusMeters(double m)
{
    if (qFuzzyCompare(m, m_compareRadius) || m <= 0)
        return;
    m_compareRadius = m;
    emit compareRadiusMetersChanged();
    scheduleRebuild();
}

void VehicleModel::setRecords(QList<VehicleRecord> records)
{
    m_pending = std::move(records);
    m_hasPending = true;
    rebuild(); // new data: update right away (single model reset)
}

void VehicleModel::scheduleRebuild()
{
    m_rebuildTimer.start();
}

void VehicleModel::rebuild()
{
    m_rebuildTimer.stop();
    const auto &ops = knownOperators();

    QList<bool> enabled(ops.size(), false);
    for (int i = 0; i < ops.size(); ++i)
        enabled[i] = m_enabled.contains(ops[i].key);

    // Rows are computed against the incoming record list (if any) and swapped
    // in together with it inside one model reset.
    const QList<VehicleRecord> &src = m_hasPending ? m_pending : m_all;

    QList<Row> rows;
    rows.reserve(src.size());
    for (int i = 0; i < src.size(); ++i) {
        const VehicleRecord &r = src.at(i);
        if (r.op < 0 || !enabled[r.op])
            continue;
        if (m_minRangeKm > 0 && r.rangeKm >= 0 && r.rangeKm < m_minRangeKm)
            continue;
        const double dist = m_focus.isValid() ? m_focus.distanceTo(r.coord) : 0.0;
        const double est = r.plan.valid ? r.plan.estimate(m_tripMinutes) : -1.0;
        rows.append(Row{ i, dist, est });
    }
    std::sort(rows.begin(), rows.end(),
              [](const Row &a, const Row &b) { return a.distance < b.distance; });

    // --- Per-operator price comparison around the focus point -------------
    QVariantList comparison;
    double bestEstimate = -1;
    for (int opIdx = 0; opIdx < ops.size(); ++opIdx) {
        if (!enabled[opIdx])
            continue;
        const Row *nearest = nullptr;
        const Row *cheapest = nullptr;
        int nearby = 0;
        for (const Row &row : std::as_const(rows)) {
            if (src.at(row.record).op != opIdx)
                continue;
            if (!nearest)
                nearest = &row;
            if (row.distance > m_compareRadius)
                break; // rows are sorted by distance
            ++nearby;
            if (row.estimate >= 0 && (!cheapest || row.estimate < cheapest->estimate))
                cheapest = &row;
        }
        const Row *pick = cheapest ? cheapest : nearest;
        QVariantMap entry;
        entry[QStringLiteral("key")] = ops[opIdx].key;
        entry[QStringLiteral("name")] = ops[opIdx].name;
        entry[QStringLiteral("color")] = ops[opIdx].color;
        entry[QStringLiteral("nearbyCount")] = nearby;
        entry[QStringLiteral("available")] = pick != nullptr;
        if (pick) {
            const VehicleRecord &r = src.at(pick->record);
            entry[QStringLiteral("vehicleId")] = r.id;
            entry[QStringLiteral("distance")] = pick->distance;
            entry[QStringLiteral("nearestDistance")] = nearest->distance;
            entry[QStringLiteral("estimate")] = pick->estimate;
            entry[QStringLiteral("unlockFee")] = r.plan.valid ? r.plan.price : -1.0;
            entry[QStringLiteral("perMinute")] = r.plan.valid ? r.plan.perMinuteRate() : -1.0;
            entry[QStringLiteral("currency")] = r.plan.currency;
            entry[QStringLiteral("battery")] = r.battery;
            if (pick->estimate >= 0 && (bestEstimate < 0 || pick->estimate < bestEstimate))
                bestEstimate = pick->estimate;
        }
        comparison.append(entry);
    }
    for (QVariant &v : comparison) {
        QVariantMap m = v.toMap();
        const double est = m.value(QStringLiteral("estimate"), -1.0).toDouble();
        m[QStringLiteral("best")] = bestEstimate >= 0 && est >= 0 && est <= bestEstimate + 0.005;
        v = m;
    }
    std::sort(comparison.begin(), comparison.end(), [](const QVariant &a, const QVariant &b) {
        const QVariantMap ma = a.toMap(), mb = b.toMap();
        const bool aa = ma.value(QStringLiteral("available")).toBool();
        const bool ab = mb.value(QStringLiteral("available")).toBool();
        if (aa != ab)
            return aa;
        const double ea = ma.value(QStringLiteral("estimate"), -1.0).toDouble();
        const double eb = mb.value(QStringLiteral("estimate"), -1.0).toDouble();
        if ((ea < 0) != (eb < 0))
            return ea >= 0;
        return ea < eb;
    });

    const int filteredTotal = int(rows.size());
    if (rows.size() > m_maxResults)
        rows.resize(m_maxResults);

    beginResetModel();
    if (m_hasPending) {
        m_all = std::move(m_pending);
        m_pending.clear();
        m_hasPending = false;
    }
    m_rows = std::move(rows);
    m_cheapest = bestEstimate;
    m_filteredTotal = filteredTotal;
    endResetModel();

    m_comparison = comparison;
    emit countChanged();
    emit comparisonChanged();
}

const VehicleRecord *VehicleModel::find(const QString &id) const
{
    for (const VehicleRecord &r : m_all)
        if (r.id == id)
            return &r;
    return nullptr;
}

QVariantMap VehicleModel::get(int row) const
{
    QVariantMap m;
    if (row < 0 || row >= m_rows.size())
        return m;
    const QModelIndex idx = index(row);
    const auto roles = roleNames();
    for (auto it = roles.cbegin(); it != roles.cend(); ++it)
        m.insert(QString::fromUtf8(it.value()), data(idx, it.key()));
    return m;
}

bool VehicleModel::openInOperatorApp(const QString &vehicleId) const
{
    const VehicleRecord *r = find(vehicleId);
    if (!r)
        return false;
    const OperatorInfo &op = knownOperators().at(r->op);

#ifdef Q_OS_ANDROID
    if (r->androidUri.isValid() && !r->androidUri.isEmpty() && QDesktopServices::openUrl(r->androidUri))
        return true;
#endif
    if (r->webUri.isValid() && !r->webUri.isEmpty() && QDesktopServices::openUrl(r->webUri))
        return true;
#ifdef Q_OS_ANDROID
    if (QDesktopServices::openUrl(QUrl(QStringLiteral("market://details?id=") + op.androidPackage)))
        return true;
#endif
    return QDesktopServices::openUrl(
            QUrl(QStringLiteral("https://play.google.com/store/apps/details?id=") + op.androidPackage));
}

bool VehicleModel::openWalkingDirections(const QString &vehicleId) const
{
    const VehicleRecord *r = find(vehicleId);
    if (!r)
        return false;
    QUrl url(QStringLiteral("https://www.google.com/maps/dir/"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("api"), QStringLiteral("1"));
    q.addQueryItem(QStringLiteral("destination"),
                   QStringLiteral("%1,%2").arg(r->coord.latitude(), 0, 'f', 6)
                           .arg(r->coord.longitude(), 0, 'f', 6));
    q.addQueryItem(QStringLiteral("travelmode"), QStringLiteral("walking"));
    url.setQuery(q);
    return QDesktopServices::openUrl(url);
}
