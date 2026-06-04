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
    void edgeUpdatedForDisplay(uint64_t osm_u, uint64_t osm_v,
                               double lat1, double lon1,
                               double lat2, double lon2, double newWeight);
    void graphEdgeUpdated(uint64_t u, uint64_t v, double weight,
                          double lat1, double lon1, double lat2, double lon2);
    void waypointsReceived(const QVariantList &coords);
    
private:
    void updateRouteGeometry();

    struct Impl;
    std::unique_ptr<Impl> d;
    QVariantList m_routePath;
};