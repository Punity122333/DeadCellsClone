#include "Player.hpp"
#include "enemies/ScrapHound.hpp"

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
    if (weapons.empty() || currentWeaponIndex >= weapons.size()) {
        return;
    }

    Weapon* currentWeapon = weapons[currentWeaponIndex].get();
    if (!currentWeapon || !currentWeapon->isAttacking()) {
        return;
    }

    Rectangle weaponHitbox = currentWeapon->getHitbox(position, facingRight);
    if (weaponHitbox.width == 0 || weaponHitbox.height == 0) {
        return;
    }

    for (auto& enemy : enemies) {
        if (enemy.isAlive()) {
            Rectangle enemyHitbox = enemy.getArrowHitbox();
            if (CheckCollisionRecs(weaponHitbox, enemyHitbox)) {
                if (enemy.canTakeDamage()) {
                    enemy.takeDamage(currentWeapon->getDamage());
                    enemy.applyKnockback(currentWeapon->getKnockback(facingRight));
                }
            }
        }
    }
}

Rectangle Player::getSwordHitbox() const {
    return getWeaponHitbox();
}

bool Player::isSwordAttacking() const {
    return isAttacking();
}