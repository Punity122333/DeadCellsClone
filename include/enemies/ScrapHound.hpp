#pragma once
#include <raylib.h>
#include "enemies/Enemy.hpp"

class Map;

class ScrapHound : public Enemy {
public:
    ScrapHound(const ScrapHound&) = delete;
    ScrapHound& operator=(const ScrapHound&) = delete;
    ScrapHound(ScrapHound&&) noexcept;
    ScrapHound& operator=(ScrapHound&&) noexcept;

    ScrapHound(Vector2 pos);
    void update(const Map& map, Vector2 playerPos, float dt) override;
    void draw() const override;
    void takeDamage(int amount) override;
    void applyKnockback(Vector2 force) override;
    Rectangle getHitbox() const override;
    EnemyType getType() const override { return EnemyType::SCRAP_HOUND; }
    EnemySpawnConfig getSpawnConfig() const override;
    void requestPathAsync(const Map& map, Vector2 start, Vector2 goal) override;

    Rectangle getArrowHitbox() const;
    bool canTakeDamage() const;

private:
    // Patrol system
    bool isPatrolling = false;
    float patrolLeftBound = 0.0f;
    float patrolRightBound = 0.0f;
    int patrolDirection = 1; // 1 for right, -1 for left
    float playerDetectionRange = 350.0f;
    bool patrolBoundsInitialized = false;
    
    void initializePatrolBounds(const Map& map);
    void updatePatrol(const Map& map, float dt);
    float findPlatformLeftEdge(const Map& map, float startX, float y) const;
    float findPlatformRightEdge(const Map& map, float startX, float y) const;

    const float gravity = 800.0f;

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