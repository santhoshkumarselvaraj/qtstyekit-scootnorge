#pragma once

#include "gbfs.h"

#include <QAbstractListModel>
#include <QGeoCoordinate>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

struct VehicleRecord
{
    QString id;
    int op = -1;              // index into knownOperators()
    QGeoCoordinate coord;
    int battery = -1;         // percent, -1 = unknown
    double rangeKm = -1;      // -1 = unknown
    Gbfs::PricingPlan plan;
    QUrl androidUri;
    QUrl webUri;
};

// Filtered, distance-sorted list of available scooters around `focus`.
class VehicleModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by MobilityService")

    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int totalCount READ totalCount NOTIFY countChanged)
    Q_PROPERTY(QGeoCoordinate focus READ focus WRITE setFocus NOTIFY focusChanged)
    Q_PROPERTY(QStringList enabledOperators READ enabledOperators WRITE setEnabledOperators NOTIFY enabledOperatorsChanged)
    Q_PROPERTY(double minRangeKm READ minRangeKm WRITE setMinRangeKm NOTIFY minRangeKmChanged)
    Q_PROPERTY(int tripMinutes READ tripMinutes WRITE setTripMinutes NOTIFY tripMinutesChanged)
    Q_PROPERTY(int maxResults READ maxResults WRITE setMaxResults NOTIFY maxResultsChanged)
    Q_PROPERTY(double compareRadiusMeters READ compareRadiusMeters WRITE setCompareRadiusMeters NOTIFY compareRadiusMetersChanged)
    Q_PROPERTY(QVariantList comparison READ comparison NOTIFY comparisonChanged)

public:
    enum Roles {
        VehicleIdRole = Qt::UserRole + 1,
        OperatorKeyRole,
        OperatorNameRole,
        OperatorColorRole,
        GeoRole,
        BatteryRole,
        RangeKmRole,
        DistanceRole,
        UnlockFeeRole,
        PerMinuteRole,
        EstimateRole,
        CurrencyRole,
        CheapestRole,
    };
    Q_ENUM(Roles)

    explicit VehicleModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return int(m_rows.size()); }
    int totalCount() const { return m_filteredTotal; }

    QGeoCoordinate focus() const { return m_focus; }
    void setFocus(const QGeoCoordinate &c);
    QStringList enabledOperators() const { return m_enabled; }
    void setEnabledOperators(const QStringList &ops);
    double minRangeKm() const { return m_minRangeKm; }
    void setMinRangeKm(double km);
    int tripMinutes() const { return m_tripMinutes; }
    void setTripMinutes(int m);
    int maxResults() const { return m_maxResults; }
    void setMaxResults(int n);
    double compareRadiusMeters() const { return m_compareRadius; }
    void setCompareRadiusMeters(double m);
    QVariantList comparison() const { return m_comparison; }

    void setRecords(QList<VehicleRecord> records);

    Q_INVOKABLE QVariantMap get(int row) const;
    Q_INVOKABLE bool openInOperatorApp(const QString &vehicleId) const;
    Q_INVOKABLE bool openWalkingDirections(const QString &vehicleId) const;

signals:
    void countChanged();
    void focusChanged();
    void enabledOperatorsChanged();
    void minRangeKmChanged();
    void tripMinutesChanged();
    void maxResultsChanged();
    void compareRadiusMetersChanged();
    void comparisonChanged();

private:
    struct Row { int record; double distance; double estimate; };

    void scheduleRebuild();
    void rebuild();
    const VehicleRecord *find(const QString &id) const;

    QList<VehicleRecord> m_all;
    QList<VehicleRecord> m_pending;
    bool m_hasPending = false;
    QList<Row> m_rows;
    double m_cheapest = -1;
    int m_filteredTotal = 0;
    QVariantList m_comparison;

    QGeoCoordinate m_focus;
    QStringList m_enabled;
    double m_minRangeKm = 0;
    int m_tripMinutes = 10;
    int m_maxResults = 250;
    double m_compareRadius = 500;
    QTimer m_rebuildTimer;
};
