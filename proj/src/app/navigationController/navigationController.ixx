module;

#include <tuple>
#include <cstdint>
#include <cstdio>
#include <vector>
#include <random>

export module app.navigationController;

import app.context;
import core.queue;
import system.config;
import core.event;

export void onGraphUpdate(TaskContext& ctx, EventQueue<EVENT_QUEUE_SIZE> & queue, void* data) {
    auto* d = static_cast<std::tuple<uint64_t, uint64_t, double>*>(data);
    auto [u, v, new_weight] = *d;

    printf("onGraphUpdate: edge (%llu, %llu) new weight = %.2f\n",
           (unsigned long long)u, (unsigned long long)v, new_weight);

    // Update the graph (dynamic override)
    ctx.graph.update_edge_weight(u, v, new_weight);

    // Check if this edge is part of the current route
    if (ctx.route_edges.contains({u, v}))
        printf("  -> Edge is on current route. \n");

    delete d;
    Event e;
    e.type = EventType::LocalReplanRequest;
    e.data = nullptr;
    queue.push(e);
}

export void onRouteNodesUpdate(TaskContext& ctx,
                               EventQueue<EVENT_QUEUE_SIZE>& queue,
                               void* data)
{
    auto* coords = static_cast<std::vector<std::pair<double, double>>*>(data);
    printf("onRouteNodesUpdate: received %zu waypoints\n", coords->size());

    std::vector<uint32_t> waypoint_nodes;
    waypoint_nodes.reserve(coords->size());
    for (auto [lat, lon] : *coords) {
        uint32_t node = ctx.graph.find_nearest_node(lat, lon);
        waypoint_nodes.push_back(node);
        printf("  (%.6f, %.6f) -> node %u\n", lat, lon, node);
    }

    // Generate a random start/return node.
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<double> lat_dist(ctx.graph.MIN_LAT, ctx.graph.MAX_LAT);
    std::uniform_real_distribution<double> lon_dist(ctx.graph.MIN_LON, ctx.graph.MAX_LON);
    double start_lat = lat_dist(gen);
    double start_lon = lon_dist(gen);
    uint32_t start_node = ctx.graph.find_nearest_node(start_lat, start_lon);
    printf("Generated start/end: (%.6f, %.6f) -> node %u\n",
           start_lat, start_lon, start_node);

    ctx.start_node = start_node;
    // FIX: do NOT push start_node into waypoint_nodes.
    // It is used as origin and return terminus by order_waypoints_nearest_neighbor.
    ctx.waypoint_nodes = std::move(waypoint_nodes);

    ctx.vehicle.current_node = start_node;
    ctx.vehicle.lat          = ctx.graph.node_lat(start_node);
    ctx.vehicle.lon          = ctx.graph.node_lon(start_node);
    ctx.vehicle.heading      = 0.0;
    ctx.vehicle.speed_ms     = 0.0;

    ctx.current_waypoint_index = 0;  // reset progress tracker
    ctx.route_edges.clear();
    ctx.replanning = false;          // cancel any in-progress replan flag

    delete coords;

    Event e;
    e.type = EventType::GlobalReplanRequest;
    e.data = nullptr;
    queue.push(e);
}

export void onOffRouteDetected(TaskContext& ctx, EventQueue<EVENT_QUEUE_SIZE>& queue, void* data) {
    printf("onOffRouteDetected\n");
}     // wyznacza nową trasę

export void onVehicleUpdate(TaskContext& ctx, EventQueue<EVENT_QUEUE_SIZE>& queue, void* data) {} //aktualizacje pozycji (musisz zaimplementować z gui), powinno sprawdzić czy
//jesteśmy na trasie dla ^ (na razie fejkuje pozycje jak dostaje waypointy, itd.)

// Request methods (push internal events)
void requestLocalReplan() { /* push LocalReplanRequest */ }
void requestGlobalReplan() { /* push GlobalReplanRequest */ }
void requestOffRouteWarning() { /* push OffRouteWarning event for UI */ }          // aktualizacja grafu (wypadki, itd)