#include "enemies/Detonode.hpp"
#include "map/Map.hpp"
#include "Player.hpp"
#include "effects/ParticleSystem.hpp"
#include "Camera.hpp"
#include <raymath.h>

void Detonode::applyKnockback(Vector2 force) {
    velocity = Vector2Add(velocity, force);
}

void Detonode::explode(Map& map, class GameCamera& camera) {
    createExplosionParticles();
    removePlatformTiles(map, position, explosionRadius);
    

    Player* player = map.getPlayer();
    if (player) {
        Vector2 playerPos = player->getPosition();
        float distToPlayer = Vector2Distance(position, playerPos);
        
        if (distToPlayer <= explosionRadius) {

            int damage = 20 + static_cast<int>(30 * (1.0f - (distToPlayer / explosionRadius)));
            player->takeDamage(damage);

            float intensityMultiplier = 1.0f - (distToPlayer / explosionRadius);
            camera.addScreenshake(0.8f * intensityMultiplier, 0.5f);

            Vector2 knockbackDirection = Vector2Normalize(Vector2Subtract(playerPos, position));
            float knockbackForce = 400.0f * (1.0f - (distToPlayer / explosionRadius));
            Vector2 knockback = Vector2Scale(knockbackDirection, knockbackForce);
            player->applyKnockback(knockback);
        }
    }
}

void Detonode::createBlinkParticles() {
    Vector2 particlePos = position;
    particlePos.x += 16.0f;
    particlePos.y += 16.0f;

    ParticleSystem::getInstance().createExplosionParticles(particlePos, 3, YELLOW);
}

void Detonode::createExplosionParticles() {
    Vector2 particlePos = position;
    particlePos.x += 16.0f;
    particlePos.y += 16.0f;

    ParticleSystem::getInstance().createExplosionParticles(particlePos, 8, ORANGE);
    ParticleSystem::getInstance().createExplosion(particlePos, 5, RED, 2.0f, 150.0f);
}

void Detonode::removePlatformTiles(Map& map, Vector2 center, float radius) {
    int centerTileX = static_cast<int>(center.x / 32.0f);
    int centerTileY = static_cast<int>(center.y / 32.0f);
    int tileRadius = static_cast<int>(radius / 32.0f);
    int tileRadiusSq = tileRadius * tileRadius;

    const int maxRadius = std::min(tileRadius, 6); 

    for (int y = centerTileY - maxRadius; y <= centerTileY + maxRadius; ++y) {
        for (int x = centerTileX - maxRadius; x <= centerTileX + maxRadius; ++x) {
            if (!map.isInsideBounds(x, y)) continue;

            int dx = x - centerTileX;
            int dy = y - centerTileY;
            int distSq = dx * dx + dy * dy;
            
            if (distSq <= tileRadiusSq) {
                if (map.getTileValue(x, y) == MapConstants::PLATFORM_TILE_VALUE) {
                    map.setTileValue(x, y, MapConstants::EMPTY_TILE_VALUE);
                }
            }
        }
    }
}
