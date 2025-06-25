#include "enemies/Automaton.hpp"
#include <raymath.h>
#include <cmath>
#include <mutex>

namespace AutomatonConstants {
    constexpr float ProjectileSpeed = 250.0f;
    constexpr int PathTimerThreshold = 30;
    constexpr float PlayerMovedThreshold = 64.0f;
    constexpr float PathTargetRadius = 16.0f;
    constexpr float Gravity = 800.0f;
    constexpr float GroundCollisionOffsetX = 16.0f;
    constexpr float GroundCollisionOffsetY = 32.0f;
    constexpr float TileSize = 32.0f;
}

void Automaton::update(const Map& map, Vector2 playerPos, float dt) {
    if (!alive) return;
    if (invincibilityTimer > 0.0f) invincibilityTimer -= dt;
    if (hitEffectTimer > 0.0f) {
        hitEffectTimer -= dt;
        currentColor = (int)(hitEffectTimer * 10) % 2 == 0 ? WHITE : ORANGE;
    } else {
        currentColor = ORANGE;
    }
    shootCooldown -= dt;
    if (shootCooldown <= 0.0f) {
        Vector2 dir = Vector2Subtract(playerPos, position);
        dir = Vector2Normalize(dir);
        projectiles.emplace_back(position, Vector2Scale(dir, AutomatonConstants::ProjectileSpeed));
        shootCooldown = shootInterval;
    }
    static int pathTimer = 0;
    static Vector2 lastPlayerPos = {0, 0};
    float distanceToPlayer = Vector2Distance(position, playerPos);
    float playerMoved = Vector2Distance(playerPos, lastPlayerPos);
    if ((pathTimer++ > AutomatonConstants::PathTimerThreshold || path.empty() || playerMoved > AutomatonConstants::PlayerMovedThreshold) && !pathfindingInProgress) {
        requestPathAsync(map, position, playerPos);
        pathTimer = 0;
        lastPlayerPos = playerPos;
    }
    if (pathReady) {
        std::lock_guard<std::mutex> lock(pathMutex);
        pathReady = false;
    }
    if (!path.empty()) {
        Vector2 target = path.front();
        if (Vector2Distance(position, target) < AutomatonConstants::PathTargetRadius) {
            std::lock_guard<std::mutex> lock(pathMutex);
            path.erase(path.begin());
        }
        if (!path.empty()) {
            float dx = path.front().x - position.x;
            float dir = 0.0f;
            if (dx > 0) {
                dir = 1.0f;
            } else if (dx < 0) {
                dir = -1.0f;
            }
            velocity.x = dir * speed;
        } else {
            velocity.x = 0;
        }
    } else {
        velocity.x = 0;
    }
    velocity.y += AutomatonConstants::Gravity * dt;
    Vector2 nextPos = position;
    nextPos.x += velocity.x * dt;
    nextPos.y += velocity.y * dt;
    if (map.collidesWithGround({nextPos.x + AutomatonConstants::GroundCollisionOffsetX, nextPos.y + AutomatonConstants::GroundCollisionOffsetY})) {
        velocity.y = 0;
        nextPos.y = ((int)((nextPos.y + AutomatonConstants::GroundCollisionOffsetY) / AutomatonConstants::TileSize)) * AutomatonConstants::TileSize - AutomatonConstants::TileSize;
    }
    if (map.collidesWithGround({nextPos.x + AutomatonConstants::GroundCollisionOffsetX, position.y + AutomatonConstants::GroundCollisionOffsetY})) {
        velocity.x = 0;
    }
    position = nextPos;
    updateProjectiles(dt);
}

void Automaton::updateProjectiles(float dt) {
    for (auto& proj : projectiles) {
        proj.update(dt);
    }
}

void AutomatonProjectile::update(float dt) {
    if (!active) return;
    position.x += velocity.x * dt;
    position.y += velocity.y * dt;
    age += dt;
    if (age > lifetime) active = false;
}
