#pragma once

#include <raylib.h>
#include <vector>

#include "map/Map.hpp"
#include "enemies/Enemy.hpp"

class AutomatonProjectile {
public:
    Vector2 position;
    Vector2 velocity;
    bool active = true;
    float lifetime = 2.0f;
    float age = 0.0f;
    AutomatonProjectile(Vector2 pos, Vector2 vel) : position(pos), velocity(vel) {}
    void update(float dt, const Map& map);
    void draw() const;
    Rectangle getHitbox() const;
};

class Automaton : public Enemy {
public:

    Automaton(const Automaton&) = delete;
    Automaton& operator=(const Automaton&) = delete;

    Automaton(Automaton&&) noexcept;
    Automaton& operator=(Automaton&&) noexcept;
    
    Automaton(Vector2 pos);
    void update(const Map& map, Vector2 playerPos, float dt) override;
    void draw() const override;
    Rectangle getHitbox() const override;
    EnemyType getType() const override { return EnemyType::AUTOMATON; }
    EnemySpawnConfig getSpawnConfig() const override;
    void requestPathAsync(const Map& map, Vector2 start, Vector2 goal) override;
    std::vector<AutomatonProjectile>& getProjectiles() { return projectiles; }
    void updateProjectiles(float dt, const Map& map);
    void checkProjectileCollisions(class Player& player, class GameCamera& camera);
    
    bool hasLineOfSight(const Map& map, Vector2 start, Vector2 end) const;
    
private:
    float shootCooldown = 0.0f;
    float shootInterval = 1.5f;
    std::vector<AutomatonProjectile> projectiles;
};
