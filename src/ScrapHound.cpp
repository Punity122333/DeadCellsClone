#include "ScrapHound.hpp"
#include "Map.hpp"
#include <algorithm>
#include <raymath.h>
#include <queue>
#include <unordered_map>
#include <cmath>
#include <thread>
#include <mutex>
#include <atomic>

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
            int nx = current->x + d[0];
            int ny = current->y + d[1];

            if (nx < 0 || ny < 0 || nx >= 500 || ny >= 300) continue;
            if (!map.isTileEmpty(nx, ny)) continue;
            if (!map.isSolidTile(nx, ny + 1)) continue;

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

    for (auto& kv : allNodes) {
        delete kv.second;
    }
    return path;
}

ScrapHound::ScrapHound(Vector2 pos)
    : position(pos),
      velocity{0, 0},
      isPouncing(false),
      pounceTimer(0.0f),
      pounceCooldown(0.0f),
      pounceTriggerDistance(250.0f),
      pounceForceX(600.0f),
      pounceForceY(-215.0f),
      pounceDuration(0.3f),
      pounceCooldownTime(2.0f),
      isMeleeAttacking(false),
      meleeTimer(0.0f),
      meleeTriggerDistance(30.0f),
      meleeDuration(0.2f),
      pathfindingInProgress(false),
      pathReady(false),
      speed(150.0f),
      gravity(800.0f)
{}

ScrapHound::ScrapHound(ScrapHound&& other) noexcept
    : position(other.position),
      velocity(other.velocity),
      isPouncing(other.isPouncing),
      pounceTimer(other.pounceTimer),
      pounceCooldown(other.pounceCooldown),
      pounceTriggerDistance(other.pounceTriggerDistance),
      pounceForceX(other.pounceForceX),
      pounceForceY(other.pounceForceY),
      pounceDuration(other.pounceDuration),
      pounceCooldownTime(other.pounceCooldownTime),
      isMeleeAttacking(other.isMeleeAttacking),
      meleeTimer(other.meleeTimer),
      meleeTriggerDistance(other.meleeTriggerDistance),
      meleeDuration(other.meleeDuration),
      pathfindingInProgress(other.pathfindingInProgress.load()),
      pathReady(other.pathReady.load())
{
    std::lock_guard<std::mutex> lock(other.pathMutex);
    path = std::move(other.path);
}

ScrapHound& ScrapHound::operator=(ScrapHound&& other) noexcept {
    if (this != &other) {
        std::lock_guard<std::mutex> lock1(pathMutex);
        std::lock_guard<std::mutex> lock2(other.pathMutex);

        position = other.position;
        velocity = other.velocity;
        isPouncing = other.isPouncing;
        pounceTimer = other.pounceTimer;
        pounceCooldown = other.pounceCooldown;
        pounceTriggerDistance = other.pounceTriggerDistance;
        pounceForceX = other.pounceForceX;
        pounceForceY = other.pounceForceY;
        pounceDuration = other.pounceDuration;
        pounceCooldownTime = other.pounceCooldownTime;
        isMeleeAttacking = other.isMeleeAttacking;
        meleeTimer = other.meleeTimer;
        meleeTriggerDistance = other.meleeTriggerDistance;
        meleeDuration = other.meleeDuration;
        path = std::move(other.path);
        pathfindingInProgress = other.pathfindingInProgress.load();
        pathReady = other.pathReady.load();
    }
    return *this;
}

void ScrapHound::requestPathAsync(const Map& map, Vector2 start, Vector2 goal) {
    if (pathfindingInProgress) return;
    pathfindingInProgress = true;
    pathReady = false;

    std::thread([this, &map, start, goal]() {
        auto newPath = FindPathAStar(map, start, goal);
        {
            std::lock_guard<std::mutex> lock(pathMutex);
            path = std::move(newPath);
        }
        pathReady = true;
        pathfindingInProgress = false;
    }).detach();
}

