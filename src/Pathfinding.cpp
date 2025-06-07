#include "Pathfinding.hpp"
#include "map/Map.hpp"
#include <raymath.h>
#include <algorithm>
#include <queue>
#include <unordered_map>
#include <cmath>

static float Heuristic(int x1, int y1, int x2, int y2) {
    return fabsf((float)x1 - x2) + fabsf((float)y1 - y2);
}

std::vector<Vector2> FindPathAStar(const Map& map, Vector2 start, Vector2 goal) {
    int sx = (int)((start.x + 16) / 32);
    int sy = (int)((start.y + 32) / 32);
    int gx = (int)((goal.x + 16) / 32);
    int gy = (int)((goal.y + 32) / 32);

    auto hash = [](int x, int y) { return x * 10000 + y; };

    std::priority_queue<
        std::pair<float, Node*>,
        std::vector<std::pair<float, Node*>>,
        std::greater<>
    > openSet;

    std::unordered_map<int, Node*> allNodes;
    std::unordered_map<int, float> gScore;

    Node* startNode = new Node(sx, sy, 0.0f, Heuristic(sx, sy, gx, gy), nullptr);
    openSet.push({startNode->f, startNode});
    allNodes[hash(sx, sy)] = startNode;
    gScore[hash(sx, sy)] = 0.0f;

    int dirs[4][2] = { {1,0}, {-1,0}, {0,1}, {0,-1} };

    Node* endNode = nullptr;

    while (!openSet.empty()) {
        Node* current = openSet.top().second;
        openSet.pop();

        if (current->x == gx && current->y == gy) {
            endNode = current;
            break;
        }

        for (auto& d : dirs) {
            int nx = current->x + d[0];
            int ny = current->y + d[1];

            if (nx < 0 || ny < 0 || nx >= 500 || ny >= 300) continue;
            if (!map.isTileEmpty(nx, ny)) continue;
            if (!map.isSolidTile(nx, ny + 1)) continue;

            float tentative_g = current->g + 1.0f;
            int nhash = hash(nx, ny);

            if (!gScore.count(nhash) || tentative_g < gScore[nhash]) {
                gScore[nhash] = tentative_g;
                Node* neighbor = new Node(nx, ny, tentative_g, Heuristic(nx, ny, gx, gy), current);
                openSet.push({neighbor->f, neighbor});
                allNodes[nhash] = neighbor;
            }
        }
    }

    std::vector<Vector2> path;
    if (endNode) {
        Node* n = endNode;
        while (n && !(n->x == sx && n->y == sy)) {
            path.push_back(Vector2{n->x * 32.0f, n->y * 32.0f});
            n = n->parent;
        }
        std::reverse(path.begin(), path.end());
    }

    for (auto& kv : allNodes) {
        delete kv.second;
    }
    return path;
}
