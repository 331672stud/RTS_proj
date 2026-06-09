#include "mainwindow.h"
#include <QPainter>
#include <QMouseEvent>
#include <QFont>
#include <random>
#include <algorithm>
#include <numeric>
#include <limits>

MainWindow::MainWindow(int cols, int rows, int goalCount, QWidget* parent)
    : QWidget(parent), done(false), pathStep(0), progress(0.0), goalStep(0)
{
    setMinimumSize(640, 480);
    setWindowTitle("Symulator Połączeń");

    graph.build(cols, rows);
    int n = graph.nodes.size();

    std::mt19937 rng(std::random_device{}());
    std::vector<int> indices(n);
    std::iota(indices.begin(), indices.end(), 0);
    std::shuffle(indices.begin(), indices.end(), rng);
    goalCount = std::min(goalCount, n - 1);
    for (int i = 0; i < goalCount; ++i) {
        graph.nodes[indices[i]].isGoal = true;
        goals.push_back(indices[i]);
    }
    visited.assign(goalCount, false);

    vehicleNode = indices[goalCount];
    vehiclePos = QPointF(0, 0); // updated on first paint

    computeTour();
    buildPathToNextGoal();
    rebuildFullRoute();

    connect(&timer, &QTimer::timeout, this, &MainWindow::tick);
    timer.start(30);
}

// Hex nodes store logical positions; we scale to screen here.
QPointF MainWindow::nodeScreenPos(int idx) const {
    // Find bounding box of all node positions
    double minX = 1e9, maxX = -1e9, minY = 1e9, maxY = -1e9;
    for (auto& nd : graph.nodes) {
        minX = std::min(minX, nd.pos.x());
        maxX = std::max(maxX, nd.pos.x());
        minY = std::min(minY, nd.pos.y());
        maxY = std::max(maxY, nd.pos.y());
    }
    double rangeX = maxX - minX, rangeY = maxY - minY;
    double scaleX = rangeX > 0 ? (width()  - 2*margin) / rangeX : 1.0;
    double scaleY = rangeY > 0 ? (height() - 2*margin) / rangeY : 1.0;
    hexScale = std::min(scaleX, scaleY);

    QPointF p = graph.nodes[idx].pos;
    return QPointF(margin + (p.x() - minX) * hexScale,
                   margin + (p.y() - minY) * hexScale);
}

QPointF MainWindow::lerp(QPointF a, QPointF b, double t) const {
    return a + (b - a) * t;
}

// Nearest-neighbor TSP over remaining (unvisited) goals
void MainWindow::computeTour() {
    int m = goals.size();
    std::vector<bool> used(m, false);
    goalOrder.clear();
    int cur = vehicleNode;
    for (int step = 0; step < m; ++step) {
        double best = std::numeric_limits<double>::infinity();
        int bestGoal = -1;
        for (int g = 0; g < m; ++g) {
            if (used[g] || visited[g]) continue;
            std::vector<int> dummy;
            double d = graph.shortestPath(cur, goals[g], dummy);
            if (d < best) { best = d; bestGoal = g; }
        }
        if (bestGoal == -1) break;
        used[bestGoal] = true;
        goalOrder.push_back(bestGoal);
        cur = goals[bestGoal];
    }
    goalStep = 0;
}

void MainWindow::buildPathToNextGoal() {
    if (goalStep >= (int)goalOrder.size()) { done = true; return; }
    int target = goals[goalOrder[goalStep]];
    graph.shortestPath(vehicleNode, target, travelPath);
    pathStep = 1;
    progress = 0.0;
}

// Build fullRoute: from vehicleNode through all remaining goals in goalOrder
void MainWindow::rebuildFullRoute() {
    fullRoute.clear();
    if (goalOrder.empty()) return;

    int cur = vehicleNode;
    bool first = true;
    for (int gi = goalStep; gi < (int)goalOrder.size(); ++gi) {
        int target = goals[goalOrder[gi]];
        std::vector<int> seg;
        graph.shortestPath(cur, target, seg);
        if (first) {
            fullRoute = seg;
            first = false;
        } else {
            // Skip duplicate junction node
            fullRoute.insert(fullRoute.end(), seg.begin() + 1, seg.end());
        }
        cur = target;
    }
}

