#pragma once
#include <atomic>
#include <mutex>
#include <raylib.h>
#include <vector>

class Map;

class ScrapHound {
public:
    ScrapHound(const ScrapHound&) = delete;
    ScrapHound& operator=(const ScrapHound&) = delete;
    ScrapHound(ScrapHound&&) noexcept;
    ScrapHound& operator=(ScrapHound&&) noexcept;
    ScrapHound(Vector2 pos);
    void update(const Map& map, Vector2 playerPos, float dt);
    void draw() const;
    Vector2 getPosition() const;
    mutable std::mutex pathMutex;
    std::atomic<bool> pathfindingInProgress = false;
    std::atomic<bool> pathReady = false;
    void requestPathAsync(const Map& map, Vector2 start, Vector2 goal);

private:
    Vector2 position;
    Vector2 velocity;

    std::vector<Vector2> path;

    const float gravity = 800.0f;
    const float speed = 100.0f;

    bool isPouncing;
    float pounceTimer;
    float pounceCooldown;

    float pounceTriggerDistance;
    float pounceForceX;
    float pounceForceY;
    float pounceDuration;
    float pounceCooldownTime;

    bool isMeleeAttacking;
    float meleeTimer;
    float meleeTriggerDistance;
    float meleeDuration;
};