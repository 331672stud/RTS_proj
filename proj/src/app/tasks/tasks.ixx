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
    std::cout << "Task localReplan" << std::endl;
    if (ctx.current_route.empty() || ctx.waypoint_nodes.empty()) return;

    // Determine the next target waypoint (first not yet reached)
    // For simplicity, find the first waypoint that is not the current node
    uint32_t target = ctx.waypoint_nodes.front(); // placeholder
    // Actually, we should track which waypoint we are heading to.
    // For now, use the last waypoint (or first)
    target = ctx.waypoint_nodes.back(); // just to compile

    double max_distance_m = 2000.0; // 2 km radius
    auto result = a_star(ctx.graph, ctx.vehicle.current_node, target,
                         std::numeric_limits<double>::infinity(), max_distance_m);
    if (result.path.empty()) {
        std::cerr << "Local replan failed, falling back to global replan" << std::endl;
        // Uncomment when Event and scheduler are ready
        // Event e;
        // e.type = EventType::GlobalReplanRequest;
        // e.data = nullptr;
        // ctx.scheduler.getEventQueue().push(e);
        return;
    }

    // Find the position of current_node in the existing route
    auto pos = std::find(ctx.current_route.begin(), ctx.current_route.end(), ctx.vehicle.current_node);
    if (pos != ctx.current_route.end()) {
        // Erase from pos+1 to end
        ctx.current_route.erase(pos + 1, ctx.current_route.end());
        // Append the new path (skip the first node because it's already current_node)
        ctx.current_route.insert(ctx.current_route.end(), result.path.begin() + 1, result.path.end());
    } else {
        // Current node not in route – replace entirely
        ctx.current_route = result.path;
    }

    // Rebuild route_edges from current_route
    ctx.route_edges.clear();
    for (size_t i = 0; i + 1 < ctx.current_route.size(); ++i) {
        ctx.route_edges.insert({ctx.current_route[i], ctx.current_route[i+1]});
    }
}

export void taskGlobalReplan(TaskContext& ctx) {
    std::cout << "Task globalReplan" << std::endl;
    if (ctx.waypoint_nodes.empty()) {
        std::cerr << "No waypoints, nothing to plan" << std::endl;
        return;
    }

    // Order waypoints using nearest neighbor heuristic (including start->end)
    std::vector<uint32_t> ordered = order_waypoints_nearest_neighbor(
        ctx.graph, ctx.vehicle.current_node, ctx.waypoint_nodes
    );

    // Compute paths between consecutive nodes in ordered list
    std::vector<uint32_t> full_route;
    full_route.push_back(ordered[0]);
    double total_cost = 0.0;
    for (size_t i = 0; i + 1 < ordered.size(); ++i) {
        uint32_t from = ordered[i];
        uint32_t to = ordered[i+1];
        auto result = a_star(ctx.graph, from, to);
        if (result.path.empty()) {
            std::cerr << "No path from " << from << " to " << to << std::endl;
            return;
        }
        total_cost += result.cost;
        // Append path without duplicating the 'from' node
        full_route.insert(full_route.end(), result.path.begin() + 1, result.path.end());
    }

    ctx.current_route = std::move(full_route);

    // Rebuild route_edges set
    ctx.route_edges.clear();
    for (size_t i = 0; i + 1 < ctx.current_route.size(); ++i) {
        ctx.route_edges.insert({ctx.current_route[i], ctx.current_route[i+1]});
    }

    std::cout << "Global replan done. Route length: " << ctx.current_route.size()
              << " nodes, cost: " << total_cost << std::endl;
}

export void taskPeriodicRouteCheck(TaskContext& ctx) {
    std::cout << "Task periodic route check" << std::endl;
    if (ctx.current_route.empty() || ctx.waypoint_nodes.empty()) return;

    // Final destination is the start_node (which is also the end)
    uint32_t final_destination = ctx.start_node;

    auto new_route_result = a_star(ctx.graph, ctx.vehicle.current_node, final_destination);
    if (new_route_result.path.empty()) return;

    // Compute current route cost from current_node to final_destination
    auto pos = std::find(ctx.current_route.begin(), ctx.current_route.end(), ctx.vehicle.current_node);
    double current_cost = 0.0;
    if (pos != ctx.current_route.end()) {
        for (auto it = pos; it + 1 != ctx.current_route.end(); ++it) {
            uint32_t u = *it;
            uint32_t v = *(it + 1);
            auto edge_idx = ctx.graph.find_edge_index(u, v);
            if (edge_idx) {
                current_cost += ctx.graph.current_edge_weight(*edge_idx);
            } else {
                current_cost = std::numeric_limits<double>::infinity();
                break;
            }
        }
    } else {
        current_cost = std::numeric_limits<double>::infinity();
    }

    double new_cost = new_route_result.cost;
    if (new_cost < 0.8 * current_cost) {
        std::cout << "Periodic check found significantly better route (new: " << new_cost
                  << ", old: " << current_cost << "). Requesting global replan." << std::endl;
        // Uncomment when Event and scheduler are ready
        // Event e;
        // e.type = EventType::GlobalReplanRequest;
        // e.data = nullptr;
        // ctx.scheduler.getEventQueue().push(e);
    }
}

export void taskWatchdog(TaskContext& ctx) {
    std::cout << "Task watchdog" << std::endl;
    // TODO: monitor queue depth, replan latency, etc.
}