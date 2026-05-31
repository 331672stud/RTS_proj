// import core.scheduler;
// import app.context;
// import app.tasks;
// import system.config;
// import socket.eventReceiver;
//
// //to będzie trzeba zastąpić executable z QT
//
// int main() {
//
//     tcp_server_thread(12345);
//
//     /*TaskContext ctx; //stan aplikacji
//     Scheduler<TaskContext, MAX_TASKS, EVENT_QUEUE_SIZE> scheduler(ctx); //nasz organizator
//
//     //tu powinniśmy wczytać dane startowe
//
//     scheduler.addTask({taskSamplePosition, PRIORITY_HIGH, 10, 0}); //buduje tablice zadań
//     scheduler.addTask({taskNavigationState, PRIORITY_MED, 50, 10});
//     scheduler.addTask({taskLocalReplan, PRIORITY_LOW, 500, 20});
//     scheduler.addTask({taskGlobalReplan, PRIORITY_LOW, 500, 20});
//     scheduler.addTask({taskPeriodicRouteCheck, PRIORITY_LOW, 2000, 5});
//     scheduler.addTask({taskWatchdog, PRIORITY_LOW, 5000, 15});
//
//     while (true) { //pętla działania
//         scheduler.tick();
//     }*/
//
//     return 0;
// }

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QThread>

#include "qt/Backend.h"
#include "qt/EventReceiver.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    Backend backend;
    engine.rootContext()->setContextProperty("backend", &backend);

    EventReceiver *receiver = new EventReceiver;
    QThread *thread = new QThread;

    receiver->moveToThread(thread);

    QObject::connect(receiver, &EventReceiver::waypointsReceived,
                     &backend, &Backend::waypointsReceived);

    QObject::connect(receiver, &EventReceiver::graphUpdate,
                     &backend, &Backend::graphEdgeUpdated);

    QObject::connect(thread, &QThread::started,
                     receiver, [=]() {
        receiver->run(12345);
    });

    QObject::connect(&app, &QCoreApplication::aboutToQuit,
                     receiver, &EventReceiver::stop);

    QObject::connect(&app, &QCoreApplication::aboutToQuit,
                     thread, &QThread::quit);

    QObject::connect(thread, &QThread::finished,
                     receiver, &QObject::deleteLater);

    engine.loadFromModule("QTmap", "Main");

    if (engine.rootObjects().isEmpty())
        return -1;

    thread->start();

    return app.exec();
}
