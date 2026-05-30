module;

#include <iostream>
#include <thread>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <vector>
#include <utility>
#include <simdjson.h>

export module socket.eventReceiver;

// ----------------------------------------------------------------------
// TCP server thread – czyta jsona na porcie i na razie tylko printuje
// ----------------------------------------------------------------------
export void tcp_server_thread(int port) {
    int server_fd, client_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[65536];
    std::string remaining;

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

    simdjson::dom::parser parser;

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

            // Parse JSON line
            simdjson::dom::element doc;
            auto error = parser.parse(line).get(doc);
            if (error) {
                std::cerr << "[TCP] JSON parse error: " << error << std::endl;
                continue;
            }

            // Get "type" field
            std::string_view type;
            auto type_error = doc["type"].get_string().get(type);
            if (type_error) {
                std::cerr << "[TCP] Missing or invalid 'type' field" << std::endl;
                continue;
            }

            if (type == "waypoints") {
                simdjson::dom::array coords_array;
                auto arr_err = doc["coordinates"].get_array().get(coords_array);
                if (arr_err) {
                    std::cerr << "[TCP] Missing 'coordinates' array" << std::endl;
                    continue;
                }
                std::vector<std::pair<double, double>> waypoints;
                for (auto point : coords_array) {
                    double lat, lon;
                    simdjson::dom::array point_arr;
                    if (point.get_array().get(point_arr)) continue;
                    if (point_arr.at(0).get_double().get(lat) || point_arr.at(1).get_double().get(lon)) {
                        std::cerr << "[TCP] Invalid coordinate format, skipping point" << std::endl;
                        continue;
                    }
                    waypoints.emplace_back(lat, lon);
                }
                std::cout << "[TCP] Waypoints (" << waypoints.size() << " points):";
                for (auto& [lat, lon] : waypoints) {
                    std::cout << " (" << lat << "," << lon << ")";
                }
                std::cout << std::endl;
            }
            else if (type == "graph_update") {
                // Extract edge array
                simdjson::dom::array edge_arr;
                if (doc["edge"].get_array().get(edge_arr)) {
                    std::cerr << "[TCP] Missing or invalid 'edge' array" << std::endl;
                    continue;
                }
                uint64_t u, v;
                if (edge_arr.at(0).get_uint64().get(u) || edge_arr.at(1).get_uint64().get(v)) {
                    std::cerr << "[TCP] Invalid edge format (u, v not uint64)" << std::endl;
                    continue;
                }
                double new_weight;
                if (doc["new_weight"].get_double().get(new_weight)) {
                    std::cerr << "[TCP] Invalid or missing 'new_weight'" << std::endl;
                    continue;
                }
                std::cout << "[TCP] Graph update: edge (" << u << "," << v << ") new weight = " << new_weight << std::endl;
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