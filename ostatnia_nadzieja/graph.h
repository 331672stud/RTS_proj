#pragma once
#include <vector>
#include <QPointF>

struct Edge {
    int to;
    double weight; // 1..10
};

struct Node {
    QPointF pos;
    std::vector<Edge> edges;
    bool isGoal = false;
};

struct Graph {
    std::vector<Node> nodes;
    int cols, rows;

    void build(int cols, int rows);
    // Returns edge index in nodes[a].edges where .to == b, or -1
    int findEdge(int a, int b) const;
    // Dijkstra shortest path cost and path
    double shortestPath(int from, int to, std::vector<int>& path) const;
};