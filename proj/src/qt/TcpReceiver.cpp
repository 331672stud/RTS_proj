#include "TcpReceiver.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <simdjson.h>

TcpReceiver::TcpReceiver(QObject *parent) : QObject(parent) {}

void TcpReceiver::run(int port)
{
    m_running = true;

    // ---------- socket setup (same as before) ----------
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        return;
    }
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        return;
    }
    if (listen(server_fd, 1) < 0) {
        perror("listen failed");
        close(server_fd);
        return;
    }
    std::cout << "[TCP] Listening on port " << port << std::endl;

    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) {
        perror("accept failed");
        close(server_fd);
        return;
    }
    std::cout << "[TCP] Client connected" << std::endl;
    // ----------------------------------------------------

    simdjson::dom::parser parser;
    char buffer[65536];
    std::string remaining;

    while (m_running) {
        ssize_t n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) {
            if (n < 0) perror("recv error");
            break;
        }
        buffer[n] = '\0';
        remaining += buffer;

        size_t pos;
        while ((pos = remaining.find('\n')) != std::string::npos) {
            std::string line = remaining.substr(0, pos);
            remaining.erase(0, pos + 1);
            if (line.empty()) continue;

            simdjson::dom::element doc;
            if (parser.parse(line).get(doc)) continue;

            std::string_view type;
            if (doc["type"].get_string().get(type)) continue;

            if (type == "waypoints") {
                // ---- Emit the raw JSON line ----
                emit waypointsJson(QString::fromStdString(line));
            }
            else if (type == "graph_update") {
                simdjson::dom::array edge;
                if (doc["edge"].get_array().get(edge)) continue;
                uint64_t u, v;
                double w;
                if (edge.at(0).get_uint64().get(u) ||
                    edge.at(1).get_uint64().get(v) ||
                    doc["new_weight"].get_double().get(w)) continue;

                emit graphUpdate(u, v, w);
            }
        }
    }

    close(client_fd);
    close(server_fd);
    std::cout << "[TCP] Disconnected" << std::endl;
}

void TcpReceiver::stop() {
    m_running = false;
}