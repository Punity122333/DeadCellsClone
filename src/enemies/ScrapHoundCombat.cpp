#include "enemies/ScrapHound.hpp"
#include <raymath.h>


void ScrapHound::takeDamage(int amount) {
    if (invincibilityTimer <= 0.0f) {
        health -= amount;
        invincibilityTimer = 0.5f;
        hitEffectTimer = 0.2f;
        if (health <= 0) {
            alive = false;
        }
    }
}

void ScrapHound::applyKnockback(Vector2 force) {
    velocity.x = force.x;
    velocity.y = force.y;
    isPouncing = false;
    isPounceCharging = false;
    isMeleeCharging = false;
    isMeleeAttacking = false;
    pounceAnimRadius = 0.0f;
    pounceAnimFade = 0.0f;
}
