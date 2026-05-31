import core.scheduler;
import app.context;
import app.tasks;
import system.config;
import socket.eventReceiver;
import system.nav_graph;

#include <iostream>

//to będzie trzeba zastąpić executable z QT

int main() {
    try
    {
        //NavGraph graph("/workspace/app/simScript/maps/Warsaw.nav"); //powinno działać w kontenerze
        NavGraph graph("../simScript/maps/Warsaw.nav"); //powinno działać natywnie
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    tcp_server_thread(12345);

    /*TaskContext ctx; //stan aplikacji
    Scheduler<TaskContext, MAX_TASKS, EVENT_QUEUE_SIZE> scheduler(ctx); //nasz organizator

    //tu powinniśmy wczytać dane startowe

    scheduler.addTask({taskSamplePosition, PRIORITY_HIGH, 10, 0}); //buduje tablice zadań
    scheduler.addTask({taskNavigationState, PRIORITY_MED, 50, 10});
    scheduler.addTask({taskLocalReplan, PRIORITY_LOW, 500, 20});
    scheduler.addTask({taskGlobalReplan, PRIORITY_LOW, 500, 20});
    scheduler.addTask({taskPeriodicRouteCheck, PRIORITY_LOW, 2000, 5});
    scheduler.addTask({taskWatchdog, PRIORITY_LOW, 5000, 15});

    while (true) { //pętla działania
        scheduler.tick();
    }*/

    return 0;
}