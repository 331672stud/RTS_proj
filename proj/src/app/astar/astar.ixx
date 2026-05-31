module;

#include <cstdint>
#include <cmath>
#include <vector>
#include <unordered_map>
#include <queue>
#include <algorithm>

export module app.astar;

import system.nav_graph;

static constexpr double VEHICLE_SPEED_MS = 14;

double distance_between(const NavGraph& graph, uint32_t a, uint32_t b) {
    double dx = graph.node_lon(a) - graph.node_lon(b);
    double dy = graph.node_lat(a) - graph.node_lat(b);
    // Convert degrees to meters (approximate: 111320 m per degree)
    const double METER_PER_DEG = 111320.0;
    return std::sqrt(dx*dx + dy*dy) * METER_PER_DEG;
}

// A* search returning (path_nodes, total_cost)
struct AStarResult {
    std::vector<uint32_t> path;
    double cost = std::numeric_limits<double>::infinity();
};

export AStarResult a_star(const NavGraph& graph, uint32_t start, uint32_t goal,
                   double max_cost = std::numeric_limits<double>::infinity(),
                   double max_distance_m = std::numeric_limits<double>::infinity()) {
    struct NodeState {
        uint32_t id;
        double g;     // cost from start
        double f;     // g + heuristic
        uint32_t parent;
        bool operator>(const NodeState& other) const { return f > other.f; }
    };
    std::unordered_map<uint32_t, NodeState> best;
    std::priority_queue<NodeState, std::vector<NodeState>, std::greater<>> open;

    best[start] = {start, 0.0, distance_between(graph, start, goal), 0};
    open.push(best[start]);

    while (!open.empty()) {
        NodeState cur = open.top(); open.pop();
        if (cur.id == goal) {
            // Reconstruct path
            std::vector<uint32_t> path;
            uint32_t n = cur.id;
            while (n != 0) {
                path.push_back(n);
                n = best[n].parent;
            }
            std::reverse(path.begin(), path.end());
            return {path, cur.g};
        }
        if (cur.g > max_cost) continue;
        // Check distance from start to goal in meters – if exceeded, stop expansion? Actually we check after expansion.
        // For local replan, we may prune if node is too far from start.
        if (distance_between(graph, start, cur.id) > max_distance_m) continue;

        // Iterate over outgoing edges
        uint32_t offset_start = graph.edge_offset(cur.id);
        uint32_t offset_end = graph.edge_offset(cur.id + 1);
        for (uint32_t i = offset_start; i < offset_end; ++i) {
            uint32_t next = graph.edge_target(i);
            double edge_weight = graph.current_edge_weight(i);
            double new_g = cur.g + edge_weight;
            auto it = best.find(next);
            if (it == best.end() || new_g < it->second.g) {
                double h = distance_between(graph, next, goal) / VEHICLE_SPEED_MS; // time heuristic
                NodeState ns = {next, new_g, new_g + h, cur.id};
                best[next] = ns;
                open.push(ns);
            }
        }
    }
    return {}; // no path found
}

// Simple nearest‑neighbor TSP ordering.
// Returns ordered list of nodes: start_node, then waypoints in heuristic order, then back to start_node.
export std::vector<uint32_t> order_waypoints_nearest_neighbor(const NavGraph& graph,
                                                       uint32_t start,
                                                       const std::vector<uint32_t>& waypoints) {
    if (waypoints.empty()) return {start, start};
    std::vector<uint32_t> remaining = waypoints;
    std::vector<uint32_t> ordered;
    ordered.reserve(remaining.size() + 2);
    ordered.push_back(start);
    uint32_t current = start;
    while (!remaining.empty()) {
        // find closest remaining waypoint to current
        auto it = std::min_element(remaining.begin(), remaining.end(),
            [&](uint32_t a, uint32_t b) {
                return distance_between(graph, current, a) < distance_between(graph, current, b);
            });
        ordered.push_back(*it);
        current = *it;
        remaining.erase(it);
    }
    ordered.push_back(start); // return to start
    return ordered;
}