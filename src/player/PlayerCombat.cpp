#include "Player.hpp"
#include "enemies/ScrapHound.hpp"
#include "weapons/WeaponTypes.hpp"

void Player::attack() {
    if (weapons.size() > 0 && currentWeaponIndex < weapons.size()) {
        weapons[currentWeaponIndex]->startAttack();
    }
}

bool Player::isAttacking() const {
    if (weapons.size() > 0 && currentWeaponIndex < weapons.size()) {
        return weapons[currentWeaponIndex]->isAttacking();
    }
    return false;
}

Rectangle Player::getWeaponHitbox() const {
    if (weapons.size() > 0 && currentWeaponIndex < weapons.size()) {
        return weapons[currentWeaponIndex]->getHitbox(position, facingRight);
    }
    return Rectangle{0, 0, 0, 0};
}

void Player::switchWeapon(int index) {
    if (index >= 0 && index < weapons.size()) {
        currentWeaponIndex = index;
    }
}

void Player::addWeapon(std::unique_ptr<Weapon> weapon) {
    weapons.push_back(std::move(weapon));
}

void Player::checkWeaponHits(std::vector<ScrapHound>& enemies) {
    if (!isAttacking()) return;
    if (weapons[currentWeaponIndex]->getType() == WeaponType::BOW) {
        dynamic_cast<Bow*>(weapons[currentWeaponIndex].get())->checkArrowCollisions(enemies);
        return;
    }
    Rectangle hitbox = getWeaponHitbox();
    for (auto& enemy : enemies) {
        if (!enemy.isAlive()) continue;
        Vector2 enemyPos = enemy.getPosition();
        Rectangle enemyRect = {
            enemyPos.x - 16.0f,
            enemyPos.y - 16.0f,
            32.0f,
            32.0f
        };
        if (CheckCollisionRecs(hitbox, enemyRect)) {
            float damage = weapons[currentWeaponIndex]->getDamage();
            enemy.takeDamage(damage);
            float knockbackForce = 100.0f;
            if (weapons[currentWeaponIndex]->getType() == WeaponType::SWORD) {
                knockbackForce = 150.0f;
            } else if (weapons[currentWeaponIndex]->getType() == WeaponType::SPEAR) {
                knockbackForce = 200.0f;
            }
            Vector2 knockbackDir = {
                facingRight ? 1.0f : -1.0f,
                -0.5f
            };
            Vector2 knockback = {
                knockbackDir.x * knockbackForce,
                knockbackDir.y * knockbackForce
            };
            enemy.applyKnockback(knockback);
        }
    }
}

Rectangle Player::getSwordHitbox() const {
    return getWeaponHitbox();
}

bool Player::isSwordAttacking() const {
    return isAttacking();
}