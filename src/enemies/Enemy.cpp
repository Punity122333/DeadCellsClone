#include "enemies/Enemy.hpp"

Enemy::Enemy(Vector2 pos, int hp, int maxHp, float spd)
    : position(pos), velocity{0, 0}, health(static_cast<float>(hp)), 
      maxHealth(static_cast<float>(maxHp)), speed(spd) {}

Enemy::Enemy(Enemy&& other) noexcept
    : position(other.position),
      velocity(other.velocity),
      health(other.health),
      maxHealth(other.maxHealth),
      alive(other.alive),
      invincibilityTimer(other.invincibilityTimer),
      hitEffectTimer(other.hitEffectTimer),
      currentColor(other.currentColor),
      speed(other.speed),
      path(std::move(other.path)),
      pathfindingInProgress(other.pathfindingInProgress.load()),
      pathReady(other.pathReady.load()) {}

Enemy& Enemy::operator=(Enemy&& other) noexcept {
    if (this != &other) {
        position = other.position;
        velocity = other.velocity;
        health = other.health;
        maxHealth = other.maxHealth;
        alive = other.alive;
        invincibilityTimer = other.invincibilityTimer;
        hitEffectTimer = other.hitEffectTimer;
        currentColor = other.currentColor;
        speed = other.speed;
        path = std::move(other.path);
        pathfindingInProgress.store(other.pathfindingInProgress.load());
        pathReady.store(other.pathReady.load());
    }
    return *this;
}

void Enemy::takeDamage(int amount) {
    if (invincibilityTimer <= 0.0f) {
        health -= amount;
        invincibilityTimer = 0.5f;
        hitEffectTimer = 0.2f;
        if (health <= 0) {
            alive = false;
        }
    }
}

void Enemy::applyKnockback(Vector2 force) {
    velocity = force;
}
