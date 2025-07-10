#ifndef SPAWNER_HPP
#define SPAWNER_HPP

#include <vector>
#include "map/Map.hpp"
#include "enemies/EnemyManager.hpp"

class ScrapHound;
class Automaton;
class Detonode;

class Spawner {
public:
    Spawner();
    void spawnEnemiesInRooms(Map& map, EnemyManager& enemyManager);
    void spawnEnemiesInRooms(Map& map, std::vector<ScrapHound>& scrapHounds, std::vector<Automaton>& automatons, std::vector<Detonode>& detonodes);
};

#endif
