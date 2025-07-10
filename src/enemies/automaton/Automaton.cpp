#include "enemies/Automaton.hpp"
#include "Pathfinding.hpp"
#include "Player.hpp"
#include "Camera.hpp"
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
    : Enemy(pos, 40, 40, 100.0f),
      shootCooldown(0.0f),
      shootInterval(1.5f) {
    currentColor = ORANGE;
    printf("[Automaton] Constructed at (%.1f, %.1f)\n", pos.x, pos.y);
}

Automaton::Automaton(Automaton&& other) noexcept
    : Enemy(std::move(other)),
      shootCooldown(other.shootCooldown),
      shootInterval(other.shootInterval),
      projectiles(std::move(other.projectiles)) {
}

Automaton& Automaton::operator=(Automaton&& other) noexcept {
    if (this != &other) {
        Enemy::operator=(std::move(other));
        shootCooldown = other.shootCooldown;
        shootInterval = other.shootInterval;
        projectiles = std::move(other.projectiles);
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

void Automaton::checkProjectileCollisions(class Player& player, class GameCamera& camera) {
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

            camera.addScreenshake(0.4f, 0.25f);
            break; 
        }
    }
}

EnemySpawnConfig Automaton::getSpawnConfig() const {
    return {0.4f, 1, true};
}
