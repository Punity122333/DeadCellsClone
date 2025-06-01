#include "ScrapHound.hpp"
#include <algorithm>
#include <raymath.h>
#include <queue>
#include <unordered_map>
#include <cmath>

struct Node {
    int x, y;
    float g, f;
    Node* parent;
    Node(int x, int y, float g, float f, Node* parent) : x(x), y(y), g(g), f(f), parent(parent) {}
};

static float Heuristic(int x1, int y1, int x2, int y2) {
    return fabsf((float)x1 - x2) + fabsf((float)y1 - y2);
}

static std::vector<Vector2> FindPathAStar(const Map& map, Vector2 start, Vector2 goal) {
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
            int nx = current->x + d[0], ny = current->y + d[1];
            if (nx < 0 || ny < 0 || nx >= 500 || ny >= 300) continue;
            if (!map.isTileEmpty(nx, ny)) continue;
            if (!map.isSolidTile(nx, ny+1)) continue; // Only walk on ground
            float tentative_g = current->g + 1.0f;
            int nhash = hash(nx, ny);
            if (!gScore.count(nhash) || tentative_g < gScore[nhash]) {
                gScore[nhash] = tentative_g;
                float f = tentative_g + Heuristic(nx, ny, gx, gy);
                Node* neighbor = new Node(nx, ny, tentative_g, f, current);
                openSet.push({f, neighbor});
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

    // Clean up
    for (auto& kv : allNodes) delete kv.second;
    return path;
}

ScrapHound::ScrapHound(Vector2 pos) : position(pos), velocity{0, 0} {}

void ScrapHound::update(const Map& map, Vector2 playerPos, float dt) {
    // Gravity
    velocity.y += gravity * dt;

    // --- Pathfinding ---
    static int pathTimer = 0;
    static Vector2 lastPlayerPos = {0, 0};
    float playerMoved = Vector2Distance(playerPos, lastPlayerPos);
    // Only recalc path if player moved > 32px or every 30 frames, or path is empty
    if (pathTimer++ > 10 || path.empty() || playerMoved > 32.0f) {
        path = FindPathAStar(map, position, playerPos);
        pathTimer = 0;
        lastPlayerPos = playerPos;
    }

    Vector2 nextPos = position;
    nextPos.y += velocity.y * dt;

    // Simple ground collision
    if (map.collidesWithGround({position.x + 16, nextPos.y + 32})) {
        velocity.y = 0;
        nextPos.y = ((int)((nextPos.y + 32) / 32)) * 32 - 32;
    }

    // Move horizontally along path
    if (!path.empty()) {
        Vector2 target = path.front();
        if (Vector2Distance(position, target) < 8.0f) { // Increased threshold for smoother node popping
            path.erase(path.begin());
        }
        if (!path.empty()) {
            float dx = path.front().x - position.x;
            if (fabs(dx) > 2.0f) {
                float dir = dx > 0 ? 1.0f : -1.0f;
                velocity.x = dir * speed;
            } else {
                velocity.x = 0;
            }
        }
    } else {
        velocity.x = 0;
    }

    // Horizontal collision
    nextPos.x += velocity.x * dt;
    if (map.collidesWithGround({nextPos.x + 16, nextPos.y + 32})) {
        velocity.x = 0;
    }

    position = nextPos;
}

void ScrapHound::draw() const {
    DrawRectangle((int)position.x, (int)position.y, 32, 32, RED);
}

Vector2 ScrapHound::getPosition() const {
    return position;
}