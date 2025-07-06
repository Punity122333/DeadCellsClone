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
    constexpr float BaseSpawnRate = 0.1f;
    constexpr float DistanceMultiplier = 0.0005f;
    constexpr float MaxSpawnRate = 0.8f;
    constexpr float MinSpawnDistance = 200.0f;
    constexpr int MaxAutomatonsPerRoom = 2;
    constexpr int MaxScrapHoundsPerRoom = 3;
}

Spawner::Spawner() {
}

void Spawner::spawnEnemiesInRooms(Map& map, std::vector<ScrapHound>& scrapHounds, std::vector<Automaton>& automatons) {
    const auto& rooms = map.getGeneratedRooms();
    scrapHounds.clear();
    automatons.clear();
    printf("[Spawner] Number of rooms: %zu\n", rooms.size());
    
    Vector2 playerSpawn = map.findEmptySpawn();
    printf("[Spawner] Player spawn at (%.1f, %.1f)\n", playerSpawn.x, playerSpawn.y);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::atomic<int> totalScrapHoundsSpawned{0};
    std::atomic<int> totalAutomatonsSpawned{0};
    std::mutex scrapHoundsMutex;
    std::mutex automatonsMutex;
    std::vector<std::future<void>> futures;
    
    for (const auto& room : rooms) {
        futures.push_back(GlobalThreadPool::getInstance().getMainPool().enqueue([&map, &scrapHounds, &automatons, &gen, &totalScrapHoundsSpawned, &totalAutomatonsSpawned, &scrapHoundsMutex, &automatonsMutex, room, playerSpawn]() mutable {
            Vector2 roomCenter = {
                static_cast<float>(room.startX + room.endX) * SpawnerConstants::TileSize * 0.5f,
                static_cast<float>(room.startY + room.endY) * SpawnerConstants::TileSize * 0.5f
            };
            
            float distanceFromPlayer = Vector2Distance(roomCenter, playerSpawn);
            
            if (distanceFromPlayer < SpawnerConstants::MinSpawnDistance) {
                return;
            }
            
            float spawnRate = SpawnerConstants::BaseSpawnRate + (distanceFromPlayer * SpawnerConstants::DistanceMultiplier);
            spawnRate = std::min(spawnRate, SpawnerConstants::MaxSpawnRate);
            
            std::uniform_real_distribution<float> spawnChance(0.0f, 1.0f);
            
            std::vector<Vector2> validSpawns;
            for (int y = room.startY + 1; y < room.endY - 1; ++y) {
                for (int x = room.startX + 1; x < room.endX - 1; ++x) {
                    if (map.isTileEmpty(x, y) && (y + 1 < map.getHeight()) && map.isSolidTile(x, y + 1)) {
                        Vector2 spawnPos = {
                            static_cast<float>(x) * SpawnerConstants::TileSize + SpawnerConstants::TileSize / 2.0f,
                            static_cast<float>(y) * SpawnerConstants::TileSize
                        };
                        validSpawns.push_back(spawnPos);
                    }
                }
            }
            
            if (validSpawns.empty()) {
                return;
            }
            
            std::shuffle(validSpawns.begin(), validSpawns.end(), gen);
            
            int maxScrapHounds = static_cast<int>(SpawnerConstants::MaxScrapHoundsPerRoom * spawnRate);
            int maxAutomatons = static_cast<int>(SpawnerConstants::MaxAutomatonsPerRoom * spawnRate);
            
            std::vector<Vector2> usedSpawns;
            
            for (int i = 0; i < maxScrapHounds && !validSpawns.empty(); ++i) {
                if (spawnChance(gen) <= spawnRate) {
                    Vector2 spawnPos = validSpawns.back();
                    validSpawns.pop_back();
                    usedSpawns.push_back(spawnPos);
                    
                    {
                        std::lock_guard<std::mutex> lock(scrapHoundsMutex);
                        scrapHounds.emplace_back(spawnPos);
                    }
                    totalScrapHoundsSpawned.fetch_add(1, std::memory_order_relaxed);
                    printf("[Spawner] ScrapHound spawned at (%.1f, %.1f), distance: %.1f, rate: %.3f\n", 
                           spawnPos.x, spawnPos.y, distanceFromPlayer, spawnRate);
                }
            }
            
            std::vector<Vector2> automatonSpawns;
            for (const auto& spawn : validSpawns) {
                bool tooClose = false;
                for (const auto& used : usedSpawns) {
                    if (Vector2Distance(spawn, used) < SpawnerConstants::ScrapHoundAutomatonMinDist) {
                        tooClose = true;
                        break;
                    }
                }
                if (!tooClose) {
                    automatonSpawns.push_back(spawn);
                }
            }
            
            std::shuffle(automatonSpawns.begin(), automatonSpawns.end(), gen);
            
            for (int i = 0; i < maxAutomatons && !automatonSpawns.empty(); ++i) {
                if (spawnChance(gen) <= spawnRate) {
                    Vector2 spawnPos = automatonSpawns.back();
                    automatonSpawns.pop_back();
                    
                    {
                        std::lock_guard<std::mutex> lock(automatonsMutex);
                        automatons.emplace_back(spawnPos);
                    }
                    totalAutomatonsSpawned.fetch_add(1, std::memory_order_relaxed);
                    printf("[Spawner] Automaton spawned at (%.1f, %.1f), distance: %.1f, rate: %.3f\n", 
                           spawnPos.x, spawnPos.y, distanceFromPlayer, spawnRate);
                }
            }
        }));
    }
    for (auto& f : futures) f.get();
    printf("[Spawner] Total ScrapHounds spawned: %d\n", totalScrapHoundsSpawned.load(std::memory_order_relaxed));
    printf("[Spawner] Total Automatons spawned: %d\n", totalAutomatonsSpawned.load(std::memory_order_relaxed));
}
