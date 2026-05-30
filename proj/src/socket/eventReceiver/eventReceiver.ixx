module;

#include <iostream>
#include <thread>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
//TO TRZEBA POBRAĆ
#include <simdjson.h>

export module socket.eventReceiver;

import core.event;
import core.time;
import core.queue;

// Forward declarations – adapt to your actual types
struct GraphUpdateData { uint64_t u, v; double new_weight; };

// ----------------------------------------------------------------------
// TCP server thread using simdjson
// ----------------------------------------------------------------------
void tcp_server_thread(int port) {
    int server_fd, client_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[65536];          // large buffer for incoming data
    std::string remaining;       // partial line storage

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        return;
    }
    listen(server_fd, 1);
    std::cout << "[TCP] Listening on port " << port << std::endl;

    client_fd = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
    if (client_fd < 0) {
        perror("accept failed");
        close(server_fd);
        return;
    }
    std::cout << "[TCP] Simulator connected." << std::endl;

    simdjson::ondemand::parser parser;   // reusable parser

    while (true) {
        ssize_t bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) break;
        buffer[bytes] = '\0';
        remaining += buffer;

        size_t pos;
        while ((pos = remaining.find('\n')) != std::string::npos) {
            std::string line = remaining.substr(0, pos);
            remaining.erase(0, pos + 1);
            if (line.empty()) continue;

            // Parse with simdjson
            auto doc = parser.iterate(line);   // on‑demand parsing
            if (doc.error()) {
                std::cerr << "[TCP] JSON parse error: " << doc.error() << std::endl;
                continue;
            }

            // Extract "type" field
            std::string_view type;
            auto type_error = doc["type"].get_string().get(type);
            if (type_error) {
                std::cerr << "[TCP] Missing or invalid 'type' field" << std::endl;
                continue;
            }

            if (type == "waypoints") {
                auto coords_array = doc["coordinates"];
                if (coords_array.error()) {
                    std::cerr << "[TCP] Missing 'coordinates' array" << std::endl;
                    continue;
                }
                std::vector<std::pair<double, double>> waypoints;
                for (auto point : coords_array) {
                    double lat, lon;
                    auto array = point.get_array();
                    if (array.at(0).get_double().get(lat) || array.at(1).get_double().get(lon)) {
                        std::cerr << "[TCP] Invalid coordinate format" << std::endl;
                        break;
                    }
                    waypoints.emplace_back(lat, lon);
                }
                std::cout << "[TCP] Received " << waypoints.size() << " waypoints." << std::endl;
                // TODO: call your route planning function (or push an internal event)
                // computeInitialRoute(waypoints);
            }
            else if (type == "graph_update") {
                auto edge_array = doc["edge"];
                auto weight_val = doc["new_weight"];
                if (edge_array.error() || weight_val.error()) {
                    std::cerr << "[TCP] Missing 'edge' or 'new_weight' field" << std::endl;
                    continue;
                }
                uint64_t u, v;
                double new_weight;
                if (edge_array.at(0).get_uint64().get(u) ||
                    edge_array.at(1).get_uint64().get(v) ||
                    weight_val.get_double().get(new_weight)) {
                    std::cerr << "[TCP] Invalid edge or weight format" << std::endl;
                    continue;
                }

                // Build your internal event
                Event e;
                e.type = EventType::GraphUpdate;
                e.timestamp = now();   // your clock
                auto* data = new GraphUpdateData{u, v, new_weight};
                e.data = data;
                //EventQueue.push(e);
                std::cout << "[TCP] Graph update: edge (" << u << "," << v << ") weight=" << new_weight << std::endl;
            }
            else {
                std::cerr << "[TCP] Unknown message type: " << type << std::endl;
            }
        }
    }

    close(client_fd);
    close(server_fd);
    std::cout << "[TCP] Simulator disconnected." << std::endl;
}