#pragma once

#include <raylib.h>

#include "enemies/Enemy.hpp"

class Map;

class Detonode : public Enemy {
public:
    enum State {
        IDLE,
        PURSUING,
        APPROACHING,
        BLINKING,
        EXPLODING,
        DEAD
    };

    Detonode(const Detonode&) = delete;
    Detonode& operator=(const Detonode&) = delete;
    Detonode(Detonode&&) noexcept;
    Detonode& operator=(Detonode&&) noexcept;
    
    Detonode(Vector2 pos);
    void update(const Map& map, Vector2 playerPos, float dt) override {}
    void update(Map& map, Vector2 playerPos, float dt, class GameCamera& camera) override;
    void draw() const override;
    void applyKnockback(Vector2 force) override;
    Rectangle getHitbox() const override;
    EnemyType getType() const override { return EnemyType::DETONODE; }
    EnemySpawnConfig getSpawnConfig() const override;
    void requestPathAsync(const Map& map, Vector2 start, Vector2 goal) override;

private:
    State currentState = IDLE;
    float stateTimer = 0.0f;
    
    float bobOffset = 0.0f;
    float bobSpeed = 2.0f;
    float bobAmplitude = 8.0f;
    
    float blinkTimer = 0.0f;
    float blinkInterval = 1.0f;
    int blinkCount = 0;
    
    float explosionRadius = 128.0f;
    float detectionRange = 200.0f;
    float approachDistance = 64.0f;
    float blinkDuration = 3.0f;

    mutable bool cachedLineOfSightResult = false;
    mutable float lineOfSightTimer = 0.0f;
    mutable Vector2 lastPlayerPosForLOS;
    static constexpr float lineOfSightCacheTime = 0.5f;
    
    bool hasLineOfSight(const Map& map, Vector2 start, Vector2 end) const;
    void explode(Map& map, class GameCamera& camera);
    void createBlinkParticles();
    void createExplosionParticles();
    void removePlatformTiles(Map& map, Vector2 center, float radius);
};
