#include "EventReceiver.h"

#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <simdjson.h>

EventReceiver::EventReceiver(QObject *parent)
    : QObject(parent)
{}

void EventReceiver::run(int port)
{
    m_running = true;

    int server_fd, client_fd;
    sockaddr_in address{};
    socklen_t addrlen = sizeof(address);

    char buffer[65536];
    std::string remaining;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        return;
    }

    listen(server_fd, 1);

    std::cout << "[TCP] listening\n";

    client_fd = accept(server_fd, (sockaddr*)&address, &addrlen);

    std::cout << "[TCP] client connected\n";

    simdjson::dom::parser parser;

    while (m_running)
    {
        ssize_t bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) break;

        buffer[bytes] = '\0';
        remaining += buffer;

        size_t pos;

        while ((pos = remaining.find('\n')) != std::string::npos)
        {
            std::string line = remaining.substr(0, pos);
            remaining.erase(0, pos + 1);

            if (line.empty()) continue;

            simdjson::dom::element doc;
            if (parser.parse(line).get(doc))
                continue;

            std::string_view type;
            if (doc["type"].get_string().get(type))
                continue;

            if (type == "waypoints")
            {
                simdjson::dom::array arr;
                if (doc["coordinates"].get_array().get(arr))
                    continue;

                QVariantList list;

                for (auto p : arr)
                {
                    simdjson::dom::array a;
                    double lat, lon;

                    if (p.get_array().get(a))
                        continue;

                    if (a.at(0).get_double().get(lat) ||
                        a.at(1).get_double().get(lon))
                        continue;

                    QVariantMap m;
                    m["lat"] = lat;
                    m["lon"] = lon;
                    list.append(m);
                }

                emit waypointsReceived(list);
            }

            else if (type == "graph_update")
            {
                simdjson::dom::array arr;

                if (doc["edge"].get_array().get(arr))
                    continue;

                uint64_t u, v;
                double w;

                if (arr.at(0).get_uint64().get(u) ||
                    arr.at(1).get_uint64().get(v) ||
                    doc["new_weight"].get_double().get(w))
                {
                    continue;
                }

                simdjson::dom::array geometry;
                if (doc["geometry"].get_array().get(geometry))
                {
                    continue;
                }

                double lat1, lon1, lat2, lon2;

                simdjson::dom::array p1;
                simdjson::dom::array p2;

                if (geometry.at(0).get_array().get(p1) ||
                    geometry.at(1).get_array().get(p2))
                {
                    continue;
                }

                if (p1.at(0).get_double().get(lat1) ||
                    p1.at(1).get_double().get(lon1) ||
                    p2.at(0).get_double().get(lat2) ||
                    p2.at(1).get_double().get(lon2))
                {
                    continue;
                }

                emit graphUpdate(
                                u,
                                v,
                                w,
                                lat1,
                                lon1,
                                lat2,
                                lon2
                            );
            }
        }
    }

    close(client_fd);
    close(server_fd);
}

void EventReceiver::stop()
{
    m_running = false;
}