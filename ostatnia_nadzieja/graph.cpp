#include "graph.h"
#include <cmath>
#include <random>
#include <queue>
#include <limits>

void Graph::build(int c, int r) {
    cols = c; rows = r;
    nodes.clear();
    nodes.resize(c * r);

    std::mt19937 rng(42);
    std::uniform_real_distribution<double> wDist(1.0, 10.0);

    // Hex grid positions (offset rows)
    // Even rows: x = col, odd rows: x = col + 0.5
    for (int row = 0; row < r; ++row)
        for (int col = 0; col < c; ++col) {
            double xOffset = (row % 2 == 1) ? 0.5 : 0.0;
            nodes[row * c + col].pos = QPointF(col + xOffset, row * 0.866); // 0.866 = sqrt(3)/2
        }

    auto idx = [&](int col, int row) -> int {
        if (col < 0 || col >= c || row < 0 || row >= r) return -1;
        return row * c + col;
    };
    auto addEdge = [&](int i, int j) {
        if (i < 0 || j < 0) return;
        // Avoid duplicate edges
        for (auto& e : nodes[i].edges) if (e.to == j) return;
        double w = std::round(wDist(rng) * 10) / 10.0;
        nodes[i].edges.push_back({j, w});
        nodes[j].edges.push_back({i, w});
    };

    for (int row = 0; row < r; ++row) {
        for (int col = 0; col < c; ++col) {
            int i = idx(col, row);
            // Right neighbor
            addEdge(i, idx(col + 1, row));
            // Two lower neighbors depend on even/odd row
            if (row % 2 == 0) {
                addEdge(i, idx(col,     row + 1));
                addEdge(i, idx(col - 1, row + 1));
            } else {
                addEdge(i, idx(col,     row + 1));
                addEdge(i, idx(col + 1, row + 1));
            }
        }
    }
}

int Graph::findEdge(int a, int b) const {
    for (int i = 0; i < (int)nodes[a].edges.size(); ++i)
        if (nodes[a].edges[i].to == b) return i;
    return -1;
}

double Graph::shortestPath(int from, int to, std::vector<int>& path) const {
    int n = nodes.size();
    std::vector<double> dist(n, std::numeric_limits<double>::infinity());
    std::vector<int> prev(n, -1);
    using P = std::pair<double,int>;
    std::priority_queue<P, std::vector<P>, std::greater<P>> pq;
    dist[from] = 0;
    pq.push({0, from});
    while (!pq.empty()) {
        auto [d, u] = pq.top(); pq.pop();
        if (d > dist[u]) continue;
        for (auto& e : nodes[u].edges) {
            double nd = dist[u] + e.weight;
            if (nd < dist[e.to]) {
                dist[e.to] = nd;
                prev[e.to] = u;
                pq.push({nd, e.to});
            }
        }
    }
    path.clear();
    for (int cur = to; cur != -1; cur = prev[cur])
        path.push_back(cur);
    std::reverse(path.begin(), path.end());
    return dist[to];
}