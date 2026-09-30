#pragma once

#include "gbfs.h"

#include <QAbstractListModel>
#include <QGeoCoordinate>
#include <QStringList>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

struct ZoneRecord
{
    int op = -1;
    Gbfs::Zone zone;
};

// Restricted areas (no-ride, no-parking, slow) near `focus`, from GBFS geofencing_zones.
class ZoneModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by MobilityService")

    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QGeoCoordinate focus READ focus WRITE setFocus NOTIFY focusChanged)
    Q_PROPERTY(double radiusMeters READ radiusMeters WRITE setRadiusMeters NOTIFY radiusMetersChanged)
    Q_PROPERTY(QStringList enabledOperators READ enabledOperators WRITE setEnabledOperators NOTIFY enabledOperatorsChanged)
    Q_PROPERTY(bool showNoRide READ showNoRide WRITE setShowNoRide NOTIFY filterChanged)
    Q_PROPERTY(bool showNoParking READ showNoParking WRITE setShowNoParking NOTIFY filterChanged)
    Q_PROPERTY(bool showSlow READ showSlow WRITE setShowSlow NOTIFY filterChanged)

public:
    enum Kind { NoRide = Gbfs::NoRide, NoParking = Gbfs::NoParking, Slow = Gbfs::SlowZone };
    Q_ENUM(Kind)

    enum Roles {
        PolygonRole = Qt::UserRole + 1,
        KindRole,
        NameRole,
        MaxSpeedRole,
        OperatorKeyRole,
        FillColorRole,
        BorderColorRole,
    };

    explicit ZoneModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return int(m_visible.size()); }
    QGeoCoordinate focus() const { return m_focus; }
    void setFocus(const QGeoCoordinate &c);
    double radiusMeters() const { return m_radius; }
    void setRadiusMeters(double r);
    QStringList enabledOperators() const { return m_enabled; }
    void setEnabledOperators(const QStringList &ops);
    bool showNoRide() const { return m_showNoRide; }
    void setShowNoRide(bool on);
    bool showNoParking() const { return m_showNoParking; }
    void setShowNoParking(bool on);
    bool showSlow() const { return m_showSlow; }
    void setShowSlow(bool on);

    void setRecords(QList<ZoneRecord> records);

    // Colours shared with the legend in QML.
    Q_INVOKABLE QColor fillColorFor(int kind) const;
    Q_INVOKABLE QColor borderColorFor(int kind) const;

signals:
    void countChanged();
    void focusChanged();
    void radiusMetersChanged();
    void enabledOperatorsChanged();
    void filterChanged();

private:
    void scheduleRebuild();
    void rebuild();

    QList<ZoneRecord> m_all;
    QList<int> m_visible;
    QGeoCoordinate m_focus;
    double m_radius = 2500;
    QStringList m_enabled;
    bool m_showNoRide = true;
    bool m_showNoParking = true;
    bool m_showSlow = false;
    int m_maxZones = 400;
    QTimer m_rebuildTimer;
};
