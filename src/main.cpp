import core.scheduler;
import app.context;
import app.tasks;
import system.config;

//to będzie trzeba zastąpić executable z QT

int main() {
    TaskContext ctx; //stan aplikacji
    Scheduler<TaskContext, MAX_TASKS, EVENT_QUEUE_SIZE> scheduler(ctx); //nasz organizator

    //tu powinniśmy wczytać dane startowe

    scheduler.addTask({taskValidate, PRIORITY_HIGH, 10, 0}); //buduje tablice zadań
    scheduler.addTask({taskLocalReplan, PRIORITY_MED, 50, 10});
    scheduler.addTask({taskGlobalReplan, PRIORITY_LOW, 500, 20});
    scheduler.addTask({taskUpdateState, PRIORITY_MED, 20, 5});

    while (true) { //pętla działania
        scheduler.tick();
    }
}