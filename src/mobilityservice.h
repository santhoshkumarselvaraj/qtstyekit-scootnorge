#pragma once

#include "gbfs.h"
#include "vehiclemodel.h"
#include "zonemodel.h"

#include <QDateTime>
#include <QGeoCoordinate>
#include <QNetworkAccessManager>
#include <QObject>
#include <QTimer>
#include <QtQml/qqmlregistration.h>
#include <functional>

// Talks to Entur's open GBFS aggregation (NLOD licence) and feeds the models.
class MobilityService : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString city READ city WRITE setCity NOTIFY cityChanged)
    Q_PROPERTY(QVariantList cities READ cities CONSTANT)
    Q_PROPERTY(QGeoCoordinate cityCenter READ cityCenter NOTIFY cityChanged)
    Q_PROPERTY(QVariantList operators READ operators CONSTANT)
    Q_PROPERTY(QStringList systems READ systems NOTIFY systemsChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QDateTime lastUpdated READ lastUpdated NOTIFY lastUpdatedChanged)
    Q_PROPERTY(int refreshIntervalSec READ refreshIntervalSec WRITE setRefreshIntervalSec NOTIFY refreshIntervalSecChanged)
    Q_PROPERTY(VehicleModel *vehicles READ vehicles CONSTANT)
    Q_PROPERTY(ZoneModel *zones READ zones CONSTANT)

public:
    explicit MobilityService(QObject *parent = nullptr);

    QString city() const { return m_city; }
    void setCity(const QString &city);
    QVariantList cities() const;
    QGeoCoordinate cityCenter() const;
    QVariantList operators() const;
    QStringList systems() const;
    bool loading() const { return m_pending > 0; }
    QString error() const { return m_error; }
    QDateTime lastUpdated() const { return m_lastUpdated; }
    int refreshIntervalSec() const { return m_refreshTimer.interval() / 1000; }
    void setRefreshIntervalSec(int s);
    VehicleModel *vehicles() const { return m_vehicles; }
    ZoneModel *zones() const { return m_zones; }

    Q_INVOKABLE void reload();   // rediscover systems for the current city
    Q_INVOKABLE void refresh();  // refetch vehicle positions only

signals:
    void cityChanged();
    void systemsChanged();
    void loadingChanged();
    void errorChanged();
    void lastUpdatedChanged();
    void refreshIntervalSecChanged();

private:
    struct System
    {
        QString id;
        int op = -1;
        QHash<QString, QUrl> feeds;
        QHash<QString, Gbfs::VehicleType> types;
        QHash<QString, Gbfs::PricingPlan> plans;
        QList<Gbfs::Vehicle> vehicles;
        QList<Gbfs::Zone> zones;
    };

    using JsonHandler = std::function<void(const QJsonObject &)>;
    void get(const QUrl &url, JsonHandler onOk);
    void setError(const QString &e);
    void discoverFromManifest(const QJsonObject &manifest);
    void addSystem(const QString &systemId, int op, const QUrl &gbfsUrl);
    void loadSystemFeeds(int index);
    void fetchVehicles(int index);
    void schedulePublish();
    void publish();

    QNetworkAccessManager m_nam;
    VehicleModel *m_vehicles;
    ZoneModel *m_zones;
    QList<System> m_systemList;
    QString m_city = QStringLiteral("oslo");
    QString m_error;
    QDateTime m_lastUpdated;
    int m_pending = 0;
    int m_generation = 0;
    QTimer m_refreshTimer;
    QTimer m_publishTimer;
    bool m_zonesDirty = false;
    bool m_started = false;
};
