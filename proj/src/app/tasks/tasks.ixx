module;

#include <iostream>
#include <vector>
#include <algorithm>
#include <limits>
#include <cstdint>
#include <unordered_set>

export module app.tasks;

import app.context;
import core.event;
import app.astar;

export void taskSamplePosition(TaskContext& ctx) {
    std::cout << "Task sample position" << std::endl;
    // TODO: read GPS, update ctx.vehicle, check deviation
}

export void taskNavigationState(TaskContext& ctx) {
    std::cout << "Task navigationState" << std::endl;
    // TODO: compute ETA, next turn, update UI
}

export void taskLocalReplan(TaskContext& ctx) {
    if (ctx.current_route.empty() || ctx.waypoint_nodes.empty()) return;

    // Find where we are in the existing route.
    auto pos = std::find(ctx.current_route.begin(),
                         ctx.current_route.end(),
                         ctx.vehicle.current_node);

    // Determine the next waypoint target: first waypoint node that appears
    // *after* the vehicle's current position in the route, or — if the vehicle
    // is not on the route at all — the nearest waypoint by graph distance.
    uint32_t target = ctx.waypoint_nodes.back(); // fallback
    if (pos != ctx.current_route.end()) {
        // Scan forward from pos to find the next waypoint in the route.
        std::unordered_set<uint32_t> wp_set(ctx.waypoint_nodes.begin(),
                                             ctx.waypoint_nodes.end());
        for (auto it = pos + 1; it != ctx.current_route.end(); ++it) {
            if (wp_set.count(*it)) {
                target = *it;
                break;
            }
        }
    }

    double max_distance_m = 2000.0;
    auto result = a_star(ctx.graph,
                         ctx.vehicle.current_node,
                         target,
                         std::numeric_limits<double>::infinity(),
                         max_distance_m);

    if (result.path.empty()) {
        std::cerr << "[localReplan] A* failed within 2 km, requesting global replan\n";
        // Push a GlobalReplanRequest so the scheduler queues taskGlobalReplan.
        Event e;
        e.type = EventType::GlobalReplanRequest;
        e.data = nullptr;
        ctx.queue->push(e);
        return;
    }

    if (pos != ctx.current_route.end()) {
        // Keep everything up to and including current_node, replace the rest.
        ctx.current_route.erase(pos + 1, ctx.current_route.end());
        ctx.current_route.insert(ctx.current_route.end(),
                                 result.path.begin() + 1,
                                 result.path.end());
    } else {
        // Vehicle is off-route; replace the whole route.
        ctx.current_route = result.path;
    }

    // Rebuild route_edges from the updated route.
    ctx.route_edges.clear();
    for (size_t i = 0; i + 1 < ctx.current_route.size(); ++i) {
        ctx.route_edges.insert({ctx.current_route[i], ctx.current_route[i + 1]});
    }
}

export void taskGlobalReplan(TaskContext& ctx) {
    if (ctx.waypoint_nodes.empty()) {
        std::cerr << "[globalReplan] No waypoints, nothing to plan\n";
        ctx.replanning = false;
        return;
    }

    // Remove start_node from the waypoint list to avoid duplicates.
    // order_waypoints_nearest_neighbor handles start as origin and appends it
    // as return terminus automatically.
    std::vector<uint32_t> intermediate_waypoints;
    intermediate_waypoints.reserve(ctx.waypoint_nodes.size());
    for (uint32_t n : ctx.waypoint_nodes) {
        if (n != ctx.vehicle.current_node) {
            intermediate_waypoints.push_back(n);
        }
    }

    if (intermediate_waypoints.empty()) {
        std::cerr << "[globalReplan] All waypoints equal current node, nothing to plan\n";
        ctx.replanning = false;
        return;
    }

    // TSP nearest-neighbour ordering: returns [current, wp1, wp2, ..., current]
    std::vector<uint32_t> ordered = order_waypoints_nearest_neighbor(
        ctx.graph, ctx.vehicle.current_node, intermediate_waypoints);

    // Run A* between each consecutive pair and concatenate.
    std::vector<uint32_t> full_route;
    full_route.push_back(ordered[0]);
    double total_cost = 0.0;

    for (size_t i = 0; i + 1 < ordered.size(); ++i) {
        uint32_t from = ordered[i];
        uint32_t to   = ordered[i + 1];
        auto result   = a_star(ctx.graph, from, to);
        if (result.path.empty()) {
            std::cerr << "[globalReplan] No path from " << from
                      << " to " << to << "\n";
            ctx.replanning = false;
            return;
        }
        total_cost += result.cost;
        // Skip result.path[0] because `from` is already the last node in full_route.
        full_route.insert(full_route.end(),
                          result.path.begin() + 1,
                          result.path.end());
    }

    ctx.current_route = std::move(full_route);

    // Rebuild route_edges.
    ctx.route_edges.clear();
    for (size_t i = 0; i + 1 < ctx.current_route.size(); ++i) {
        ctx.route_edges.insert({ctx.current_route[i], ctx.current_route[i + 1]});
    }

    ctx.replanning = false;  // allow future periodic checks

    std::cout << "[globalReplan] Done. Route: " << ctx.current_route.size()
              << " nodes, cost: " << total_cost << "\n";
}

export void taskPeriodicRouteCheck(TaskContext& ctx) {
    if (ctx.current_route.empty() || ctx.waypoint_nodes.empty()) return;
    if (ctx.replanning) return;  // a replan is already queued or running

    // Find current position in route.
    auto pos = std::find(ctx.current_route.begin(),
                         ctx.current_route.end(),
                         ctx.vehicle.current_node);

    // Measure cost of remaining route from current_node onward.
    double current_remaining_cost = 0.0;
    if (pos != ctx.current_route.end()) {
        for (auto it = pos; it + 1 != ctx.current_route.end(); ++it) {
            auto edge_idx = ctx.graph.find_edge_index(*it, *(it + 1));
            if (edge_idx) {
                current_remaining_cost += ctx.graph.current_edge_weight(*edge_idx);
            } else {
                current_remaining_cost = std::numeric_limits<double>::infinity();
                break;
            }
        }
    } else {
        // Vehicle is off-route; always trigger a replan.
        std::cout << "[periodicCheck] Vehicle off route, requesting global replan\n";
        ctx.replanning = true;
        Event e;
        e.type = EventType::GlobalReplanRequest;
        e.data = nullptr;
        ctx.queue->push(e);
        return;
    }

    // Find the next waypoint target (same logic as taskLocalReplan).
    std::unordered_set<uint32_t> wp_set(ctx.waypoint_nodes.begin(),
                                         ctx.waypoint_nodes.end());
    uint32_t next_target = ctx.waypoint_nodes.back();
    for (auto it = pos + 1; it != ctx.current_route.end(); ++it) {
        if (wp_set.count(*it)) { next_target = *it; break; }
    }

    // Cost of a fresh direct path to the next waypoint.
    auto fresh = a_star(ctx.graph, ctx.vehicle.current_node, next_target);
    if (fresh.path.empty()) return; // no alternative found

    if (fresh.cost < 0.8 * current_remaining_cost) {
        std::cout << "[periodicCheck] Significantly better route found (fresh: "
                  << fresh.cost << ", current: " << current_remaining_cost
                  << "). Requesting global replan.\n";
        ctx.replanning = true;
        Event e;
        e.type = EventType::GlobalReplanRequest;
        e.data = nullptr;
        ctx.queue->push(e);
    }
}

export void taskWatchdog(TaskContext& ctx) {
    std::cout << "Task watchdog" << std::endl;
    // TODO: monitor queue depth, replan latency, etc.
}