#include "Spawner.hpp"
#include "map/Map.hpp" 
#include "enemies/ScrapHound.hpp" 
#include "enemies/Automaton.hpp"
#include "enemies/Detonode.hpp"
#include "enemies/EnemyManager.hpp"
#include "core/GlobalThreadPool.hpp"
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <future>
#include <mutex>
#include <random>
#include <vector>
#include <raymath.h>
#include <memory>

namespace SpawnerConstants {
    constexpr float TileSize = 32.0f;
    constexpr float BaseSpawnRate = 0.1f;
    constexpr float DistanceMultiplier = 0.0005f;
    constexpr float MaxSpawnRate = 0.7f;
    constexpr float MinSpawnDistance = 200.0f;
    constexpr float MinEnemyDistance = 64.0f;
}

Spawner::Spawner() {
}

void Spawner::spawnEnemiesInRooms(Map& map, std::vector<ScrapHound>& scrapHounds, std::vector<Automaton>& automatons, std::vector<Detonode>& detonodes) {
    const auto& rooms = map.getGeneratedRooms();
    scrapHounds.clear();
    automatons.clear();
    detonodes.clear();
    printf("[Spawner] Number of rooms: %zu\n", rooms.size());
    
    Vector2 playerSpawn = map.findEmptySpawn();
    printf("[Spawner] Player spawn at (%.1f, %.1f)\n", playerSpawn.x, playerSpawn.y);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::atomic<int> totalScrapHoundsSpawned{0};
    std::atomic<int> totalAutomatonsSpawned{0};
    std::atomic<int> totalDetonodesSpawned{0};
    std::mutex scrapHoundsMutex;
    std::mutex automatonsMutex;
    std::mutex detonodeMutex;
    std::vector<std::future<void>> futures;
    
    for (const auto& room : rooms) {
        futures.push_back(GlobalThreadPool::getInstance().getMainPool().enqueue([&map, &scrapHounds, &automatons, &detonodes, &gen, &totalScrapHoundsSpawned, &totalAutomatonsSpawned, &totalDetonodesSpawned, &scrapHoundsMutex, &automatonsMutex, &detonodeMutex, room, playerSpawn]() mutable {
            Vector2 roomCenter = {
                static_cast<float>(room.startX + room.endX) * SpawnerConstants::TileSize * 0.5f,
                static_cast<float>(room.startY + room.endY) * SpawnerConstants::TileSize * 0.5f
            };
            
            float distanceFromPlayer = Vector2Distance(roomCenter, playerSpawn);
            
            if (distanceFromPlayer < SpawnerConstants::MinSpawnDistance) {
                return;
            }
            
            float baseSpawnRate = SpawnerConstants::BaseSpawnRate + (distanceFromPlayer * SpawnerConstants::DistanceMultiplier);
            baseSpawnRate = std::min(baseSpawnRate, SpawnerConstants::MaxSpawnRate);
            
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
            std::vector<Vector2> usedSpawns;
            
            enum class EnemyType { SCRAP_HOUND, AUTOMATON, DETONODE };
            struct EnemySpawnAttempt {
                EnemyType type;
                float finalSpawnChance;
                int maxCount;
                bool requiresMinDistance;
                std::mutex* mutex;
            };
            
            ScrapHound tempScrapHound({0, 0});
            Automaton tempAutomaton({0, 0});
            Detonode tempDetonode({0, 0});
            
            auto scrapHoundConfig = tempScrapHound.getSpawnConfig();
            auto automatonConfig = tempAutomaton.getSpawnConfig();
            auto detonodeConfig = tempDetonode.getSpawnConfig();
            
            std::vector<EnemySpawnAttempt> spawnAttempts = {
                {EnemyType::SCRAP_HOUND, baseSpawnRate * scrapHoundConfig.spawnChance, 
                 scrapHoundConfig.maxPerRoom, scrapHoundConfig.requiresMinDistance, &scrapHoundsMutex},
                {EnemyType::AUTOMATON, baseSpawnRate * automatonConfig.spawnChance, 
                 automatonConfig.maxPerRoom, automatonConfig.requiresMinDistance, &automatonsMutex},
                {EnemyType::DETONODE, detonodeConfig.spawnChance, 
                 detonodeConfig.maxPerRoom, detonodeConfig.requiresMinDistance, &detonodeMutex}
            };
            
            for (const auto& spawnAttempt : spawnAttempts) {
                int spawned = 0;
                auto availableSpawns = validSpawns;
                
                if (spawnAttempt.requiresMinDistance) {
                    availableSpawns.erase(
                        std::remove_if(availableSpawns.begin(), availableSpawns.end(),
                            [&usedSpawns](const Vector2& spawn) {
                                for (const auto& used : usedSpawns) {
                                    if (Vector2Distance(spawn, used) < SpawnerConstants::MinEnemyDistance) {
                                        return true;
                                    }
                                }
                                return false;
                            }),
                        availableSpawns.end());
                }
                
                for (int i = 0; i < spawnAttempt.maxCount && spawned < spawnAttempt.maxCount && !availableSpawns.empty(); ++i) {
                    if (spawnChance(gen) <= spawnAttempt.finalSpawnChance) {
                        Vector2 spawnPos = availableSpawns.back();
                        availableSpawns.pop_back();
                        usedSpawns.push_back(spawnPos);
                        
                        std::atomic<int>* counter = nullptr;
                        const char* typeName = "";
                        
                        switch (spawnAttempt.type) {
                            case EnemyType::SCRAP_HOUND:
                                {
                                    std::lock_guard<std::mutex> lock(*spawnAttempt.mutex);
                                    scrapHounds.emplace_back(spawnPos);
                                }
                                counter = &totalScrapHoundsSpawned;
                                typeName = "ScrapHound";
                                break;
                            case EnemyType::AUTOMATON:
                                {
                                    std::lock_guard<std::mutex> lock(*spawnAttempt.mutex);
                                    automatons.emplace_back(spawnPos);
                                }
                                counter = &totalAutomatonsSpawned;
                                typeName = "Automaton";
                                break;
                            case EnemyType::DETONODE:
                                {
                                    std::lock_guard<std::mutex> lock(*spawnAttempt.mutex);
                                    detonodes.emplace_back(spawnPos);
                                }
                                counter = &totalDetonodesSpawned;
                                typeName = "Detonode";
                                break;
                        }
                        
                        if (counter) {
                            counter->fetch_add(1, std::memory_order_relaxed);
                            printf("[Spawner] %s spawned at (%.1f, %.1f), distance: %.1f, rate: %.3f\n", 
                                   typeName, spawnPos.x, spawnPos.y, distanceFromPlayer, spawnAttempt.finalSpawnChance);
                            spawned++;
                        }
                    }
                }
            }
        }));
    }
    for (auto& f : futures) f.get();
    printf("[Spawner] Total ScrapHounds spawned: %d\n", totalScrapHoundsSpawned.load(std::memory_order_relaxed));
    printf("[Spawner] Total Automatons spawned: %d\n", totalAutomatonsSpawned.load(std::memory_order_relaxed));
    printf("[Spawner] Total Detonodes spawned: %d\n", totalDetonodesSpawned.load(std::memory_order_relaxed));
}

