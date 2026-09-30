#pragma once

#include <QColor>
#include <QGeoCoordinate>
#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QString>
#include <QUrl>
#include <QVariantList>

// Plain data types + parsers for the GBFS feeds (v3.0, with v2.3 fallbacks)
// served by Entur: https://api.entur.io/mobility/v2/gbfs/v3/manifest.json
namespace Gbfs {

struct PriceSegment
{
    double start = 0;     // minute the segment starts
    double rate = 0;      // amount charged per interval
    double interval = 1;  // minutes per charge
    double end = -1;      // -1 = open ended
};

struct PricingPlan
{
    QString id;
    QString currency = QStringLiteral("NOK");
    double price = 0;     // fixed part, usually the unlock fee
    QList<PriceSegment> perMin;
    bool valid = false;

    double estimate(double minutes) const;
    double perMinuteRate() const; // first segment, normalised to 1 minute
};

struct VehicleType
{
    QString id;
    QString formFactor;
    double maxRangeMeters = -1;
    QString defaultPlanId;
};

struct Vehicle
{
    QString id;
    double lat = 0;
    double lon = 0;
    bool reserved = false;
    bool disabled = false;
    double rangeMeters = -1;
    double fuelPercent = -1; // 0..100
    QString typeId;
    QString planId;
    QUrl androidUri;
    QUrl webUri;
};

enum ZoneKind { NoRide = 0, NoParking = 1, SlowZone = 2 };

struct Zone
{
    int kind = NoRide;
    QString name;
    double maxSpeedKph = -1;
    QVariantList path;   // list of QGeoCoordinate (outer ring)
    QGeoCoordinate center;
    double radiusMeters = 0;
};

QString localized(const QJsonValue &v);
double number(const QJsonValue &v, double fallback = -1);

QHash<QString, QUrl> parseFeeds(const QJsonObject &root);
QList<VehicleType> parseVehicleTypes(const QJsonObject &root);
QList<PricingPlan> parsePricingPlans(const QJsonObject &root);
QList<Vehicle> parseVehicles(const QJsonObject &root);
QList<Zone> parseZones(const QJsonObject &root);

} // namespace Gbfs
