#pragma once
#include <QObject>
#include <cstdint>
#include <QString>

class TcpReceiver : public QObject
{
    Q_OBJECT
public:
    explicit TcpReceiver(QObject *parent = nullptr);

public slots:
    void run(int port);
    void stop();

    signals:
        // Raw JSON line for waypoints (the backend will parse it)
        void waypointsJson(const QString &json);

    // Edge weight update – keep this for QML visualisation
    void graphUpdate(uint64_t u, uint64_t v, double newWeight);

private:
    bool m_running = false;
};