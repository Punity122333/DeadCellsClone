#include "Player.hpp"

#include "weapons/Weapon.hpp"
#include "weapons/WeaponTypes.hpp"
#include "enemies/EnemyManager.hpp"
#include "effects/ParticleSystem.hpp"
#include "core/GlobalThreadPool.hpp"
#include <raylib.h>
#include <vector>
#include <memory>
#include <thread>
#include <future>
#include <mutex>


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

void Player::checkWeaponHits(EnemyManager& enemyManager) {
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

    auto enemies = enemyManager.getAllEnemies();
    std::mutex enemyMutex;
    size_t numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 2;

    size_t enemyChunk = (enemies.size() + numThreads - 1) / numThreads;
    std::vector<std::future<void>> enemyFutures;
    for (size_t t = 0; t < numThreads; ++t) {
        size_t start = t * enemyChunk;
        size_t end = std::min(start + enemyChunk, enemies.size());
        if (start >= end) continue;
        enemyFutures.push_back(GlobalThreadPool::getInstance().getMainPool().enqueue([&, start, end]() {
            for (size_t i = start; i < end; ++i) {
                auto* enemy = enemies[i];
                if (enemy->isAlive()) {
                    Rectangle enemyHitbox = enemy->getHitbox();
                    if (enemy->getType() == EnemyType::SCRAP_HOUND) {
                        ScrapHound* scrapHound = static_cast<ScrapHound*>(enemy);
                        enemyHitbox = scrapHound->getArrowHitbox();
                    }
                    if (CheckCollisionRecs(weaponHitbox, enemyHitbox)) {
                        if (enemy->canTakeDamage()) {
                            std::lock_guard<std::mutex> lock(enemyMutex);
                            enemy->takeDamage(currentWeapon->getDamage());
                            enemy->applyKnockback(currentWeapon->getKnockback(facingRight));
                            
                            Vector2 hitPos = {enemyHitbox.x + enemyHitbox.width/2, enemyHitbox.y + enemyHitbox.height/2};
                            Color particleColor = RED;
                            if (enemy->getType() == EnemyType::AUTOMATON) {
                                particleColor = ORANGE;
                            } else if (enemy->getType() == EnemyType::DETONODE) {
                                particleColor = YELLOW;
                            }
                            ParticleSystem::getInstance().createExplosionParticles(hitPos, 6, particleColor);
                        }
                    }
                }
            }
        }));
    }
    for (auto& f : enemyFutures) {
        if (f.valid()) f.get();
    }
}

Rectangle Player::getSwordHitbox() const {
    return getWeaponHitbox();
}

bool Player::isSwordAttacking() const {
    return isAttacking();
}