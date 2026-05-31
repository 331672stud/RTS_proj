module;

#include <iostream>
#include <ostream>

export module app.tasks;

import app.context;

export void taskSamplePosition(TaskContext& ctx) {
    // 1. Read GPS/odometry (simulate for now)
    // 2. Update ctx.vehicle position
    // 3. Find nearest node (ctx.graph.find_nearest_node)
    // 4. If distance to current route > threshold, push OffRouteDetected event
    std::cout << "Task sample position" << std::endl;
}

export void taskNavigationState(TaskContext& ctx) {
    // 1. From current position and current_route, compute:
    //    - distance to next waypoint
    //    - estimated time of arrival
    //    - next turn instruction
    // 2. Publish to UI (e.g., via callback)
    std::cout << "Task navigationState" << std::endl;
}

export void taskLocalReplan(TaskContext& ctx) {
    // Perform bounded A* search (radius ~2 km) from current position
    // to the next waypoint (ctx.waypoint_nodes[ctx.current_waypoint_idx]).
    // Update ctx.current_route and ctx.route_edges.
    std::cout << "Task localReplan" << std::endl;
}

export void taskGlobalReplan(TaskContext& ctx) {
    // Compute full route visiting all remaining waypoints in order.
    // Use A* with current edge weights (including overrides).
    // ctx.current_route = concatenation of paths from current node -> wp1 -> wp2 -> ... -> wpN
    // Update ctx.route_edges set.
    std::cout << "Task globalReplan" << std::endl;
}

export void taskPeriodicRouteCheck(TaskContext& ctx) {
    // Compute a fresh global route from current position to final destination.
    // If cost(new) < cost(current) * 0.8, push GlobalReplanRequest.
    std::cout << "Task periodic route check" << std::endl;
}

export void taskWatchdog(TaskContext& ctx) {
    // Check event queue depth, last replan timestamp, graph consistency.
    // Log warnings if thresholds exceeded.
    std::cout << "Task watchdog" << std::endl;
}