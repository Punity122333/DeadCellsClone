#include "enemies/Automaton.hpp"
#include "Pathfinding.hpp"
#include "Player.hpp"
#include <raymath.h>
#include <cmath>
#include <thread>
#include <mutex>
#include <atomic>

namespace AutomatonConstants {
    constexpr float PlayerHitboxOffsetX = 4.8f;
    constexpr float PlayerHitboxOffsetY = 3.0f;
    constexpr float PlayerHitboxWidth = 33.6f;
    constexpr float PlayerHitboxHeight = 54.0f;
    constexpr int ProjectileDamage = 15;
}

Automaton::Automaton(Vector2 pos) 
    : position(pos), 
      velocity{0, 0}, 
      health(40),
      maxHealth(40),
      alive(true),
      invincibilityTimer(0.0f), 
      hitEffectTimer(0.0f), 
      currentColor(ORANGE),
      shootCooldown(0.0f),
      shootInterval(1.5f),
      pathfindingInProgress(false),
      pathReady(false),
      speed(100.0f) {
    printf("[Automaton] Constructed at (%.1f, %.1f)\n", pos.x, pos.y);
}

Automaton::Automaton(Automaton&& other) noexcept
    : position(other.position),
      velocity(other.velocity),
      health(other.health),
      maxHealth(other.maxHealth),
      alive(other.alive),
      invincibilityTimer(other.invincibilityTimer),
      hitEffectTimer(other.hitEffectTimer),
      currentColor(other.currentColor),
      shootCooldown(other.shootCooldown),
      shootInterval(other.shootInterval),
      projectiles(std::move(other.projectiles)),
      pathfindingInProgress(other.pathfindingInProgress.load()),
      pathReady(other.pathReady.load()),
      speed(other.speed) {
    std::lock_guard<std::mutex> lock(other.pathMutex);
    path = std::move(other.path);
}

Automaton& Automaton::operator=(Automaton&& other) noexcept {
    if (this != &other) {
        std::lock_guard<std::mutex> lock1(pathMutex);
        std::lock_guard<std::mutex> lock2(other.pathMutex);

        position = other.position;
        velocity = other.velocity;
        health = other.health;
        maxHealth = other.maxHealth;
        alive = other.alive;
        invincibilityTimer = other.invincibilityTimer;
        hitEffectTimer = other.hitEffectTimer;
        currentColor = other.currentColor;
        shootCooldown = other.shootCooldown;
        shootInterval = other.shootInterval;
        projectiles = std::move(other.projectiles);
        path = std::move(other.path);
        pathfindingInProgress = other.pathfindingInProgress.load();
        pathReady = other.pathReady.load();
    }
    return *this;
}

void Automaton::requestPathAsync(const Map& map, Vector2 start, Vector2 goal) {
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

void Automaton::checkProjectileCollisions(class Player& player) {
    if (!player.canTakeDamage()) return;
    
    Vector2 playerPos = player.getPosition();
    Rectangle playerHitbox = { 
        playerPos.x + AutomatonConstants::PlayerHitboxOffsetX,
        playerPos.y + AutomatonConstants::PlayerHitboxOffsetY,
        AutomatonConstants::PlayerHitboxWidth,
        AutomatonConstants::PlayerHitboxHeight
    };
    
    for (auto& projectile : projectiles) {
        if (!projectile.active) continue;
        
        Rectangle projectileHitbox = projectile.getHitbox();
        if (CheckCollisionRecs(playerHitbox, projectileHitbox)) {
            player.takeDamage(AutomatonConstants::ProjectileDamage);
            projectile.active = false;
            break; 
        }
    }
}
