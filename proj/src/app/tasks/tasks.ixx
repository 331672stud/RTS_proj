module;

#include <iostream>
#include <vector>
#include <algorithm>
#include <limits>
#include <cstdint>
#include <unordered_set>
#include <cmath>

export module app.tasks;

import app.context;
import core.event;
import app.astar;
import system.config;

static constexpr double PI = 3.14159265358979323846;
static constexpr double DEFAULT_SPEED_MS = 5000.0;

static void remove_consecutive_duplicates(std::vector<uint32_t>& route) noexcept {
    size_t dst = 0;
    for (size_t src = 0; src < route.size(); ++src) {
        if (dst == 0 || route[src] != route[dst - 1]) {
            route[dst++] = route[src];
        }
    }
    route.resize(dst);
}

static double degrees_to_radians(double degrees) noexcept {
    return degrees * (PI / 180.0);
}

static double haversine_distance_m(double lat1, double lon1,
                                  double lat2, double lon2) noexcept {
    double dlat = degrees_to_radians(lat2 - lat1);
    double dlon = degrees_to_radians(lon2 - lon1);
    double a = std::sin(dlat / 2.0) * std::sin(dlat / 2.0)
             + std::cos(degrees_to_radians(lat1))
             * std::cos(degrees_to_radians(lat2))
             * std::sin(dlon / 2.0) * std::sin(dlon / 2.0);
    return 6371000.0 * 2.0 * std::asin(std::sqrt(a));
}

static double bearing_degrees(double lat1, double lon1,
                              double lat2, double lon2) noexcept {
    double phi1 = degrees_to_radians(lat1);
    double phi2 = degrees_to_radians(lat2);
    double lambda1 = degrees_to_radians(lon1);
    double lambda2 = degrees_to_radians(lon2);
    double y = std::sin(lambda2 - lambda1) * std::cos(phi2);
    double x = std::cos(phi1) * std::sin(phi2)
             - std::sin(phi1) * std::cos(phi2) * std::cos(lambda2 - lambda1);
    double bearing = std::atan2(y, x);
    double degrees = bearing * (180.0 / PI);
    if (degrees < 0.0) degrees += 360.0;
    return degrees;
}

static double interpolate(double start, double end, double t) noexcept {
    return start + (end - start) * t;
}

export void taskSamplePosition(TaskContext& ctx) {
    if (ctx.current_route.empty()) {
        std::cout << "[taskSamplePosition] no route available" << std::endl;
        return;
    }

    std::cout << "[taskSamplePosition] route_size=" << ctx.current_route.size()
              << " current_node=" << ctx.vehicle.current_node
              << " current_pos=(" << ctx.vehicle.lat << "," << ctx.vehicle.lon << ")"
              << std::endl;

    auto pos = std::find(ctx.current_route.begin(),
                         ctx.current_route.end(),
                         ctx.vehicle.current_node);
    if (pos == ctx.current_route.end()) {
        std::cout << "[taskSamplePosition] current_node not on route, snapping to route front" << std::endl;
        ctx.vehicle.current_node = ctx.current_route.front();
        ctx.vehicle.lat = ctx.graph.node_lat(ctx.vehicle.current_node);
        ctx.vehicle.lon = ctx.graph.node_lon(ctx.vehicle.current_node);
        ctx.vehicle.heading = 0.0;
        ctx.vehicle.speed_ms = 0.0;
        ctx.current_waypoint_index = 0;
        return;
    }

    size_t index = static_cast<size_t>(std::distance(ctx.current_route.begin(), pos));
    if (index + 1 >= ctx.current_route.size()) {
        std::cout << "[taskSamplePosition] at route end, stopping" << std::endl;
        ctx.vehicle.speed_ms = 0.0;
        return;
    }

    double remaining_travel = DEFAULT_SPEED_MS * (static_cast<double>(TICK_MS) / 1000.0);
    ctx.vehicle.speed_ms = DEFAULT_SPEED_MS;

    double start_lat = ctx.vehicle.lat;
    double start_lon = ctx.vehicle.lon;

    while (remaining_travel > 1e-9 && index + 1 < ctx.current_route.size()) {
        uint32_t next_node = ctx.current_route[index + 1];
        double to_lat   = ctx.graph.node_lat(next_node);
        double to_lon   = ctx.graph.node_lon(next_node);
        double from_lat = ctx.vehicle.lat;
        double from_lon = ctx.vehicle.lon;

        double segment_distance = haversine_distance_m(from_lat, from_lon, to_lat, to_lon);
        if (segment_distance <= 1e-6) {
            ctx.vehicle.current_node = next_node;
            ctx.vehicle.lat = to_lat;
            ctx.vehicle.lon = to_lon;
            ctx.current_waypoint_index = index + 1;
            ++index;
            continue;
        }

        ctx.vehicle.heading = bearing_degrees(from_lat, from_lon, to_lat, to_lon);
        if (remaining_travel + 1e-9 >= segment_distance) {
            ctx.vehicle.current_node = next_node;
            ctx.vehicle.lat = to_lat;
            ctx.vehicle.lon = to_lon;
            ctx.current_waypoint_index = index + 1;
            remaining_travel -= segment_distance;
            ++index;
            continue;
        }

        double fraction = remaining_travel / segment_distance;
        ctx.vehicle.lat = interpolate(from_lat, to_lat, fraction);
        ctx.vehicle.lon = interpolate(from_lon, to_lon, fraction);
        remaining_travel = 0.0;
    }

    double moved = haversine_distance_m(start_lat, start_lon, ctx.vehicle.lat, ctx.vehicle.lon);
    std::cout << "[taskSamplePosition] moved=" << moved
              << "m new_pos=(" << ctx.vehicle.lat << "," << ctx.vehicle.lon << ")"
              << " current_node=" << ctx.vehicle.current_node << std::endl;

    if (index + 1 >= ctx.current_route.size()) {
        ctx.vehicle.speed_ms = 0.0;
    }

    // If we've reached a waypoint node, remove it from the remaining waypoint list
    auto it = std::find(ctx.waypoint_nodes.begin(), ctx.waypoint_nodes.end(), ctx.vehicle.current_node);
    if (it != ctx.waypoint_nodes.end()) {
        std::cout << "[taskSamplePosition] reached waypoint node " << ctx.vehicle.current_node << ", removing" << std::endl;
        ctx.waypoint_nodes.erase(std::remove(ctx.waypoint_nodes.begin(), ctx.waypoint_nodes.end(), ctx.vehicle.current_node), ctx.waypoint_nodes.end());
        // reset index tracker if needed
        ctx.current_waypoint_index = 0;
    }
}

