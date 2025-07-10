#include "enemies/ScrapHound.hpp"
#include "map/Map.hpp"
#include <raymath.h>
#include <cmath>
#include <mutex>


void ScrapHound::update(const Map& map, Vector2 playerPos, float dt) {
    velocity.y += gravity * dt;
    if (invincibilityTimer > 0.0f) {
        invincibilityTimer -= dt;
    }
    if (hitEffectTimer > 0.0f) {
        hitEffectTimer -= dt;
        currentColor = (int)(hitEffectTimer * 10) % 2 == 0 ? RED : WHITE;
    } else {
        currentColor = WHITE;
    }
    if (pounceCooldown > 0) {
        pounceCooldown -= dt;
    }
    if (meleeTimer > 0) {
        meleeTimer -= dt;
        if (meleeTimer <= 0) {
            isMeleeAttacking = false;
        }
    }
    
    float distanceToPlayer = Vector2Distance(position, playerPos);

    bool shouldPatrol = distanceToPlayer > playerDetectionRange;
    
    if (shouldPatrol && !isPatrolling && !isPouncing && !isMeleeAttacking && !isPounceCharging && !isMeleeCharging) {

        isPatrolling = true;
        {
            std::lock_guard<std::mutex> lock(pathMutex);
            path.clear();
        }
    } else if (!shouldPatrol && isPatrolling) {

        isPatrolling = false;
        patrolBoundsInitialized = false;
    }
    
    if (!isPouncing && !isMeleeAttacking && !isPounceCharging && distanceToPlayer < meleeTriggerDistance) {
        isMeleeCharging = true;
        meleeCharge += dt;
        if (meleeCharge >= meleeChargeTime) {
            isMeleeCharging = false;
            isMeleeAttacking = true;
            meleeTimer = meleeDuration;
            velocity.x = 0;
            velocity.y = 0;
            meleeCharge = 0.0f;
        }
    } else if (isMeleeCharging && distanceToPlayer > meleeTriggerDistance) {
        isMeleeCharging = false;
        meleeCharge = 0.0f;
    }
    else if (!isPouncing && !isMeleeAttacking && !isMeleeCharging && pounceCooldown <= 0 && distanceToPlayer < pounceTriggerDistance) {
        if (!isPounceCharging) {
            pounceAnimRadius = 64.0f;
            pounceAnimFade = 0.0f;
        }
        isPounceCharging = true;
        pounceCharge += dt;
        float progress = pounceCharge / pounceChargeTime;
        const float FADE_IN_RATIO = 0.2f;
        if (pounceCharge < pounceChargeTime * FADE_IN_RATIO) {
            pounceAnimFade = pounceCharge / (pounceChargeTime * FADE_IN_RATIO);
            pounceAnimRadius = 64.0f;
        } else {
            pounceAnimFade = 1.0f;
            float shrinkProgress = (pounceCharge - (pounceChargeTime * FADE_IN_RATIO)) / (pounceChargeTime * (1.0f - FADE_IN_RATIO));
            pounceAnimRadius = 64.0f - (62.0f * shrinkProgress);
            if (pounceAnimRadius < 2.0f) pounceAnimRadius = 2.0f;
        }
        if (pounceCharge >= pounceChargeTime) {
            isPounceCharging = false;
            isPouncing = true;
            pounceTimer = pounceDuration;
            pounceCooldown = pounceCooldownTime;
            float dir = (playerPos.x > position.x) ? 1.0f : -1.0f;
            velocity.x = dir * pounceForceX;
            velocity.y = pounceForceY;
            pounceCharge = 0.0f;
            pounceAnimRadius = 0.0f;
            pounceAnimFade = 0.0f;
        }
    } else if (isPounceCharging && distanceToPlayer > pounceTriggerDistance) {
        isPounceCharging = false;
        pounceCharge = 0.0f;
        pounceAnimRadius = 0.0f;
        pounceAnimFade = 0.0f;
    }
    if (isPouncing && fabsf(position.x - playerPos.x) < 32.0f && fabsf(position.y - playerPos.y) < 32.0f) {
        isPouncing = false;
        velocity.x = 0;
    }

    if (!isPouncing && !isMeleeAttacking && !isMeleeCharging && !isPounceCharging) {
        if (isPatrolling) {
            updatePatrol(map, dt);
        } else {

            static int pathTimer = 0;
            static Vector2 lastPlayerPos = {0, 0};
            float playerMoved = Vector2Distance(playerPos, lastPlayerPos);
            if ((pathTimer++ > 5 || path.empty() || playerMoved > 32.0f) && !pathfindingInProgress) {
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
                if (Vector2Distance(position, target) < 16.0f) {
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
        }
    }
    Vector2 nextPos = position;
    nextPos.y += velocity.y * dt;
    nextPos.x += velocity.x * dt;
    if (isPouncing) {
        bool collided = false;
        if (map.collidesWithGround({nextPos.x + 16, nextPos.y + 32})) {
            collided = true;
        }
        if (velocity.x < 0 && map.collidesWithGround({nextPos.x, nextPos.y + 16})) {
            collided = true;
        }
        if (velocity.x > 0 && map.collidesWithGround({nextPos.x + 32, nextPos.y + 16})) {
            collided = true;
        }
        if (collided) {
            isPouncing = false;
            velocity.x = 0;
            velocity.y = 0;
            nextPos = position;
        }
    }
    if (map.collidesWithGround({position.x + 16, nextPos.y + 32})) {
        velocity.y = 0;
        nextPos.y = ((int)((nextPos.y + 32) / 32)) * 32 - 32;
        if (isPouncing) {
            isPouncing = false;
        }
    }
    if (map.collidesWithGround({nextPos.x + 16, position.y + 32}) && !isPouncing) {
        velocity.x = 0;
    }
    position = nextPos;
}
