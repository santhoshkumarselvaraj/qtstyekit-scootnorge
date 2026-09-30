#include "zonemodel.h"
#include "operators.h"

ZoneModel::ZoneModel(QObject *parent)
    : QAbstractListModel(parent)
{
    for (const OperatorInfo &op : knownOperators())
        m_enabled.append(op.key);
    m_rebuildTimer.setSingleShot(true);
    m_rebuildTimer.setInterval(250);
    connect(&m_rebuildTimer, &QTimer::timeout, this, &ZoneModel::rebuild);
}

int ZoneModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_visible.size());
}

QHash<int, QByteArray> ZoneModel::roleNames() const
{
    return {
        { PolygonRole, "polygon" },
        { KindRole, "kind" },
        { NameRole, "zoneName" },
        { MaxSpeedRole, "maxSpeed" },
        { OperatorKeyRole, "operatorKey" },
        { FillColorRole, "fillColor" },
        { BorderColorRole, "borderColor" },
    };
}

QColor ZoneModel::fillColorFor(int kind) const
{
    switch (kind) {
    case NoRide: return QColor(229, 57, 53, 70);
    case NoParking: return QColor(251, 140, 0, 60);
    default: return QColor(253, 216, 53, 55);
    }
}

QColor ZoneModel::borderColorFor(int kind) const
{
    switch (kind) {
    case NoRide: return QColor(229, 57, 53, 200);
    case NoParking: return QColor(251, 140, 0, 200);
    default: return QColor(200, 160, 0, 200);
    }
}

QVariant ZoneModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_visible.size())
        return {};
    const ZoneRecord &r = m_all.at(m_visible.at(index.row()));
    switch (role) {
    case PolygonRole: return r.zone.path;
    case KindRole: return r.zone.kind;
    case NameRole: return r.zone.name;
    case MaxSpeedRole: return r.zone.maxSpeedKph;
    case OperatorKeyRole: return knownOperators().at(r.op).key;
    case FillColorRole: return fillColorFor(r.zone.kind);
    case BorderColorRole: return borderColorFor(r.zone.kind);
    }
    return {};
}

void ZoneModel::setFocus(const QGeoCoordinate &c)
{
    if (c == m_focus)
        return;
    m_focus = c;
    emit focusChanged();
    scheduleRebuild();
}

void ZoneModel::setRadiusMeters(double r)
{
    if (qFuzzyCompare(r, m_radius) || r <= 0)
        return;
    m_radius = r;
    emit radiusMetersChanged();
    scheduleRebuild();
}

void ZoneModel::setEnabledOperators(const QStringList &ops)
{
    if (ops == m_enabled)
        return;
    m_enabled = ops;
    emit enabledOperatorsChanged();
    scheduleRebuild();
}

void ZoneModel::setShowNoRide(bool on)
{
    if (on == m_showNoRide)
        return;
    m_showNoRide = on;
    emit filterChanged();
    scheduleRebuild();
}

void ZoneModel::setShowNoParking(bool on)
{
    if (on == m_showNoParking)
        return;
    m_showNoParking = on;
    emit filterChanged();
    scheduleRebuild();
}

void ZoneModel::setShowSlow(bool on)
{
    if (on == m_showSlow)
        return;
    m_showSlow = on;
    emit filterChanged();
    scheduleRebuild();
}

void ZoneModel::setRecords(QList<ZoneRecord> records)
{
    beginResetModel();
    m_all = std::move(records);
    m_visible.clear();
    endResetModel();
    rebuild();
}

void ZoneModel::scheduleRebuild()
{
    m_rebuildTimer.start();
}

void ZoneModel::rebuild()
{
    m_rebuildTimer.stop();
    const auto &ops = knownOperators();

    QList<int> visible;
    for (int i = 0; i < m_all.size() && visible.size() < m_maxZones; ++i) {
        const ZoneRecord &r = m_all.at(i);
        if (r.op < 0 || !m_enabled.contains(ops.at(r.op).key))
            continue;
        const int kind = r.zone.kind;
        if ((kind == NoRide && !m_showNoRide) || (kind == NoParking && !m_showNoParking)
            || (kind == Slow && !m_showSlow))
            continue;
        if (m_focus.isValid() && m_focus.distanceTo(r.zone.center) > m_radius + r.zone.radiusMeters)
            continue;
        visible.append(i);
    }

    // Skip the (expensive) polygon re-creation if nothing changed.
    if (visible == m_visible)
        return;

    beginResetModel();
    m_visible = std::move(visible);
    endResetModel();
    emit countChanged();
}
