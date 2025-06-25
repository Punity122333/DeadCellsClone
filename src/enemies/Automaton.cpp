#include "enemies/Automaton.hpp"
#include <raymath.h>
#include <cmath>

Automaton::Automaton(Vector2 pos) : position(pos) {}

void Automaton::update(const Map& map, Vector2 playerPos, float dt) {
    if (!alive) return;
    if (invincibilityTimer > 0.0f) invincibilityTimer -= dt;
    // Simple AI: face player, shoot at intervals
    shootCooldown -= dt;
    if (shootCooldown <= 0.0f) {
        Vector2 dir = Vector2Subtract(playerPos, position);
        dir = Vector2Normalize(dir);
        float speed = 250.0f;
        projectiles.emplace_back(position, Vector2Scale(dir, speed));
        shootCooldown = shootInterval;
    }
    // Gravity
    velocity.y += 800.0f * dt;
    position.y += velocity.y * dt;
    // Clamp to ground (simple)
    if (position.y > 600) position.y = 600;
    updateProjectiles(dt);
}

void Automaton::draw() const {
    if (!alive) return;
    DrawRectangle((int)position.x, (int)position.y, 32, 32, ORANGE);
    for (const auto& proj : projectiles) {
        if (proj.active) proj.draw();
    }
}

void Automaton::takeDamage(int amount) {
    if (invincibilityTimer <= 0.0f) {
        health -= amount;
        invincibilityTimer = 0.5f;
        if (health <= 0) alive = false;
    }
}

void Automaton::applyKnockback(Vector2 force) {
    velocity = force;
}

Rectangle Automaton::getHitbox() const {
    return { position.x, position.y, 32, 32 };
}

void AutomatonProjectile::update(float dt) {
    if (!active) return;
    position.x += velocity.x * dt;
    position.y += velocity.y * dt;
    age += dt;
    if (age > lifetime) active = false;
}

void AutomatonProjectile::draw() const {
    if (!active) return;
    DrawCircle((int)position.x + 8, (int)position.y + 8, 8, RED);
}

void Automaton::updateProjectiles(float dt) {
    for (auto& proj : projectiles) {
        proj.update(dt);
    }
}

void Automaton::checkProjectileCollisions(class Player& player) {
    // Implement collision with player if needed
}
