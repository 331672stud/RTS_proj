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

};

