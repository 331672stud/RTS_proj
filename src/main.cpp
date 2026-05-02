#include <thread>

import core.scheduler;
import app.context;
import app.tasks;
import system.config;

//to będzie trzeba zastąpić executable z QT


int main() {
    TaskContext ctx; //stan aplikacji
    Scheduler<TaskContext, MAX_TASKS, EVENT_QUEUE_SIZE> scheduler(ctx); //nasz organizator

    //tu powinniśmy wczytać dane startowe

    scheduler.addTask({taskSamplePosition, PRIORITY_HIGH, 10, 0}); //buduje tablice zadań
    scheduler.addTask({taskNavigationState, PRIORITY_MED, 50, 10});
    scheduler.addTask({taskLocalReplan, PRIORITY_LOW, 500, 20});
    scheduler.addTask({taskGlobalReplan, PRIORITY_LOW, 500, 20});
    scheduler.addTask({taskPeriodicRouteCheck, PRIORITY_LOW, 2000, 5});
    scheduler.addTask({taskWatchdog, PRIORITY_LOW, 5000, 15});

    std::thread tcp_thread(tcp_server_thread, 12345);
    tcp_thread.detach();  // or keep a handle to join later

    while (true) { //pętla działania
        scheduler.tick();
    }
}