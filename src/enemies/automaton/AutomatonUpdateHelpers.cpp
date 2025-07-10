#include "enemies/Automaton.hpp"
#include <raymath.h>
#include <cmath>
#include <mutex>

using namespace MapConstants;

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

        if (hasLineOfSight(map, position, playerPos)) {
            Vector2 dir = Vector2Subtract(playerPos, position);
            dir = Vector2Normalize(dir);
            projectiles.emplace_back(position, Vector2Scale(dir, AutomatonConstants::ProjectileSpeed));
        }
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
    updateProjectiles(dt, map);
}

void Automaton::updateProjectiles(float dt, const Map& map) {
    for (auto& proj : projectiles) {
        proj.update(dt, map);
    }
}

void AutomatonProjectile::update(float dt, const Map& map) {
    if (!active) return;
    
    position.x += velocity.x * dt;
    position.y += velocity.y * dt;
    age += dt;

    int tileX = (int)(position.x / 32);
    int tileY = (int)(position.y / 32);
    if (map.isSolidTile(tileX, tileY)) {
        active = false;
        return;
    }
    
    if (age > lifetime) active = false;
}

bool Automaton::hasLineOfSight(const Map& map, Vector2 start, Vector2 end) const {

    int startX = static_cast<int>(start.x / 32.0f);
    int startY = static_cast<int>(start.y / 32.0f);
    int endX = static_cast<int>(end.x / 32.0f);
    int endY = static_cast<int>(end.y / 32.0f);

    int dx = abs(endX - startX);
    int dy = abs(endY - startY);
    int x = startX;
    int y = startY;
    int n = 1 + dx + dy;
    int x_inc = (endX > startX) ? 1 : -1;
    int y_inc = (endY > startY) ? 1 : -1;
    int error = dx - dy;

    dx *= 2;
    dy *= 2;

    for (; n > 0; --n) {

        int tileValue = map.getTileValue(x, y);
        if (tileValue == WALL_TILE_VALUE || 
            tileValue == TILE_HIGHLIGHT_DELETE) {
            return false;
        }

        if (error > 0) {
            x += x_inc;
            error -= dy;
        } else {
            y += y_inc;
            error += dx;
        }
    }

    return true;
}
