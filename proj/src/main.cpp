import core.scheduler;
import app.context;
import app.tasks;
import system.config;
import socket.eventReceiver;
import system.nav_graph;
import core.time;

#include <iostream>
#include <thread>

int main() {
    NavGraph graph;
    try {
        NavGraph graph("../simScript/maps/Warsaw.nav");
        std::cout << "Loaded graph: " << graph.node_count() << " nodes, "
                  << graph.edge_count() << " edges.\n";
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    TaskContext ctx(std::move(graph));
    Scheduler<TaskContext, MAX_TASKS, EVENT_QUEUE_SIZE> scheduler(ctx);
    try {
        scheduler.addTask({taskSamplePosition,      PRIORITY_HIGH, 10,  0});   // period 10 ticks (~100ms), offset 0
        scheduler.addTask({taskNavigationState,     PRIORITY_HIGH, 50,  10});   // 500ms
        scheduler.addTask({taskLocalReplan,         PRIORITY_HIGH, 0,   0});   // one‑shot, triggered by events
        scheduler.addTask({taskGlobalReplan,        PRIORITY_HIGH, 0,   0});   // one‑shot
        scheduler.addTask({taskPeriodicRouteCheck,  PRIORITY_HIGH, 300, 5});   // 3000ms
        scheduler.addTask({taskWatchdog,            PRIORITY_HIGH, 500, 15});   // 5000ms
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
    }
    std::thread tcp_thread(tcp_server_thread, 12345);
    tcp_thread.detach();   // let it run independently

    while (true) {
        auto start = std::chrono::steady_clock::now();
        scheduler.tick();
        auto end = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        if (elapsed < TICK_MS) {
            std::this_thread::sleep_for(std::chrono::milliseconds(TICK_MS - elapsed));
        } else printf("Elapsed time: %ld ms\n", elapsed);
    }

    return 0;
}