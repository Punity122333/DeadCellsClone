#include "weapons/Weapon.hpp"

Weapon::Weapon(const std::string& name, WeaponType type, float baseDamage, float attackSpeed, float range)
    : name(name), type(type), baseDamage(baseDamage), attackSpeed(attackSpeed), range(range) {
}

void Weapon::update(float dt) {
    if (attacking) {
        attackTimer -= dt;
        if (attackTimer <= 0) {
            attacking = false;
        }
    }
}

void Weapon::draw(Vector2 playerPosition, bool facingRight) const {
    // Base implementation - override in derived classes
}

void Weapon::startAttack() {
    if (!attacking) {
        attacking = true;
        attackTimer = 1.0f / attackSpeed;
        comboCount = (comboCount + 1) % 3; // 3-hit combo system
    }
}

bool Weapon::isAttacking() const {
    return attacking;
}

Rectangle Weapon::getHitbox(Vector2 playerPosition, bool facingRight) const {
    // Default hitbox, override in specific weapons
    float offsetX = facingRight ? 16.0f : -16.0f - range;
    return Rectangle{
        playerPosition.x + offsetX,
        playerPosition.y - 8.0f,
        range,
        24.0f
    };
}

void Weapon::levelUp() {
    level++;
    damageMultiplier += 0.2f;  // 20% damage increase per level
}