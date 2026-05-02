#include <iostream>
#include <thread>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

// Forward declarations – these must match your existing types
struct Event;
enum class EventType { GraphUpdate /* , others */ };
struct GraphUpdateData { uint64_t u, v; double new_weight; };

// Assume you have a global or accessible EventQueue (your scheduler's queue)
// For simplicity, we'll use a function pointer to push events.
// You can adapt this to your actual EventQueue API.
extern void push_event_to_queue(const Event& e);  // implement this to call scheduler.getEventQueue().push(e)

// ----------------------------------------------------------------------
// TCP server thread
// ----------------------------------------------------------------------
void tcp_server_thread(int port) {
    int server_fd, client_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[4096];
    std::string remaining;  // for partial line handling

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 1);

    std::cout << "[TCP] Listening on port " << port << std::endl;

    client_fd = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
    if (client_fd < 0) {
        perror("accept failed");
        close(server_fd);
        return;
    }
    std::cout << "[TCP] Simulator connected." << std::endl;

    while (true) {
        ssize_t bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) break;
        buffer[bytes] = '\0';
        remaining += buffer;

        // Split by newline
        size_t pos;
        while ((pos = remaining.find('\n')) != std::string::npos) {
            std::string line = remaining.substr(0, pos);
            remaining.erase(0, pos + 1);
            if (line.empty()) continue;

            // Parse JSON
            try {
                json msg = json::parse(line);
                std::string type = msg.value("type", "");

                if (type == "waypoints") {
                    // Extract coordinates array
                    auto coords = msg["coordinates"];
                    std::vector<std::pair<double, double>> waypoints;
                    for (auto& p : coords) {
                        double lat = p[0];
                        double lon = p[1];
                        waypoints.emplace_back(lat, lon);
                    }
                    // --- Action for your RTS: compute route from these waypoints ---
                    // This is not an event; it's a configuration command.
                    // You can call a function like "setTargetWaypoints(waypoints)" which
                    // triggers initial route planning.
                    std::cout << "[TCP] Received " << waypoints.size() << " waypoints." << std::endl;
                    // TODO: call your own function (e.g., computeInitialRoute(waypoints))
                }
                else if (type == "graph_update") {
                    auto edge = msg["edge"];
                    uint64_t u = edge[0];
                    uint64_t v = edge[1];
                    double new_weight = msg["new_weight"];

                    // Build an internal GraphUpdate event and push to queue
                    Event e;
                    e.type = EventType::GraphUpdate;
                    e.timestamp = Tick::now();  // use your core.time Tick
                    GraphUpdateData* data = new GraphUpdateData{u, v, new_weight};
                    e.data = data;  // will be deleted after dispatch (your dispatchEvent must handle ownership)

                    push_event_to_queue(e);
                    std::cout << "[TCP] Graph update: edge (" << u << "," << v << ") weight=" << new_weight << std::endl;
                }
                else {
                    std::cerr << "[TCP] Unknown message type: " << type << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << "[TCP] JSON parse error: " << e.what() << " on line: " << line << std::endl;
            }
        }
    }

    close(client_fd);
    close(server_fd);
    std::cout << "[TCP] Simulator disconnected." << std::endl;
}