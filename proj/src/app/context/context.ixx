module;

#include <iostream>
#include <vector>
#include <cstdint>
#include <unordered_set>
#include <random>

export module app.context;

import core.event;
import system.nav_graph;
import core.event;

struct pair_hash {
    std::size_t operator()(const std::pair<uint32_t, uint32_t>& p) const {
        return (static_cast<std::size_t>(p.first) << 32) | p.second;
    }
};

export struct TaskContext {
    NavGraph graph;                     // loaded .nav (shared read‑only)
    std::vector<uint32_t> waypoint_nodes; // ordered list of target node indices
    uint32_t start_node;                    // vehicle's start node (also end)
    size_t current_waypoint_idx = 0;    // next waypoint to reach
    std::vector<uint32_t> current_route;
    std::unordered_set<std::pair<uint32_t,uint32_t>, pair_hash> route_edges; // krawędzie trasy
    struct {
        uint32_t current_node;              // current graph node (or nearest)
        double lat, lon;                // current position
        double heading;
        double speed_ms;
    } vehicle;

    // Constructor
    TaskContext(NavGraph&& g): graph(std::move(g)) {
    }

    void onGraphUpdate(void* data) {
        auto* d = static_cast<std::tuple<uint64_t, uint64_t, double>*>(data);
        auto [u, v, new_weight] = *d;

        printf("onGraphUpdate: edge (%llu, %llu) new weight = %.2f\n",
               (unsigned long long)u, (unsigned long long)v, new_weight);

        // Update the graph (dynamic override)
        graph.update_edge_weight(u, v, new_weight);

        // Check if this edge is part of the current route
        if (route_edges.contains({u, v}))
            printf("  -> Edge is on current route. \n");

        delete d;
    }

    void onRouteNodesUpdate(void* data) {
        auto* coords = static_cast<std::vector<std::pair<double, double>>*>(data);

        printf("onRouteNodesUpdate: received %zu unordered waypoints\n", coords->size());

        // 1. Convert all received coordinates to node indices (unordered set)
        std::vector<uint32_t> waypoint_nodes;
        waypoint_nodes.reserve(coords->size());
        for (auto [lat, lon] : *coords) {
            uint32_t node = graph.find_nearest_node(lat, lon);
            waypoint_nodes.push_back(node);
            printf("  (%.6f, %.6f) -> node %u\n", lat, lon, node);
        }

        // 2. Generate a random start/end point within map bounds
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<double> lat_dist(graph.MIN_LAT, graph.MAX_LAT);
        std::uniform_real_distribution<double> lon_dist(graph.MIN_LON, graph.MAX_LON);
        double start_lat = lat_dist(gen);
        double start_lon = lon_dist(gen);
        uint32_t start_node = graph.find_nearest_node(start_lat, start_lon);
        printf("Generated start/end point: (%.6f, %.6f) -> node %u\n",
               start_lat, start_lon, start_node);
        this->start_node = start_node;
        waypoint_nodes.push_back(start_node);

        // 3. Set vehicle state
        vehicle.current_node = start_node;
        vehicle.lat = start_lat;
        vehicle.lon = start_lon;
        vehicle.heading = 0.0;
        vehicle.speed_ms = 0.0;

        // 4. Store waypoints (unordered) and clear any previous route
        this->waypoint_nodes = std::move(waypoint_nodes);
        route_edges.clear();

        delete coords;
    } //dodanie default waypointów (można rozwinąć o aktualizację poprzednich)

    void onOffRouteDetected(void* data) {
        printf("onOffRouteDetected\n");
    }     // wyznacza nową trasę

    void onVehicleUpdate(void* data) {} //aktualizacje pozycji (musisz zaimplementować z gui), powinno sprawdzić czy
    //jesteśmy na trasie dla ^ (na razie fejkuje pozycje jak dostaje waypointy, itd.)

    // Request methods (push internal events)
    void requestLocalReplan() { /* push LocalReplanRequest */ }
    void requestGlobalReplan() { /* push GlobalReplanRequest */ }
    void requestOffRouteWarning() { /* push OffRouteWarning event for UI */ }          // aktualizacja grafu (wypadki, itd)

};

