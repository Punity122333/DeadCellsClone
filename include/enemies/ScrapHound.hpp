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
    float getHealth() const { return health; }
    float getMaxHealth() const { return maxHealth; }

    
    bool isAlive() const { return alive; }
    void takeDamage(int amount);
    void applyKnockback(Vector2 force);

    mutable std::mutex pathMutex;
    std::atomic<bool> pathfindingInProgress = false;
    std::atomic<bool> pathReady = false;
    void requestPathAsync(const Map& map, Vector2 start, Vector2 goal);

private:
    float health = 100.0f;
    float maxHealth = 100.0f;
    Vector2 position;
    Vector2 velocity;
    bool alive = true;
    float invincibilityTimer = 0.0f;
    float hitEffectTimer = 0.0f;
    Color currentColor = WHITE;

    std::vector<Vector2> path;

    const float gravity = 800.0f;
    const float speed = 100.0f;

    bool isPouncing;
    bool isPounceCharging = false;
    float pounceCharge = 0.0f;
    float pounceTimer;
    float pounceCooldown;

    float pounceTriggerDistance;
    float pounceChargeTime = 0.5f;
    float pounceForceX;
    float pounceForceY;
    float pounceDuration;
    float pounceCooldownTime;

    bool isMeleeCharging = false;
    bool isMeleeAttacking;
    float meleeCharge = 0.0f;
    float meleeTimer;
    float meleeTriggerDistance;
    float meleeChargeTime = 0.3f;
    float meleeDuration;

    float pounceAnimRadius = 0.0f;
    float pounceAnimFade = 0.0f;
};