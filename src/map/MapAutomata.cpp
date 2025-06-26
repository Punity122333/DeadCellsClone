#include "map/Map.hpp"
#include <random>
#include <algorithm>
#include <thread>

#include <atomic>

namespace {
    constexpr int MIN_CONWAY_CHUNK_SIZE_X = 4;
    constexpr int MAX_CONWAY_CHUNK_SIZE_X = 12;
    constexpr int MIN_CONWAY_CHUNK_SIZE_Y = 1;
    constexpr int MAX_CONWAY_CHUNK_SIZE_Y = 2;
    constexpr int CHUNK_ALIVE_ROLL_MAX = 10;
    constexpr int CHUNK_ALIVE_SUCCESS_ROLL = 0;
    constexpr int CHUNK_COOLDOWN_FRAMES = 120;

    constexpr int TILE_ID_LADDER = 2;
    constexpr int TILE_ID_ROPE = 3;
    constexpr int TILE_ID_TEMP_CREATE_A = 4;
    constexpr int TILE_ID_TEMP_DELETE = 5;
    constexpr int TILE_ID_TEMP_CREATE_B = 8;
    constexpr int TILE_ID_SOLID = 1;
    constexpr int TILE_ID_PLATFORM = 6;
    constexpr int TILE_ID_EMPTY = 0;
    constexpr float HIGHLIGHT_TIME = 2.0f;
    constexpr float GLITCH_TIME = 0.5f;
    constexpr float BLINK_CYCLE_TIME = 1.0f;
    constexpr float MIN_HIGHLIGHT_OPACITY = 0.1f;

    constexpr int TILE_HIGHLIGHT_CREATE = 11;
    constexpr int TILE_HIGHLIGHT_DELETE = 12;
}

inline bool isNonEditable(int tile) {
    return tile == TILE_ID_LADDER || tile == TILE_ID_ROPE || tile == TILE_ID_TEMP_CREATE_A || tile == TILE_ID_TEMP_DELETE || tile == TILE_ID_TEMP_CREATE_B || tile == TILE_HIGHLIGHT_CREATE || tile == TILE_HIGHLIGHT_DELETE || tile == 7;
}

void Map::applyConwayAutomata() {
    if (cooldownMap.empty()) {
        cooldownMap.resize(width, std::vector<int>(height, 0));
    }
    
    std::vector<std::vector<int>> nextTiles = tiles;
    std::random_device rd;
    int numThreads = std::min(8, (int)std::thread::hardware_concurrency());
    
    std::vector<std::mt19937> gens;
    gens.reserve(numThreads);
    for (int t = 0; t < numThreads; ++t) gens.emplace_back(rd());
    
    std::vector<std::pair<int, int>> candidateChunks;
    candidateChunks.reserve(width * height / 16);
    
    std::mt19937 masterGen(rd());
    std::uniform_int_distribution<> chunkSizeDist(MIN_CONWAY_CHUNK_SIZE_X, MAX_CONWAY_CHUNK_SIZE_X);
    std::uniform_int_distribution<> chunkYSizeDist(MIN_CONWAY_CHUNK_SIZE_Y, MAX_CONWAY_CHUNK_SIZE_Y);
    std::uniform_int_distribution<> shouldChunkBeAliveDist(0, CHUNK_ALIVE_ROLL_MAX + 3);
    std::uniform_int_distribution<> spacingDist(3, 8);
    
    for (int x = 0; x < width; x += spacingDist(masterGen)) {
        for (int y = 0; y < height; y += spacingDist(masterGen)) {
            int chunkW = chunkSizeDist(masterGen);
            int chunkH = chunkYSizeDist(masterGen);
            if (x + chunkW < width && y + chunkH < height) {
                bool canPlace = true;
                for (int cy = y; cy < y + chunkH && canPlace; ++cy) {
                    for (int cx = x; cx < x + chunkW && canPlace; ++cx) {
                        if (isConwayProtected[cx][cy] || isNonEditable(tiles[cx][cy]) ||
                            tiles[cx][cy] == MapConstants::CHEST_TILE_VALUE ||
                            tiles[cx][cy] == MapConstants::WALL_TILE_VALUE ||
                            tiles[cx][cy] == MapConstants::SHOP_TILE_VALUE ||
                            tiles[cx][cy] == MapConstants::TREASURE_TILE_VALUE ||
                            tiles[cx][cy] == MapConstants::PROTECTED_EMPTY_TILE_VALUE ||
                            cooldownMap[cx][cy] > 0) {
                            canPlace = false;
                        }
                    }
                }
                if (canPlace) {
                    candidateChunks.emplace_back(x, y);
                }
            }
        }
    }
    
    if (candidateChunks.empty()) return;
    
    std::shuffle(candidateChunks.begin(), candidateChunks.end(), masterGen);
    size_t maxChunks = std::min(candidateChunks.size(), (size_t)(width * height / 200));
    candidateChunks.resize(maxChunks);
    
    std::atomic<int> createdCount(0), deletedCount(0), processedChunks(0);
    std::vector<std::thread> threads;
    threads.reserve(numThreads);
    
    size_t chunksPerThread = (candidateChunks.size() + numThreads - 1) / numThreads;
    
    for (int t = 0; t < numThreads; ++t) {
        size_t start = t * chunksPerThread;
        size_t end = std::min(start + chunksPerThread, candidateChunks.size());
        if (start >= end) break;
        
        threads.emplace_back([&, t, start, end]() {
            auto& gen = gens[t];
            int localCreated = 0, localDeleted = 0;
            
            for (size_t i = start; i < end; ++i) {
                int x = candidateChunks[i].first;
                int y = candidateChunks[i].second;
                
                bool shouldCreate = (shouldChunkBeAliveDist(gen) == CHUNK_ALIVE_SUCCESS_ROLL);
                int chunkW = chunkSizeDist(gen);
                int chunkH = chunkYSizeDist(gen);
                
                for (int cy = y; cy < std::min(y + chunkH, height); ++cy) {
                    for (int cx = x; cx < std::min(x + chunkW, width); ++cx) {
                        if (isConwayProtected[cx][cy] || cooldownMap[cx][cy] > 0) continue;
                        
                        bool isSolid = (tiles[cx][cy] == TILE_ID_SOLID || tiles[cx][cy] == TILE_ID_PLATFORM);
                        
                        if (shouldCreate && !isSolid) {
                            nextTiles[cx][cy] = TILE_HIGHLIGHT_CREATE;
                            transitionTimers[cx][cy] = 0.0f;
                            cooldownMap[cx][cy] = CHUNK_COOLDOWN_FRAMES;
                            
                            localCreated++;
                        } else if (!shouldCreate && isSolid) {
                            nextTiles[cx][cy] = TILE_HIGHLIGHT_DELETE;
                            transitionTimers[cx][cy] = 0.0f;
                            cooldownMap[cx][cy] = CHUNK_COOLDOWN_FRAMES;
                            
                            localDeleted++;
                        }
                    }
                }
                processedChunks++;
            }
            
            createdCount += localCreated;
            deletedCount += localDeleted;
        });
    }
    
    for (auto& th : threads) th.join();
    tiles = nextTiles;
    printf("[ConwayAutomata] Processed: %d chunks, Created: %d, Deleted: %d\n", 
           processedChunks.load(), createdCount.load(), deletedCount.load());
}

