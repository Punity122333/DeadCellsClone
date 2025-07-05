#include "Spawner.hpp"
#include "map/Map.hpp" 
#include "enemies/ScrapHound.hpp" 
#include "enemies/Automaton.hpp"
#include "core/GlobalThreadPool.hpp"
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <future>
#include <mutex>
#include <random>
#include <vector>
#include <raymath.h>

namespace SpawnerConstants {
    constexpr float TileSize = 32.0f;
    constexpr float ScrapHoundAutomatonMinDist = 64.0f;
    constexpr int AutomatonsPerRoom = 1;
}

Spawner::Spawner() {
}

void Spawner::spawnEnemiesInRooms(Map& map, std::vector<ScrapHound>& scrapHounds, std::vector<Automaton>& automatons) {
    const auto& rooms = map.getGeneratedRooms();
    scrapHounds.clear();
    automatons.clear();
    printf("[Spawner] Number of rooms: %zu\n", rooms.size());
    std::random_device rd;
    std::mt19937 gen(rd());
    std::atomic<int> totalScrapHoundsSpawned{0};
    std::atomic<int> totalAutomatonsSpawned{0};
    std::mutex scrapHoundsMutex;
    std::mutex automatonsMutex;
    std::vector<std::future<void>> futures;
    for (const auto& room : rooms) {
        futures.push_back(GlobalThreadPool::getInstance().getMainPool().enqueue([&map, &scrapHounds, &automatons, &gen, &totalScrapHoundsSpawned, &totalAutomatonsSpawned, &scrapHoundsMutex, &automatonsMutex, room]() mutable {
            std::vector<Vector2> validScrapHoundSpawns;
            std::vector<Vector2> validAutomatonSpawns;
            for (int y = room.startY + 1; y < room.endY - 1; ++y) {
                for (int x = room.startX + 1; x < room.endX - 1; ++x) {
                    if (map.isTileEmpty(x, y) && (y + 1 < map.getHeight()) && map.isSolidTile(x, y + 1)) {
                        Vector2 spawnPos = {
                            static_cast<float>(x) * SpawnerConstants::TileSize + SpawnerConstants::TileSize / 2.0f,
                            static_cast<float>(y) * SpawnerConstants::TileSize
                        };
                        validScrapHoundSpawns.push_back(spawnPos);
                        validAutomatonSpawns.push_back(spawnPos);
                    }
                }
            }
            if (!validScrapHoundSpawns.empty()) {
                std::uniform_int_distribution<> pickSpawn(0, validScrapHoundSpawns.size() - 1);
                Vector2 chosen = validScrapHoundSpawns[pickSpawn(gen)];
                {
                    std::lock_guard<std::mutex> lock(scrapHoundsMutex);
                    scrapHounds.emplace_back(chosen);
                }
                totalScrapHoundsSpawned.fetch_add(1, std::memory_order_relaxed);
                printf("[Spawner] ScrapHound spawned in room at (%.1f, %.1f)\n", chosen.x, chosen.y);
                validAutomatonSpawns.erase(
                    std::remove_if(validAutomatonSpawns.begin(), validAutomatonSpawns.end(),
                        [chosen](const Vector2& pos) { 
                            return Vector2Distance(pos, chosen) < SpawnerConstants::ScrapHoundAutomatonMinDist; 
                        }),
                    validAutomatonSpawns.end()
                );
            } else {
                printf("[Spawner] No valid spawn found for ScrapHound in room (%d,%d)-(%d,%d)\n", room.startX, room.startY, room.endX, room.endY);
            }
            for (int i = 0; i < SpawnerConstants::AutomatonsPerRoom; ++i) {
                if (!validAutomatonSpawns.empty()) {
                    std::shuffle(validAutomatonSpawns.begin(), validAutomatonSpawns.end(), gen);
                    Vector2 spawnPos = validAutomatonSpawns.back();
                    validAutomatonSpawns.pop_back();
                    {
                        std::lock_guard<std::mutex> lock(automatonsMutex);
                        automatons.emplace_back(spawnPos);
                    }
                    totalAutomatonsSpawned.fetch_add(1, std::memory_order_relaxed);
                    printf("[Spawner] Automaton spawned at (%.1f, %.1f) in room (%d, %d)\n", spawnPos.x, spawnPos.y, room.startX, room.startY);
                } else {
                    printf("[Spawner] No valid spawn found for Automaton in room (%d,%d)-(%d,%d)\n", room.startX, room.startY, room.endX, room.endY);
                }
            }
        }));
    }
    for (auto& f : futures) f.get();
    printf("[Spawner] Total ScrapHounds spawned: %d\n", totalScrapHoundsSpawned.load(std::memory_order_relaxed));
    printf("[Spawner] Total Automatons spawned: %d\n", totalAutomatonsSpawned.load(std::memory_order_relaxed));
}
