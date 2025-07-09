#include "enemies/Detonode.hpp"
#include "Pathfinding.hpp"
#include "map/Map.hpp"
#include "core/GlobalThreadPool.hpp"
#include <raymath.h>
#include <cmath>
#include <mutex>

using namespace MapConstants;

bool Detonode::hasLineOfSight(const Map& map, Vector2 start, Vector2 end) const {
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
        if (map.isSolidTile(x, y)) {
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

void Detonode::update(Map& map, Vector2 playerPos, float dt) {
    if (!alive) return;

    // Distance-based update optimization - skip detailed updates if too far from player
    float distanceToPlayer = Vector2Distance(position, playerPos);
    if (distanceToPlayer > detectionRange * 3.0f) {
        // Minimal updates for far-away Detonodes
        if (hitEffectTimer > 0.0f) {
            hitEffectTimer -= dt;
        }
        if (invincibilityTimer > 0.0f) {
            invincibilityTimer -= dt;
        }
        bobOffset += bobSpeed * dt;
        return;
    }

    if (invincibilityTimer > 0.0f) {
        invincibilityTimer -= dt;
    }

    if (hitEffectTimer > 0.0f) {
        hitEffectTimer -= dt;
        currentColor = RED;
    } else {
        currentColor = WHITE;
    }

    bobOffset += bobSpeed * dt;
    Vector2 renderPos = position;
    renderPos.y += sin(bobOffset) * bobAmplitude;

    // Optimized line-of-sight check with caching
    bool playerVisible;
    lineOfSightTimer -= dt;
    
    // Use cached result if timer is still valid and player hasn't moved too much
    if (lineOfSightTimer > 0.0f && Vector2Distance(playerPos, lastPlayerPosForLOS) < 32.0f) {
        playerVisible = cachedLineOfSightResult;
    } else {
        // Recalculate and cache the result
        playerVisible = hasLineOfSight(map, position, playerPos);
        cachedLineOfSightResult = playerVisible;
        lineOfSightTimer = lineOfSightCacheTime;
        lastPlayerPosForLOS = playerPos;
    }

    switch (currentState) {
        case IDLE:
            if (playerVisible && distanceToPlayer <= detectionRange) {
                currentState = PURSUING;
                stateTimer = 0.0f;
                
                // Immediately start rushing towards the player
                Vector2 direction = Vector2Normalize(Vector2Subtract(playerPos, position));
                velocity.x = direction.x * speed * 1.2f;
                velocity.y = direction.y * speed * 1.2f;
            }
            break;

        case PURSUING:
            if (!playerVisible || distanceToPlayer > detectionRange * 1.5f) {
                currentState = IDLE;
                stateTimer = 0.0f;
                break;
            }

            if (distanceToPlayer <= approachDistance) {
                currentState = APPROACHING;
                stateTimer = 0.0f;
                break;
            }

            if (pathReady) {
                std::lock_guard<std::mutex> lock(pathMutex);
                if (!path.empty()) {
                    Vector2 targetPos = path[0];
                    Vector2 direction = Vector2Normalize(Vector2Subtract(targetPos, position));
                    velocity.x = direction.x * speed;
                    velocity.y = direction.y * speed;

                    if (Vector2Distance(position, targetPos) < 16.0f) {
                        path.erase(path.begin());
                    }
                } else {
                    velocity = {0, 0};
                }
                pathReady = false;
            }

            // Reduced pathfinding frequency - now every 3 seconds instead of 1
            if (!pathfindingInProgress && stateTimer > 3.0f) {
                // Skip pathfinding if player is visible and use direct approach
                if (playerVisible && distanceToPlayer < detectionRange) {
                    Vector2 direction = Vector2Normalize(Vector2Subtract(playerPos, position));
                    velocity.x = direction.x * speed;
                    velocity.y = direction.y * speed;
                } else {
                    requestPathAsync(map, position, playerPos);
                }
                stateTimer = 0.0f;
            }
            break;

        case APPROACHING:
            {
                Vector2 direction = Vector2Normalize(Vector2Subtract(playerPos, position));
                velocity.x = direction.x * speed * 1.5f;
                velocity.y = direction.y * speed * 1.5f;

                if (distanceToPlayer <= 48.0f) {
                    currentState = BLINKING;
                    stateTimer = 0.0f;
                    blinkTimer = 0.0f;
                    blinkCount = 0;
                    velocity = {0, 0};
                }
            }
            break;

        case BLINKING:
            stateTimer += dt;
            blinkTimer += dt;

            if (blinkTimer >= blinkInterval) {
                blinkTimer = 0.0f;
                blinkCount++;
                blinkInterval *= 0.8f;
                createBlinkParticles();
            }

            if (stateTimer >= blinkDuration) {
                currentState = EXPLODING;
                stateTimer = 0.0f;
            }
            break;

        case EXPLODING:
            explode(map);
            alive = false;
            break;

        case DEAD:
            break;
    }

    if (currentState != BLINKING && currentState != EXPLODING && currentState != DEAD) {
        Vector2 nextPos = Vector2Add(position, Vector2Scale(velocity, dt));

        int tileX = static_cast<int>(nextPos.x / 32.0f);
        int tileY = static_cast<int>(nextPos.y / 32.0f);

        if (!map.isSolidTile(tileX, tileY)) {
            position = nextPos;
        } else {
            velocity = {0, 0};
        }
    }

    stateTimer += dt;
}

void Detonode::requestPathAsync(const Map& map, Vector2 start, Vector2 goal) {
    if (pathfindingInProgress) return;
    pathfindingInProgress = true;
    pathReady = false;

    GlobalThreadPool::getInstance().getMainPool().enqueue([this, &map, start, goal]() {
        auto newPath = FindPathAStar(map, start, goal);
        {
            std::lock_guard<std::mutex> lock(pathMutex);
            path = std::move(newPath);
        }
        pathReady = true;
        pathfindingInProgress = false;
    });
}
