#include "Player.hpp"

#include "weapons/Weapon.hpp"
#include "weapons/WeaponTypes.hpp"
#include "enemies/ScrapHound.hpp"
#include "enemies/Automaton.hpp"
#include "effects/ParticleSystem.hpp"
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

void Player::checkWeaponHits(std::vector<ScrapHound>& enemies, std::vector<Automaton>& automatons) {
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

    std::mutex enemyMutex;
    std::mutex automatonMutex;
    size_t numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 2;

    size_t enemyChunk = (enemies.size() + numThreads - 1) / numThreads;
    std::vector<std::future<void>> enemyFutures;
    for (size_t t = 0; t < numThreads; ++t) {
        size_t start = t * enemyChunk;
        size_t end = std::min(start + enemyChunk, enemies.size());
        if (start >= end) continue;
        enemyFutures.push_back(std::async(std::launch::async, [&, start, end]() {
            for (size_t i = start; i < end; ++i) {
                auto& enemy = enemies[i];
                if (enemy.isAlive()) {
                    Rectangle enemyHitbox = enemy.getArrowHitbox();
                    if (CheckCollisionRecs(weaponHitbox, enemyHitbox)) {
                        if (enemy.canTakeDamage()) {
                            std::lock_guard<std::mutex> lock(enemyMutex);
                            enemy.takeDamage(currentWeapon->getDamage());
                            enemy.applyKnockback(currentWeapon->getKnockback(facingRight));
                            
                            Vector2 hitPos = {enemyHitbox.x + enemyHitbox.width/2, enemyHitbox.y + enemyHitbox.height/2};
                            ParticleSystem::getInstance().createExplosionParticles(hitPos, 6, RED);
                        }
                    }
                }
            }
        }));
    }
    for (auto& f : enemyFutures) {
        if (f.valid()) f.get();
    }

    size_t automatonChunk = (automatons.size() + numThreads - 1) / numThreads;
    std::vector<std::future<void>> automatonFutures;
    for (size_t t = 0; t < numThreads; ++t) {
        size_t start = t * automatonChunk;
        size_t end = std::min(start + automatonChunk, automatons.size());
        if (start >= end) continue;
        automatonFutures.push_back(std::async(std::launch::async, [&, start, end]() {
            for (size_t i = start; i < end; ++i) {
                auto& automaton = automatons[i];
                if (automaton.isAlive()) {
                    Rectangle enemyHitbox = automaton.getHitbox();
                    if (CheckCollisionRecs(weaponHitbox, enemyHitbox)) {
                        if (automaton.canTakeDamage()) {
                            std::lock_guard<std::mutex> lock(automatonMutex);
                            automaton.takeDamage(currentWeapon->getDamage());
                            automaton.applyKnockback(currentWeapon->getKnockback(facingRight));
                            
                            Vector2 hitPos = {enemyHitbox.x + enemyHitbox.width/2, enemyHitbox.y + enemyHitbox.height/2};
                            ParticleSystem::getInstance().createExplosionParticles(hitPos, 6, ORANGE);
                        }
                    }
                }
            }
        }));
    }
    for (auto& f : automatonFutures) {
        if (f.valid()) f.get();
    }
}

Rectangle Player::getSwordHitbox() const {
    return getWeaponHitbox();
}

bool Player::isSwordAttacking() const {
    return isAttacking();
}