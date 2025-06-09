#include "Spawner.hpp"
#include "map/Map.hpp" // Included via Spawner.hpp but good for clarity
#include "enemies/ScrapHound.hpp" // Included via Spawner.hpp
#include <vector>
#include <cstdio>
#include <random>

namespace {
    constexpr float TILE_SIZE_FLOAT = 32.0f; // Assuming default tile size
}

Spawner::Spawner() {
}

void Spawner::spawnEnemiesInRooms(Map& map, std::vector<ScrapHound>& scrapHounds) {
    const auto& rooms = map.getGeneratedRooms();
    scrapHounds.clear(); // Clear existing hounds before spawning new ones based on rooms

    printf("[Spawner] Number of rooms: %zu\n", rooms.size());
    std::random_device rd;
    std::mt19937 gen(rd());

    int totalSpawned = 0;
    for (const auto& room : rooms) {
        std::vector<Vector2> validSpawns;
        for (int y = room.startY + 1; y < room.endY - 1; ++y) {
            for (int x = room.startX + 1; x < room.endX - 1; ++x) {
                if (map.isTileEmpty(x, y) && (y + 1 < map.getHeight()) && map.isSolidTile(x, y + 1)) {
                    Vector2 spawnPos = {
                        static_cast<float>(x) * TILE_SIZE_FLOAT + TILE_SIZE_FLOAT / 2.0f,
                        static_cast<float>(y) * TILE_SIZE_FLOAT
                    };
                    validSpawns.push_back(spawnPos);
                }
            }
        }
        if (!validSpawns.empty()) {
            std::uniform_int_distribution<> pickSpawn(0, validSpawns.size() - 1);
            Vector2 chosen = validSpawns[pickSpawn(gen)];
            scrapHounds.emplace_back(chosen);
            totalSpawned++;
            printf("[Spawner] ScrapHound spawned in room at (%.1f, %.1f)\n", chosen.x, chosen.y);
        } else {
            printf("[Spawner] No valid spawn found for room (%d,%d)-(%d,%d)\n", room.startX, room.startY, room.endX, room.endY);
        }
    }
    printf("[Spawner] Total ScrapHounds spawned: %d\n", totalSpawned);
}
