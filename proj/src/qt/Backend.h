#pragma once

#include <QObject>
#include <QVariantList>

class Backend : public QObject
{
    Q_OBJECT

public:
    explicit Backend(QObject *parent = nullptr);

    signals:
        void waypointsReceived(const QVariantList &points);
        void graphEdgeUpdated(
            uint64_t u,
            uint64_t v,
            double weight,
            double lat1,
            double lon1,
            double lat2,
            double lon2
        );
};