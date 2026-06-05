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
import system.config;
import core.queue;

struct pair_hash {
    std::size_t operator()(const std::pair<uint32_t, uint32_t>& p) const {
        return (static_cast<std::size_t>(p.first) << 32) | p.second;
    }
};

export struct VehicleState {
    uint32_t current_node = 0;
    double   lat          = 0.0;
    double   lon          = 0.0;
    double   heading      = 0.0;
    double   speed_ms     = 0.0;
};

export struct TaskContext {
    NavGraph                          graph;
    VehicleState                      vehicle;
    uint32_t                          start_node           = 0;
    std::vector<uint32_t>             current_route;
    std::vector<uint32_t>             waypoint_nodes;
    std::unordered_set<
        std::pair<uint32_t, uint32_t>,
        pair_hash>                     route_edges;
    size_t                            current_waypoint_index = 0;  // NEW
    bool                              replanning             = false; // NEW

    // Pointer to the scheduler's EventQueue so tasks can push events.
    // Set by Scheduler immediately after constructing TaskContext.
    // Never null during task execution.
    EventQueue<EVENT_QUEUE_SIZE>*     queue                  = nullptr; // NEW

    explicit TaskContext(NavGraph&& g) : graph(std::move(g)) {}
};

