#ifndef SPAWNER_HPP
#define SPAWNER_HPP

#include <vector>
#include "map/Map.hpp"
#include "enemies/ScrapHound.hpp"

class Spawner {
public:
    Spawner();
    void spawnEnemiesInRooms(Map& map, std::vector<ScrapHound>& scrapHounds);
};

#endif // SPAWNER_HPP
