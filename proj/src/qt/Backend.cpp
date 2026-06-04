#include "Backend.h"

import system.nav_graph;
import core.scheduler;
import app.context;
import app.tasks;
import system.config;
import core.queue;
import core.event;

#include <QTimer>
#include <QDebug>
#include <stdexcept>
#include <simdjson.h>

struct Backend::Impl {
    TaskContext ctx;
    Scheduler<TaskContext, MAX_TASKS, EVENT_QUEUE_SIZE> scheduler;
    QTimer tickTimer;

    Impl(const std::string &navFile)
        : ctx(NavGraph(navFile)), scheduler(ctx)
    {
        scheduler.addTask({taskSamplePosition,      PRIORITY_HIGH, 10,  0});
        scheduler.addTask({taskNavigationState,     PRIORITY_HIGH, 50, 10});
        scheduler.addTask({taskPeriodicRouteCheck,  PRIORITY_HIGH, 30,  5});
        scheduler.addTask({taskWatchdog,            PRIORITY_HIGH, 50, 15});
    }
};

Backend::Backend(QObject *parent)
    : QObject(parent), d(std::make_unique<Impl>("../simScript/maps/Warsaw.nav"))
{
    qDebug() << "Graph loaded:" << d->ctx.graph.node_count() << "nodes,"
             << d->ctx.graph.edge_count() << "edges";

    connect(&d->tickTimer, &QTimer::timeout, this, [this]() {
        d->scheduler.tick();
        updateRouteGeometry();
    });
}

Backend::~Backend() = default;

void Backend::startSimulation() {
    d->tickTimer.start(TICK_MS);
}

void Backend::stopSimulation() {
    d->tickTimer.stop();
}

void Backend::onWaypointsReceived(const QVariantList &points) {
    qDebug() << "[Backend] Received waypoints, count:" << points.size();
    if (!points.isEmpty()) {
        qDebug() << "[Backend] First element type:" << points.first().typeName()
                 << "value:" << points.first();
    }
    auto *coords = new std::vector<std::pair<double, double>>;
    for (const auto &p : points) {
        auto list = p.toList();
        if (list.size() < 2) {
            qWarning() << "Invalid waypoint format, expected [lat, lon]";
            continue;
        }
        coords->emplace_back(list[0].toDouble(), list[1].toDouble());
    }
    Event e;
    e.type = EventType::RouteNodesUpdate;
    e.data = coords;
    d->scheduler.getEventQueue().push(e);
}

QVariantList Backend::routePath() const {
    return m_routePath;
}

void Backend::updateRouteGeometry() {
    const auto &route = d->ctx.current_route;   // vector<uint32_t>
    QVariantList path;
    path.reserve(route.size());
    for (uint32_t idx : route) {
        QVariantMap point;
        point["lat"] = d->ctx.graph.node_lat(idx);
        point["lon"] = d->ctx.graph.node_lon(idx);
        path.append(point);
    }

    if (path != m_routePath) {
        m_routePath = std::move(path);
        emit routeChanged();
    }
}

// Helper: OSM ID → index
static uint32_t findNodeIndexByOsm(const NavGraph& graph, uint64_t osm_id) {
    uint32_t left = 0, right = static_cast<uint32_t>(graph.node_count() - 1);
    while (left <= right) {
        uint32_t mid = (left + right) / 2;
        uint64_t id = graph.node_osm_id(mid);
        if (id < osm_id) left = mid + 1;
        else if (id > osm_id) right = mid - 1;
        else return mid;
    }
    throw std::runtime_error("OSM node not found");
}

// ---- Slot for raw JSON ----
void Backend::onWaypointsJson(const QString &json) {
    simdjson::dom::parser parser;
    simdjson::dom::element doc;
    auto error = parser.parse(json.toStdString()).get(doc);
    if (error) {
        qWarning() << "Failed to parse waypoints JSON";
        return;
    }
    simdjson::dom::array coords_array;
    if (doc["coordinates"].get_array().get(coords_array)) {
        qWarning() << "Missing 'coordinates' array";
        return;
    }

    auto* coords = new std::vector<std::pair<double, double>>;
    QVariantList qmlPoints;                     // for QML

    for (auto point : coords_array) {
        double lat, lon;
        simdjson::dom::array point_arr;
        if (point.get_array().get(point_arr)) continue;
        if (point_arr.at(0).get_double().get(lat) ||
            point_arr.at(1).get_double().get(lon)) continue;
        coords->emplace_back(lat, lon);

        QVariantMap m;
        m["lat"] = lat;
        m["lon"] = lon;
        qmlPoints.append(m);
    }

    emit waypointsReceived(qmlPoints);          // ← let QML update pointModel

    Event e;
    e.type = EventType::RouteNodesUpdate;
    e.data = coords;
    d->scheduler.getEventQueue().push(e);

    qDebug() << "[Backend] Pushed" << coords->size() << "waypoints into event queue";
}

// ---- Slot for edge updates (kept for QML visualisation) ----
void Backend::onGraphEdgeUpdated(uint64_t osm_u, uint64_t osm_v, double newWeight) {
    try {
        uint32_t uIdx = findNodeIndexByOsm(d->ctx.graph, osm_u);
        uint32_t vIdx = findNodeIndexByOsm(d->ctx.graph, osm_v);

        d->ctx.graph.update_edge_weight(uIdx, vIdx, newWeight);

        double lat1 = d->ctx.graph.node_lat(uIdx);
        double lon1 = d->ctx.graph.node_lon(uIdx);
        double lat2 = d->ctx.graph.node_lat(vIdx);
        double lon2 = d->ctx.graph.node_lon(vIdx);
        emit graphEdgeUpdated(osm_u, osm_v, newWeight, lat1, lon1, lat2, lon2);

        // Optionally trigger a local replan
        Event e;
        e.type = EventType::LocalReplanRequest;
        e.data = nullptr;
        d->scheduler.getEventQueue().push(e);
    } catch (const std::exception &ex) {
        qWarning() << "Graph update failed:" << ex.what();
    }
}