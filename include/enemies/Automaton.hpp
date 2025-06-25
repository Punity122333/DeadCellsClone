#pragma once

#include <raylib.h>
#include <vector>
#include <atomic>
#include <mutex>
#include "map/Map.hpp"

class AutomatonProjectile {
public:
    Vector2 position;
    Vector2 velocity;
    bool active = true;
    float lifetime = 2.0f;
    float age = 0.0f;
    AutomatonProjectile(Vector2 pos, Vector2 vel) : position(pos), velocity(vel) {}
    void update(float dt);
    void draw() const;
    Rectangle getHitbox() const;
};

class Automaton {
public:

    Automaton(const Automaton&) = delete;
    Automaton& operator=(const Automaton&) = delete;

    Automaton(Automaton&&) noexcept;
    Automaton& operator=(Automaton&&) noexcept;
    
    Automaton(Vector2 pos);
    void update(const Map& map, Vector2 playerPos, float dt);
    void draw() const;
    void takeDamage(int amount);
    void applyKnockback(Vector2 force);
    bool isAlive() const { return alive; }
    bool canTakeDamage() const { return invincibilityTimer <= 0.0f; }
    Rectangle getHitbox() const;
    std::vector<AutomatonProjectile>& getProjectiles() { return projectiles; }
    void updateProjectiles(float dt);
    void checkProjectileCollisions(class Player& player);

    mutable std::mutex pathMutex;
    std::atomic<bool> pathfindingInProgress = false;
    std::atomic<bool> pathReady = false;
    void requestPathAsync(const Map& map, Vector2 start, Vector2 goal);
private:
    Vector2 position;
    Vector2 velocity;
    int health = 40;
    int maxHealth = 40;
    bool alive = true;
    float invincibilityTimer = 0.0f;
    float hitEffectTimer = 0.0f;
    Color currentColor = WHITE;
    float shootCooldown = 0.0f;
    float shootInterval = 1.5f;
    std::vector<AutomatonProjectile> projectiles;

    std::vector<Vector2> path;
    const float speed = 100.0f;
};
