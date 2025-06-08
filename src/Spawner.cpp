#include "Spawner.hpp"
#include "map/Map.hpp" // Included via Spawner.hpp but good for clarity
#include "enemies/ScrapHound.hpp" // Included via Spawner.hpp
#include <vector>

namespace {
    constexpr float TILE_SIZE_FLOAT = 32.0f; // Assuming default tile size
}

Spawner::Spawner() {
}

void Spawner::spawnEnemiesInRooms(Map& map, std::vector<ScrapHound>& scrapHounds) {
    const auto& rooms = map.getGeneratedRooms();
    scrapHounds.clear(); // Clear existing hounds before spawning new ones based on rooms

    for (const auto& room : rooms) {
        bool spawnedInRoom = false;
        for (int y = room.startY + 1; y < room.endY -1 && !spawnedInRoom; ++y) {
            for (int x = room.startX + 1; x < room.endX -1; ++x) {
                if (map.isTileEmpty(x, y) && (y + 1 < map.getHeight()) && map.isSolidTile(x, y + 1)) {
                    Vector2 spawnPos = {
                        static_cast<float>(x) * TILE_SIZE_FLOAT + TILE_SIZE_FLOAT / 2.0f,
                        static_cast<float>(y) * TILE_SIZE_FLOAT
                    };
                    scrapHounds.emplace_back(spawnPos);
                    spawnedInRoom = true;
                    break;
                }
            }
        }
    }
}
