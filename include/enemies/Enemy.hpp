#pragma once
#include <raylib.h>
#include <atomic>
#include <mutex>
#include <vector>

class Map;
class GameCamera;

enum class EnemyType {
    SCRAP_HOUND,
    AUTOMATON,
    DETONODE
};

struct EnemySpawnConfig {
    float spawnChance;
    int maxPerRoom;
    bool requiresMinDistance;
};

class Enemy {
public:
    Enemy(Vector2 pos, int hp, int maxHp, float spd);
    virtual ~Enemy() = default;

    Enemy(const Enemy&) = delete;
    Enemy& operator=(const Enemy&) = delete;
    Enemy(Enemy&&) noexcept;
    Enemy& operator=(Enemy&&) noexcept;

    virtual void update(const Map& map, Vector2 playerPos, float dt) = 0;
    virtual void update(Map& map, Vector2 playerPos, float dt, GameCamera& camera) {}
    virtual void draw() const = 0;
    virtual void takeDamage(int amount);
    virtual void applyKnockback(Vector2 force);
    virtual Rectangle getHitbox() const = 0;
    virtual EnemyType getType() const = 0;
    virtual EnemySpawnConfig getSpawnConfig() const = 0;

    bool isAlive() const { return alive; }
    bool canTakeDamage() const { return invincibilityTimer <= 0.0f; }
    Vector2 getPosition() const { return position; }
    float getHealth() const { return health; }
    float getMaxHealth() const { return maxHealth; }

    mutable std::mutex pathMutex;
    std::atomic<bool> pathfindingInProgress = false;
    std::atomic<bool> pathReady = false;
    virtual void requestPathAsync(const Map& map, Vector2 start, Vector2 goal) = 0;

protected:
    Vector2 position;
    Vector2 velocity;
    float health;
    float maxHealth;
    bool alive = true;
    float invincibilityTimer = 0.0f;
    float hitEffectTimer = 0.0f;
    Color currentColor = WHITE;
    float speed;
    std::vector<Vector2> path;
};