void Spawner::spawnEnemiesInRooms(Map& map, EnemyManager& enemyManager) {
    const auto& rooms = map.getGeneratedRooms();
    enemyManager.clearEnemies();
    printf("[Spawner] Number of rooms: %zu\n", rooms.size());
    
    Vector2 playerSpawn = map.findEmptySpawn();
    printf("[Spawner] Player spawn at (%.1f, %.1f)\n", playerSpawn.x, playerSpawn.y);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::atomic<int> totalScrapHoundsSpawned{0};
    std::atomic<int> totalAutomatonsSpawned{0};
    std::atomic<int> totalDetonodesSpawned{0};
    std::mutex enemyMutex;
    
    std::vector<std::future<void>> futures;
    
    for (const auto& room : rooms) {
        futures.push_back(GlobalThreadPool::getInstance().getMainPool().enqueue([&map, &enemyManager, &gen, &totalScrapHoundsSpawned, &totalAutomatonsSpawned, &totalDetonodesSpawned, &enemyMutex, room, playerSpawn]() mutable {
            Vector2 roomCenter = {
                static_cast<float>(room.startX + room.endX) * SpawnerConstants::TileSize * 0.5f,
                static_cast<float>(room.startY + room.endY) * SpawnerConstants::TileSize * 0.5f
            };
            
            float distanceFromPlayer = Vector2Distance(roomCenter, playerSpawn);
            if (distanceFromPlayer < SpawnerConstants::MinSpawnDistance) return;
            
            float baseSpawnRate = std::min(SpawnerConstants::BaseSpawnRate + distanceFromPlayer * SpawnerConstants::DistanceMultiplier, 
                                         SpawnerConstants::MaxSpawnRate);
            
            std::uniform_real_distribution<float> spawnChance(0.0f, 1.0f);
            std::vector<Vector2> validSpawns;
            
            for (int y = room.startY; y <= room.endY; ++y) {
                for (int x = room.startX; x <= room.endX; ++x) {
                    if (map.isTileEmpty(x, y) && (y + 1 < map.getHeight()) && map.isSolidTile(x, y + 1)) {
                        Vector2 spawnPos = {
                            static_cast<float>(x) * SpawnerConstants::TileSize + SpawnerConstants::TileSize / 2.0f,
                            static_cast<float>(y) * SpawnerConstants::TileSize
                        };
                        validSpawns.push_back(spawnPos);
                    }
                }
            }
            
            if (validSpawns.empty()) return;
            
            std::shuffle(validSpawns.begin(), validSpawns.end(), gen);
            std::vector<Vector2> usedSpawns;
            
            enum class EnemyType { SCRAP_HOUND, AUTOMATON, DETONODE };
            struct EnemySpawnAttempt {
                EnemyType type;
                float finalSpawnChance;
                int maxCount;
                bool requiresMinDistance;
            };
            
            ScrapHound tempScrapHound({0, 0});
            Automaton tempAutomaton({0, 0});
            Detonode tempDetonode({0, 0});
            
            auto scrapHoundConfig = tempScrapHound.getSpawnConfig();
            auto automatonConfig = tempAutomaton.getSpawnConfig();
            auto detonodeConfig = tempDetonode.getSpawnConfig();
            
            std::vector<EnemySpawnAttempt> spawnAttempts = {
                {EnemyType::SCRAP_HOUND, baseSpawnRate * scrapHoundConfig.spawnChance, 
                 scrapHoundConfig.maxPerRoom, scrapHoundConfig.requiresMinDistance},
                {EnemyType::AUTOMATON, baseSpawnRate * automatonConfig.spawnChance, 
                 automatonConfig.maxPerRoom, automatonConfig.requiresMinDistance},
                {EnemyType::DETONODE, detonodeConfig.spawnChance, 
                 detonodeConfig.maxPerRoom, detonodeConfig.requiresMinDistance}
            };
            
            for (const auto& spawnAttempt : spawnAttempts) {
                int spawned = 0;
                auto availableSpawns = validSpawns;
                
                if (spawnAttempt.requiresMinDistance) {
                    availableSpawns.erase(
                        std::remove_if(availableSpawns.begin(), availableSpawns.end(),
                            [&usedSpawns](const Vector2& spawn) {
                                for (const auto& used : usedSpawns) {
                                    if (Vector2Distance(spawn, used) < SpawnerConstants::MinEnemyDistance) {
                                        return true;
                                    }
                                }
                                return false;
                            }),
                        availableSpawns.end());
                }
                
                for (int i = 0; i < spawnAttempt.maxCount && spawned < spawnAttempt.maxCount && !availableSpawns.empty(); ++i) {
                    if (spawnChance(gen) <= spawnAttempt.finalSpawnChance) {
                        Vector2 spawnPos = availableSpawns.back();
                        availableSpawns.pop_back();
                        usedSpawns.push_back(spawnPos);
                        
                        std::unique_ptr<Enemy> enemy;
                        std::atomic<int>* counter = nullptr;
                        const char* typeName = "";
                        
                        switch (spawnAttempt.type) {
                            case EnemyType::SCRAP_HOUND:
                                enemy = std::make_unique<ScrapHound>(spawnPos);
                                counter = &totalScrapHoundsSpawned;
                                typeName = "ScrapHound";
                                break;
                            case EnemyType::AUTOMATON:
                                enemy = std::make_unique<Automaton>(spawnPos);
                                counter = &totalAutomatonsSpawned;
                                typeName = "Automaton";
                                break;
                            case EnemyType::DETONODE:
                                enemy = std::make_unique<Detonode>(spawnPos);
                                counter = &totalDetonodesSpawned;
                                typeName = "Detonode";
                                break;
                        }
                        
                        if (enemy && counter) {
                            {
                                std::lock_guard<std::mutex> lock(enemyMutex);
                                enemyManager.addEnemy(std::move(enemy));
                            }
                            counter->fetch_add(1, std::memory_order_relaxed);
                            printf("[Spawner] %s spawned at (%.1f, %.1f), distance: %.1f, rate: %.3f\n", 
                                   typeName, spawnPos.x, spawnPos.y, distanceFromPlayer, spawnAttempt.finalSpawnChance);
                            spawned++;
                        }
                    }
                }
            }
        }));
    }
    for (auto& f : futures) f.get();
    
    printf("[Spawner] Total ScrapHounds spawned: %d\n", totalScrapHoundsSpawned.load(std::memory_order_relaxed));
    printf("[Spawner] Total Automatons spawned: %d\n", totalAutomatonsSpawned.load(std::memory_order_relaxed));
    printf("[Spawner] Total Detonodes spawned: %d\n", totalDetonodesSpawned.load(std::memory_order_relaxed));
}