void ScrapHound::update(const Map& map, Vector2 playerPos, float dt) {
    velocity.y += gravity * dt;

    if (pounceCooldown > 0) {
        pounceCooldown -= dt;
    }

    if (meleeTimer > 0) {
        meleeTimer -= dt;
        if (meleeTimer <= 0) {
            isMeleeAttacking = false;
        }
    }

    float distanceToPlayer = Vector2Distance(position, playerPos);

    if (!isPouncing && !isMeleeAttacking && distanceToPlayer < meleeTriggerDistance) {
        isMeleeAttacking = true;
        meleeTimer = meleeDuration;
        velocity.x = 0;
        velocity.y = 0;
    }
    else if (!isPouncing && !isMeleeAttacking && pounceCooldown <= 0 && distanceToPlayer < pounceTriggerDistance) {
        isPouncing = true;
        pounceTimer = pounceDuration;
        pounceCooldown = pounceCooldownTime;
        float dir = (playerPos.x > position.x) ? 1.0f : -1.0f;
        velocity.x = dir * pounceForceX;
        velocity.y = pounceForceY;
    }

    if (isPouncing && fabsf(position.x - playerPos.x) < 32.0f && fabsf(position.y - playerPos.y) < 32.0f) {
        isPouncing = false;
        velocity.x = 0;
    }

    if (!isPouncing && !isMeleeAttacking) {
        static int pathTimer = 0;
        static Vector2 lastPlayerPos = {0, 0};
        float playerMoved = Vector2Distance(playerPos, lastPlayerPos);

        if ((pathTimer++ > 5 || path.empty() || playerMoved > 32.0f) && !pathfindingInProgress) {
            requestPathAsync(map, position, playerPos);
            pathTimer = 0;
            lastPlayerPos = playerPos;
        }

        if (pathReady) {
            std::lock_guard<std::mutex> lock(pathMutex);
            pathReady = false;
        }

        if (!path.empty()) {
            Vector2 target = path.front();

            if (Vector2Distance(position, target) < 16.0f) {
                std::lock_guard<std::mutex> lock(pathMutex);
                path.erase(path.begin());
            }

            if (!path.empty()) {
                float dx = path.front().x - position.x;
                float dir = 0.0f;
                if (dx > 0) {
                    dir = 1.0f;
                } else if (dx < 0) {
                    dir = -1.0f;
                }
                velocity.x = dir * speed;
            } else {
                velocity.x = 0;
            }
        } else {
            velocity.x = 0;
        }
    }

    Vector2 nextPos = position;
    nextPos.y += velocity.y * dt;
    nextPos.x += velocity.x * dt;

    if (isPouncing) {
        bool collided = false;
        if (map.collidesWithGround({nextPos.x + 16, nextPos.y + 32})) {
            collided = true;
        }
        if (velocity.x < 0 && map.collidesWithGround({nextPos.x, nextPos.y + 16})) {
            collided = true;
        }
        if (velocity.x > 0 && map.collidesWithGround({nextPos.x + 32, nextPos.y + 16})) {
            collided = true;
        }
        if (collided) {
            isPouncing = false;
            velocity.x = 0;
            velocity.y = 0;
            nextPos = position;
        }
    }

    if (map.collidesWithGround({position.x + 16, nextPos.y + 32})) {
        velocity.y = 0;
        nextPos.y = ((int)((nextPos.y + 32) / 32)) * 32 - 32;
        if (isPouncing) {
            isPouncing = false;
        }
    }

    if (map.collidesWithGround({nextPos.x + 16, position.y + 32}) && !isPouncing) {
        velocity.x = 0;
    }

    position = nextPos;
}

void ScrapHound::draw() const {
    if (isMeleeAttacking) {
        DrawRectangle((int)position.x, (int)position.y, 32, 32, GREEN);
    } else if (isPouncing) {
        DrawRectangle((int)position.x, (int)position.y, 32, 32, ORANGE);
    } else {
        DrawRectangle((int)position.x, (int)position.y, 32, 32, RED);
    }
}

Vector2 ScrapHound::getPosition() const {
    return position;
}
