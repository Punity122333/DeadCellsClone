#include "enemies/EnemyManager.hpp"
#include "map/Map.hpp"
#include "Camera.hpp"

void EnemyManager::addEnemy(std::unique_ptr<Enemy> enemy) {
    enemies.push_back(std::move(enemy));
}

void EnemyManager::removeDeadEnemies() {
    enemies.erase(
        std::remove_if(enemies.begin(), enemies.end(),
                       [](const std::unique_ptr<Enemy>& enemy) {
                           return !enemy->isAlive();
                       }),
        enemies.end());
}

void EnemyManager::updateEnemies(const Map& map, Vector2 playerPos, float dt) {
    for (auto& enemy : enemies) {
        if (enemy->isAlive()) {
            enemy->update(map, playerPos, dt);
        }
    }
}

void EnemyManager::updateEnemies(Map& map, Vector2 playerPos, float dt, GameCamera& camera) {
    for (auto& enemy : enemies) {
        if (enemy->isAlive()) {
            if (enemy->getType() == EnemyType::DETONODE) {
                Detonode* detonode = static_cast<Detonode*>(enemy.get());
                detonode->update(map, playerPos, dt, camera);
            } else {
                enemy->update(map, playerPos, dt);
            }
        }
    }
}

void EnemyManager::drawEnemies() const {
    for (const auto& enemy : enemies) {
        if (enemy->isAlive()) {
            enemy->draw();
        }
    }
}

void EnemyManager::clearEnemies() {
    enemies.clear();
}

std::vector<Enemy*> EnemyManager::getEnemiesOfType(EnemyType type) const {
    std::vector<Enemy*> result;
    for (const auto& enemy : enemies) {
        if (enemy->getType() == type && enemy->isAlive()) {
            result.push_back(enemy.get());
        }
    }
    return result;
}

std::vector<Enemy*> EnemyManager::getAllEnemies() const {
    std::vector<Enemy*> result;
    for (const auto& enemy : enemies) {
        if (enemy->isAlive()) {
            result.push_back(enemy.get());
        }
    }
    return result;
}

size_t EnemyManager::getEnemyCountOfType(EnemyType type) const {
    size_t count = 0;
    for (const auto& enemy : enemies) {
        if (enemy->getType() == type && enemy->isAlive()) {
            count++;
        }
    }
    return count;
}
