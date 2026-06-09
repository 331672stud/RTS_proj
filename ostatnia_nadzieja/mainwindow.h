#pragma once
#include <QWidget>
#include <QTimer>
#include "graph.h"

class MainWindow : public QWidget {
    Q_OBJECT
public:
    explicit MainWindow(int cols, int rows, int goalCount, QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;

private slots:
    void tick();

private:
    Graph graph;
    std::vector<int> goals;
    std::vector<bool> visited;

    int vehicleNode;
    QPointF vehiclePos;
    std::vector<int> travelPath;  // path to current goal
    int pathStep;
    double progress;
    bool done;

    std::vector<int> goalOrder;   // indices into goals[], remaining order
    int goalStep;

    // Full planned route: concatenation of Dijkstra paths through all remaining goals
    // Rebuilt whenever tour changes. Used only for drawing.
    std::vector<int> fullRoute;

    QTimer timer;
    static constexpr double BASE_SPEED = 0.05;

    QPointF nodeScreenPos(int idx) const;
    QPointF lerp(QPointF a, QPointF b, double t) const;
    void computeTour();
    void advanceToNextGoal();
    void buildPathToNextGoal();
    void rebuildFullRoute();      // rebuilds fullRoute from current state

    int margin = 60;
    mutable double hexScale = 1.0; // pixels per hex unit, computed in nodeScreenPos
};