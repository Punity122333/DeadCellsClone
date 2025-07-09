#pragma once
#include <atomic>
#include <mutex>
#include <raylib.h>
#include <vector>

class Map;

class Detonode {
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
    void update(Map& map, Vector2 playerPos, float dt);
    void draw() const;
    void takeDamage(int amount);
    void applyKnockback(Vector2 force);
    bool isAlive() const { return alive; }
    bool canTakeDamage() const { return invincibilityTimer <= 0.0f; }
    Rectangle getHitbox() const;
    Vector2 getPosition() const;

    mutable std::mutex pathMutex;
    std::atomic<bool> pathfindingInProgress = false;
    std::atomic<bool> pathReady = false;
    void requestPathAsync(const Map& map, Vector2 start, Vector2 goal);

private:
    Vector2 position;
    Vector2 velocity;
    int health = 60;
    int maxHealth = 60;
    bool alive = true;
    float invincibilityTimer = 0.0f;
    float hitEffectTimer = 0.0f;
    Color currentColor = WHITE;
    
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
    float speed = 120.0f;
    float blinkDuration = 3.0f;
    
    // Performance optimization: line-of-sight caching
    mutable bool cachedLineOfSightResult = false;
    mutable float lineOfSightTimer = 0.0f;
    mutable Vector2 lastPlayerPosForLOS;
    static constexpr float lineOfSightCacheTime = 0.5f;
    
    std::vector<Vector2> path;
    
    bool hasLineOfSight(const Map& map, Vector2 start, Vector2 end) const;
    void explode(Map& map);
    void createBlinkParticles();
    void createExplosionParticles();
    void removePlatformTiles(Map& map, Vector2 center, float radius);
};
