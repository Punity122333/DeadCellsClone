#include "Spawner.hpp"
#include "map/Map.hpp" // Included via Spawner.hpp but good for clarity
#include "enemies/ScrapHound.hpp" // Included via Spawner.hpp
#include "enemies/Automaton.hpp"
#include <vector>
#include <cstdio>
#include <random>

namespace {
    constexpr float TILE_SIZE_FLOAT = 32.0f; // Assuming default tile size
}

Spawner::Spawner() {
}

void Spawner::spawnEnemiesInRooms(Map& map, std::vector<ScrapHound>& scrapHounds, std::vector<Automaton>& automatons) {
    const auto& rooms = map.getGeneratedRooms();
    scrapHounds.clear(); // Clear existing hounds before spawning new ones based on rooms
    automatons.clear(); // Clear existing automatons before spawning new ones based on rooms

    printf("[Spawner] Number of rooms: %zu\n", rooms.size());
    std::random_device rd;
    std::mt19937 gen(rd());

    int totalScrapHoundsSpawned = 0;
    int totalAutomatonsSpawned = 0;
    for (const auto& room : rooms) {
        std::vector<Vector2> validScrapHoundSpawns;
        std::vector<Vector2> validAutomatonSpawns;
        for (int y = room.startY + 1; y < room.endY - 1; ++y) {
            for (int x = room.startX + 1; x < room.endX - 1; ++x) {
                if (map.isTileEmpty(x, y) && (y + 1 < map.getHeight()) && map.isSolidTile(x, y + 1)) {
                    Vector2 spawnPos = {
                        static_cast<float>(x) * TILE_SIZE_FLOAT + TILE_SIZE_FLOAT / 2.0f,
                        static_cast<float>(y) * TILE_SIZE_FLOAT
                    };
                    validScrapHoundSpawns.push_back(spawnPos);
                }
            }
        }
        if (!validScrapHoundSpawns.empty()) {
            std::uniform_int_distribution<> pickSpawn(0, validScrapHoundSpawns.size() - 1);
            Vector2 chosen = validScrapHoundSpawns[pickSpawn(gen)];
            scrapHounds.emplace_back(chosen);
            totalScrapHoundsSpawned++;
            printf("[Spawner] ScrapHound spawned in room at (%.1f, %.1f)\n", chosen.x, chosen.y);
        } else {
            printf("[Spawner] No valid spawn found for ScrapHound in room (%d,%d)-(%d,%d)\n", room.startX, room.startY, room.endX, room.endY);
        }

        // Spawn Automatons
        for (int i = 0; i < 1; ++i) { // Currently set to spawn 1 Automaton per room
            std::uniform_int_distribution<> xDist(room.startX + 2, room.endX - 2);
            std::uniform_int_distribution<> yDist(room.startY + 2, room.endY - 2);
            int x = xDist(gen);
            int y = yDist(gen);
            Vector2 spawnPos = {
                static_cast<float>(x) * TILE_SIZE_FLOAT + TILE_SIZE_FLOAT / 2.0f,
                static_cast<float>(y) * TILE_SIZE_FLOAT
            };
            automatons.emplace_back(spawnPos);
            totalAutomatonsSpawned++;
            printf("[Spawner] Automaton spawned in room at (%.1f, %.1f)\n", spawnPos.x, spawnPos.y);
        }
    }
    printf("[Spawner] Total ScrapHounds spawned: %d\n", totalScrapHoundsSpawned);
    printf("[Spawner] Total Automatons spawned: %d\n", totalAutomatonsSpawned);
}
