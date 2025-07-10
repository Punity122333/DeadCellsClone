#pragma once
#include "enemies/Enemy.hpp"
#include "enemies/ScrapHound.hpp"

#include "enemies/Detonode.hpp"
#include <vector>
#include <memory>

class Map;
class Player;
class GameCamera;

class EnemyManager {
public:
    EnemyManager() = default;
    ~EnemyManager() = default;
    
    EnemyManager(const EnemyManager&) = delete;
    EnemyManager& operator=(const EnemyManager&) = delete;
    
    EnemyManager(EnemyManager&&) = default;
    EnemyManager& operator=(EnemyManager&&) = default;

    void addEnemy(std::unique_ptr<Enemy> enemy);
    void removeDeadEnemies();
    void updateEnemies(const Map& map, Vector2 playerPos, float dt);
    void updateEnemies(Map& map, Vector2 playerPos, float dt, GameCamera& camera);
    void drawEnemies() const;
    void clearEnemies();
    
    std::vector<Enemy*> getEnemiesOfType(EnemyType type) const;
    std::vector<Enemy*> getAllEnemies() const;
    size_t getEnemyCount() const { return enemies.size(); }
    size_t getEnemyCountOfType(EnemyType type) const;

private:
    std::vector<std::unique_ptr<Enemy>> enemies;
};