void Map::updateTransitions(float dt) {
    int numThreads = std::thread::hardware_concurrency();
    int chunkSize = (height - 2 + numThreads - 1) / numThreads;
    std::vector<std::thread> threads;
    std::random_device rd;
    std::vector<std::mt19937> gens;
    for (int t = 0; t < numThreads; ++t) gens.emplace_back(rd());
    for (int t = 0; t < numThreads; ++t) {
        int yStart = 1 + t * chunkSize;
        int yEnd = std::min(height - 1, yStart + chunkSize);
        threads.emplace_back([&, t, yStart, yEnd]() {
            auto& gen = gens[t];
            std::uniform_int_distribution<> tileChoiceDist(0, 1);
            std::vector<int> batchChoices(width * (yEnd - yStart));
            for (auto& v : batchChoices) v = tileChoiceDist(gen);
            int batchIdx = 0;
            for (int x = 1; x < width - 1; ++x) {
                for (int y = yStart; y < yEnd; ++y) {
                    if (cooldownMap[x][y] > 0) {
                        cooldownMap[x][y]--;
                    }
                    if (isConwayProtected[x][y]) {
                        if (isOriginalSolid[x][y]) {
                            tiles[x][y] = TILE_ID_SOLID;
                        } else {
                            tiles[x][y] = TILE_ID_EMPTY;
                        }
                        transitionTimers[x][y] = 0.0f;
                        continue;
                    }
                    if (tiles[x][y] == TILE_HIGHLIGHT_CREATE) {
                        float timer = transitionTimers[x][y] + dt;
                        if (timer >= HIGHLIGHT_TIME) {
                            if (batchChoices[batchIdx++] == 0) {
                                tiles[x][y] = TILE_ID_TEMP_CREATE_A;
                            } else {
                                tiles[x][y] = TILE_ID_TEMP_CREATE_B;
                            }
                            createPopEffect({(float)(x * 32 + 16), (float)(y * 32 + 16)});
                            transitionTimers[x][y] = 0.0f;
                        } else {
                            transitionTimers[x][y] = timer;
                        }
                    } else if (tiles[x][y] == TILE_HIGHLIGHT_DELETE) {
                        float timer = transitionTimers[x][y] + dt;
                        if (timer >= HIGHLIGHT_TIME) {
                            createSuctionEffect({(float)(x * 32 + 16), (float)(y * 32 + 16)});
                            tiles[x][y] = TILE_ID_TEMP_DELETE;
                            transitionTimers[x][y] = 0.0f;
                        } else {
                            transitionTimers[x][y] = timer;
                        }
                    } else if (tiles[x][y] == TILE_ID_TEMP_CREATE_A || tiles[x][y] == TILE_ID_TEMP_CREATE_B) {
                        float timer = transitionTimers[x][y] + dt;
                        if (timer >= GLITCH_TIME) {
                            if (tiles[x][y] == TILE_ID_TEMP_CREATE_A) {
                                tiles[x][y] = TILE_ID_PLATFORM;
                                isOriginalSolid[x][y] = false;
                                isConwayProtected[x][y] = false;
                            } else if (tiles[x][y] == TILE_ID_TEMP_CREATE_B) {
                                tiles[x][y] = TILE_ID_PLATFORM;
                                isOriginalSolid[x][y] = false;
                                isConwayProtected[x][y] = false;
                            }
                            transitionTimers[x][y] = 0.0f;
                        } else {
                            transitionTimers[x][y] = timer;
                        }
                    } else if (tiles[x][y] == TILE_ID_TEMP_DELETE) {
                        float timer = transitionTimers[x][y] + dt;
                        if (timer >= GLITCH_TIME) {
                            tiles[x][y] = TILE_ID_EMPTY;
                            transitionTimers[x][y] = 0.0f;
                        } else {
                            transitionTimers[x][y] = timer;
                        }
                    }
                }
            }
        });
    }
    for (auto& th : threads) th.join();
}