void MainWindow::tick() {
    if (done) { update(); return; }
    if (travelPath.size() < 2) { advanceToNextGoal(); return; }

    int segFrom = travelPath[pathStep - 1];
    int segTo   = travelPath[pathStep];

    int ei = graph.findEdge(segFrom, segTo);
    double weight = (ei >= 0) ? graph.nodes[segFrom].edges[ei].weight : 1.0;
    progress += BASE_SPEED / weight;

    if (progress >= 1.0) {
        progress = 0.0;
        vehicleNode = segTo;
        vehiclePos = nodeScreenPos(segTo);
        pathStep++;
        if (pathStep >= (int)travelPath.size())
            advanceToNextGoal();
    } else {
        vehiclePos = lerp(nodeScreenPos(segFrom), nodeScreenPos(segTo), progress);
    }
    update();
}

void MainWindow::advanceToNextGoal() {
    if (goalStep < (int)goalOrder.size()) {
        if (vehicleNode == goals[goalOrder[goalStep]]) {
            visited[goalOrder[goalStep]] = true;
            goalStep++;
        }
    }
    bool allVisited = true;
    for (bool v : visited) if (!v) { allVisited = false; break; }
    if (allVisited) { done = true; update(); return; }

    computeTour();
    if (goalOrder.empty()) { done = true; return; }
    buildPathToNextGoal();
    rebuildFullRoute();
}

void MainWindow::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(18, 20, 32));

    QFont smallFont; smallFont.setPointSize(7);
    p.setFont(smallFont);

    // --- Draw all edges ---
    for (int u = 0; u < (int)graph.nodes.size(); ++u) {
        QPointF a = nodeScreenPos(u);
        for (auto& e : graph.nodes[u].edges) {
            if (e.to <= u) continue;
            QPointF b = nodeScreenPos(e.to);
            p.setPen(QPen(QColor(55, 65, 85), 1));
            p.drawLine(a, b);
            QPointF mid = lerp(a, b, 0.5);
            p.setPen(QColor(100, 110, 130));
            p.drawText(mid + QPointF(2, -2), QString::number(e.weight, 'f', 1));
        }
    }

    // --- Draw full planned route as dashed coloured overlay ---
    if (!done && fullRoute.size() >= 2) {
        // Draw behind current-leg highlight, slightly transparent
        QPen routePen(QColor(80, 200, 255, 160), 2, Qt::DashLine);
        routePen.setDashPattern({4, 4});
        p.setPen(routePen);
        for (int s = 1; s < (int)fullRoute.size(); ++s) {
            QPointF a = nodeScreenPos(fullRoute[s-1]);
            QPointF b = nodeScreenPos(fullRoute[s]);
            p.drawLine(a, b);
        }

        // Solid highlight for current leg only
        if (pathStep > 0 && pathStep < (int)travelPath.size()) {
            p.setPen(QPen(QColor(255, 200, 50), 2.5));
            for (int s = pathStep; s < (int)travelPath.size(); ++s)
                p.drawLine(nodeScreenPos(travelPath[s-1]), nodeScreenPos(travelPath[s]));
        }
    }

    // --- Draw nodes ---
    for (int i = 0; i < (int)graph.nodes.size(); ++i) {
        QPointF pos = nodeScreenPos(i);
        bool isGoal = graph.nodes[i].isGoal;
        bool isVisited = false;
        for (int g = 0; g < (int)goals.size(); ++g)
            if (goals[g] == i && visited[g]) isVisited = true;

        QColor col = isVisited ? QColor(60, 180, 90)
                   : isGoal   ? QColor(220, 70, 70)
                              : QColor(60, 110, 190);
        p.setBrush(col);
        p.setPen(QPen(col.lighter(150), 1));
        p.drawEllipse(pos, 11, 11);
        p.setPen(Qt::white);
        p.setFont(smallFont);
        p.drawText(QRectF(pos.x()-11, pos.y()-11, 22, 22), Qt::AlignCenter, QString::number(i));
    }

    // --- Draw vehicle ---
    p.setBrush(QColor(255, 230, 0));
    p.setPen(QPen(Qt::white, 1.5));
    p.drawEllipse(vehiclePos, 9, 9);

    // --- Status ---
    QFont statusFont; statusFont.setPointSize(11); statusFont.setBold(true);
    p.setFont(statusFont);
    if (done) {
        p.setPen(QColor(80, 255, 120));
        p.drawText(rect(), Qt::AlignCenter, "Koniec");
    } else {
        int remaining = 0;
        for (bool v : visited) if (!v) remaining++;
        p.setPen(Qt::white);
        p.drawText(10, 22, QString("Pozostałych celi: %1").arg(remaining));
        if (goalStep < (int)goalOrder.size())
            p.drawText(10, 42, QString("Kolejny cel: Węzeł %1").arg(goals[goalOrder[goalStep]]));
    }

    p.setFont(smallFont);
    p.setPen(QColor(160, 170, 180));
    p.drawText(10, height() - 10, "Lewy przycisk = +1  |  Prawy przycisk = -1");
}

