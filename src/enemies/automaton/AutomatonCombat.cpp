#include "enemies/Automaton.hpp"

void Automaton::takeDamage(int amount) {
    if (invincibilityTimer <= 0.0f) {
        health -= amount;
        invincibilityTimer = 0.5f;
        hitEffectTimer = 0.2f;
        if (health <= 0) alive = false;
    }
}

void Automaton::applyKnockback(Vector2 force) {
    velocity = force;
}

Rectangle Automaton::getHitbox() const {
    return { position.x, position.y, 32, 32 };
}