export void taskNavigationState(TaskContext& ctx) {
    if (ctx.current_route.empty()) return;

    auto pos = std::find(ctx.current_route.begin(),
                         ctx.current_route.end(),
                         ctx.vehicle.current_node);
    if (pos == ctx.current_route.end()) return;

    size_t index = static_cast<size_t>(std::distance(ctx.current_route.begin(), pos));
    if (index + 1 >= ctx.current_route.size()) {
        std::cout << "[navigation] Destination reached" << std::endl;
        return;
    }

    double remaining_m = 0.0;
    for (size_t i = index; i + 1 < ctx.current_route.size(); ++i) {
        double lat1 = ctx.graph.node_lat(ctx.current_route[i]);
        double lon1 = ctx.graph.node_lon(ctx.current_route[i]);
        double lat2 = ctx.graph.node_lat(ctx.current_route[i + 1]);
        double lon2 = ctx.graph.node_lon(ctx.current_route[i + 1]);
        remaining_m += haversine_distance_m(lat1, lon1, lat2, lon2);
    }

    std::cout << "[navigation] current_node=" << ctx.vehicle.current_node
              << " next_node=" << ctx.current_route[index + 1]
              << " remaining_m=" << remaining_m << std::endl;
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

    remove_consecutive_duplicates(ctx.current_route);

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
    remove_consecutive_duplicates(ctx.current_route);

    // Rebuild route_edges.
    for (size_t i = 0; i + 1 < ctx.current_route.size(); ++i) {
        ctx.route_edges.insert({ctx.current_route[i], ctx.current_route[i + 1]});
    }

    ctx.replanning = false;  // allow future periodic checks

    std::cout << "[globalReplan] Done. Route: " << ctx.current_route.size()
              << " nodes, cost: " << total_cost
              << " start=" << ctx.current_route.front()
              << " end=" << ctx.current_route.back()
              << "\n";
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
    double current_cost_to_target = 0.0;
    bool found_target = false;
    for (auto it = pos; it + 1 != ctx.current_route.end(); ++it) {
        auto edge_idx = ctx.graph.find_edge_index(*it, *(it + 1));
        if (!edge_idx) {
            current_cost_to_target = std::numeric_limits<double>::infinity();
            break;
        }
        current_cost_to_target += ctx.graph.current_edge_weight(*edge_idx);
        if (wp_set.count(*(it + 1))) {
            next_target = *(it + 1);
            found_target = true;
            break;
        }
    }
    if (!found_target || current_cost_to_target == std::numeric_limits<double>::infinity()) {
        return;
    }

    // Cost of a fresh direct path to the next waypoint.
    auto fresh = a_star(ctx.graph, ctx.vehicle.current_node, next_target);
    if (fresh.path.empty()) return; // no alternative found

    if (fresh.cost < 0.8 * current_cost_to_target) {
        std::cout << "[periodicCheck] Significantly better route found (fresh: "
                  << fresh.cost << ", current: " << current_cost_to_target
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