void MainWindow::mousePressEvent(QMouseEvent* ev) {
    if (done) return;
    QPointF click = ev->pos();
    double threshold = 12.0;
    int bestU = -1, bestEdge = -1;
    double bestDist = threshold;

    for (int u = 0; u < (int)graph.nodes.size(); ++u) {
        QPointF a = nodeScreenPos(u);
        for (int ei = 0; ei < (int)graph.nodes[u].edges.size(); ++ei) {
            int v = graph.nodes[u].edges[ei].to;
            if (v <= u) continue;
            QPointF b = nodeScreenPos(v);
            QPointF ab = b - a, ac = click - a;
            double len2 = ab.x()*ab.x() + ab.y()*ab.y();
            double t = len2 > 0 ? std::clamp((ab.x()*ac.x() + ab.y()*ac.y()) / len2, 0.0, 1.0) : 0.0;
            double d = QLineF(click, a + ab * t).length();
            if (d < bestDist) { bestDist = d; bestU = u; bestEdge = ei; }
        }
    }

    if (bestU < 0) return;
    int v = graph.nodes[bestU].edges[bestEdge].to;
    double delta = (ev->button() == Qt::LeftButton) ? 1.0 : -1.0;
    double& w = graph.nodes[bestU].edges[bestEdge].weight;
    w = std::clamp(w + delta, 1.0, 20.0);
    int ei2 = graph.findEdge(v, bestU);
    if (ei2 >= 0) graph.nodes[v].edges[ei2].weight = w;

    // Remember which edge the vehicle is currently traversing
    int savedFrom = -1, savedTo = -1;
    double savedProgress = 0.0;
    if (!done && pathStep > 0 && pathStep < (int)travelPath.size()) {
        savedFrom     = travelPath[pathStep - 1];
        savedTo       = travelPath[pathStep];
        savedProgress = progress;
    }

    // Recompute path from savedTo (where we'll be once current edge is done)
    // so the vehicle never snaps back mid-edge.
    if (savedFrom >= 0) {
        vehicleNode = savedTo; // plan route from the far end of current edge
        buildPathToNextGoal();
        rebuildFullRoute();
        vehicleNode = savedFrom; // restore logical position to edge start

        // Prepend savedFrom->savedTo so the vehicle finishes its current edge first
        travelPath.insert(travelPath.begin(), savedFrom);
        pathStep  = 1;
        progress  = savedProgress;
        vehiclePos = lerp(nodeScreenPos(savedFrom), nodeScreenPos(savedTo), savedProgress);
    } else {
        buildPathToNextGoal();
        rebuildFullRoute();
    }

    update();
}