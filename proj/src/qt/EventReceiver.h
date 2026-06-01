#pragma once

#include <QObject>
#include <QVariantList>

class EventReceiver : public QObject
{
    Q_OBJECT

public:
    explicit EventReceiver(QObject *parent = nullptr);

    void run(int port);
    void stop();

    signals:
        void waypointsReceived(const QVariantList &points);
        void graphUpdate(
            uint64_t u,
            uint64_t v,
            double weight,
            double lat1,
            double lon1,
            double lat2,
            double lon2
        );

private:
    bool m_running = false;
};