#pragma once
#include <QObject>
#include <QVariantList>
#include <cstdint>

class Backend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList routePath READ routePath NOTIFY routeChanged)

public:
    explicit Backend(QObject *parent = nullptr);
    ~Backend();

    QVariantList routePath() const;

public slots:
    void onWaypointsReceived(const QVariantList &points);

    void onWaypointsJson(const QString &json);
    void onGraphEdgeUpdated(uint64_t osm_u, uint64_t osm_v, double newWeight);

    void startSimulation();
    void stopSimulation();

signals:
    void routeChanged();
    void routeRecalculating();                       // NEW — emitted before any replan
    void routeRecalculated();                        // NEW — emitted after current_route is updated
    void vehiclePositionChanged(double lat,
                                double lon,
                                double heading,
                                double speedMs);     // NEW — drives the vehicle marker in QML
    void startNodeChanged(double lat, double lon);   // NEW — random origin, shown on map
    void offRouteDetected(double lat, double lon);   // NEW — off-route warning for QML
    void simulationError(const QString& message);    // NEW — surfaces C++ errors to QML
    void graphEdgeUpdated(quint64 osmU, quint64 osmV,
                          double  weight,
                          double  lat1, double lon1,
                          double  lat2, double lon2);
    void edgeUpdatedForDisplay(uint64_t osm_u, uint64_t osm_v,
                               double lat1, double lon1,
                               double lat2, double lon2, double newWeight);
    void waypointsReceived(const QVariantList &coords);
    
private:
    void updateRouteGeometry();
    void updateVehicleState();   // NEW — emits vehiclePositionChanged / startNodeChanged
    void updateWaypointsModel(); // NEW — emit updated waypoints to QML

    uint32_t m_lastEmittedStartNode = UINT32_MAX;

    struct Impl;
    std::unique_ptr<Impl> d;
    QVariantList m_routePath;
    QVariantList m_waypoints;
};