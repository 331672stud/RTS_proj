module;

#include <iostream>
#include <vector>
#include <cstdint>

export module app.context;

import core.event;
import system.nav_graph;
import core.event;


export struct TaskContext {
    NavGraph graph;                     // loaded .nav (shared read‑only)
    std::vector<uint32_t> waypoint_nodes; // ordered list of target node indices
    size_t current_waypoint_idx = 0;    // next waypoint to reach
    std::vector<uint32_t> current_route; // node indices along the current path
    struct {
        uint32_t node_idx;              // current graph node (or nearest)
        double lat, lon;                // current position
        double heading;
        double speed_ms;
    } vehicle;

    // For checking if an edge belongs to current route
    //std::unordered_set<std::pair<uint32_t,uint32_t>> route_edges;

    // Constructor
    TaskContext(NavGraph&& g): graph(std::move(g)) {}

    void onGraphUpdate(void* data) {
        printf("onGraphUpdate\n");
    } //na aktualizacji wag

    void onOffRouteDetected(void* data) {
        printf("onOffRouteDetected\n");
    }     // wyznacza nową trasę

    void onRouteNodesUpdate(void* data) {
        printf("onRouteNodesUpdate\n");
    } //dodanie default waypointów (można rozwinąć o aktualizację poprzednich)

    // Request methods (push internal events)
    void requestLocalReplan() { /* push LocalReplanRequest */ }
    void requestGlobalReplan() { /* push GlobalReplanRequest */ }
    void requestOffRouteWarning() { /* push OffRouteWarning event for UI */ }          // aktualizacja grafu (wypadki, itd)
    void requestPositionUpdate(){}
};