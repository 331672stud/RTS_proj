#include "qt/Backend.h"
#include "qt/TcpReceiver.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QThread>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;

    Backend backend;
    engine.rootContext()->setContextProperty("backend", &backend);

    TcpReceiver *receiver = new TcpReceiver;          // no parent yet
    QThread *netThread = new QThread;

    receiver->moveToThread(netThread);

    // Cross‑thread signal‑slot connections
    QObject::connect(receiver, &TcpReceiver::waypointsJson,
                     &backend, &Backend::onWaypointsJson);
    QObject::connect(receiver, &TcpReceiver::graphUpdate,
                     &backend, &Backend::onGraphEdgeUpdated);

    // --- Start server when the thread begins ---
    //     startServer() returns immediately; the event loop runs afterwards.
    QObject::connect(netThread, &QThread::started, receiver, [receiver]() {
        receiver->startServer(12345);
    });

    // --- Graceful shutdown ---
    // When the app quits, tell the receiver to stop.
    QObject::connect(&app, &QCoreApplication::aboutToQuit, receiver, [receiver]() {
        QMetaObject::invokeMethod(receiver, "stopServer", Qt::QueuedConnection);
    });
    // After stopServer() emits stopped(), quit the thread’s event loop.
    QObject::connect(receiver, &TcpReceiver::stopped,
                     netThread, &QThread::quit);
    // Clean up: receiver is already deleted by deleteLater() inside stopServer().
    // Delete the thread object when it finishes.
    QObject::connect(netThread, &QThread::finished,
                     netThread, &QObject::deleteLater);

    netThread->start();

    engine.loadFromModule("QTmap", "Main");
    if (engine.rootObjects().isEmpty())
        return -1;

    backend.startSimulation();
    return app.exec();
}