#include "map/Map.hpp"
#include <random>
#include <algorithm>
#include <thread>
#include <mutex>
#include <atomic>

namespace {
    constexpr int MIN_CONWAY_CHUNK_SIZE_X = 4;
    constexpr int MAX_CONWAY_CHUNK_SIZE_X = 8;
    constexpr int MIN_CONWAY_CHUNK_SIZE_Y = 1;
    constexpr int MAX_CONWAY_CHUNK_SIZE_Y = 2;
    constexpr int CHUNK_ALIVE_ROLL_MAX = 10;
    constexpr int CHUNK_ALIVE_SUCCESS_ROLL = 0;

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
    std::vector<std::vector<int>> nextTiles = tiles;
    std::vector<std::thread> threads;
    std::random_device rd;
    int numThreads = std::thread::hardware_concurrency();
    int chunkSize = (width + numThreads - 1) / numThreads;
    std::vector<std::mt19937> gens;
    for (int t = 0; t < numThreads; ++t) gens.emplace_back(rd());
    std::uniform_int_distribution<> chunkSizeDist(MIN_CONWAY_CHUNK_SIZE_X, MAX_CONWAY_CHUNK_SIZE_X - 2);
    std::uniform_int_distribution<> chunkYSizeDist(MIN_CONWAY_CHUNK_SIZE_Y, MAX_CONWAY_CHUNK_SIZE_Y);
    std::uniform_int_distribution<> horizontalBiasDist(0, 2);
    std::uniform_int_distribution<> shouldChunkBeAliveDist(0, CHUNK_ALIVE_ROLL_MAX + 3);
    std::uniform_int_distribution<> chunkSpacingDist(0, 1);
    std::mutex processedTilesMutex;
    std::vector<std::vector<bool>> processedTiles(width, std::vector<bool>(height, false));
    std::atomic<int> createdCount(0);
    std::atomic<int> deletedCount(0);
    std::atomic<int> skippedCount(0);

    int totalCreatable = 0, totalDeletable = 0;
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            if (isConwayProtected[x][y] || isNonEditable(tiles[x][y]) ||
                tiles[x][y] == MapConstants::CHEST_TILE_VALUE ||
                tiles[x][y] == MapConstants::WALL_TILE_VALUE ||
                tiles[x][y] == MapConstants::SHOP_TILE_VALUE ||
                tiles[x][y] == MapConstants::TREASURE_TILE_VALUE ||
                tiles[x][y] == MapConstants::PROTECTED_EMPTY_TILE_VALUE) {
                continue;
            }
            if (tiles[x][y] != TILE_ID_SOLID && tiles[x][y] != TILE_ID_PLATFORM) {
                totalCreatable++;
            } else {
                totalDeletable++;
            }
        }
    }
    int maxBalanced = std::min(totalCreatable, totalDeletable);
    std::atomic<int> runningCreated(0);
    std::atomic<int> runningDeleted(0);

    for (int t = 0; t < numThreads; ++t) {
        int xStart = t * chunkSize;
        int xEnd = std::min(width, xStart + chunkSize);
        threads.emplace_back([&, t, xStart, xEnd]() {
            auto& gen = gens[t];
            for (int startX = xStart; startX < xEnd;) {
                int startY = 0;
                while (startY < height) {
                    {
                        std::lock_guard<std::mutex> lock(processedTilesMutex);
                        if (processedTiles[startX][startY]) {
                            startY += chunkSpacingDist(gen);
                            continue;
                        }
                    }
                    bool shouldChunkBeAlive = (shouldChunkBeAliveDist(gen) == CHUNK_ALIVE_SUCCESS_ROLL);
                    int bias = horizontalBiasDist(gen);
                    int currentChunkSizeX, currentChunkSizeY;
                    if (bias == 1) {
                        currentChunkSizeX = MAX_CONWAY_CHUNK_SIZE_X;
                        currentChunkSizeY = chunkYSizeDist(gen);
                    } else if (bias == 2) {
                        currentChunkSizeX = chunkSizeDist(gen);
                        currentChunkSizeY = MAX_CONWAY_CHUNK_SIZE_Y;
                    } else {
                        currentChunkSizeX = chunkSizeDist(gen);
                        currentChunkSizeY = chunkYSizeDist(gen);
                    }
                    int chunkEndX = std::min(startX + currentChunkSizeX, xEnd);
                    int chunkEndY = std::min(startY + currentChunkSizeY, height);
                    bool isChunkProtectedOrSpecial = false;
                    for (int y = startY; y < chunkEndY; ++y) {
                        for (int x = startX; x < chunkEndX; ++x) {
                            if (isConwayProtected[x][y] || isNonEditable(tiles[x][y]) || tiles[x][y] == MapConstants::CHEST_TILE_VALUE || tiles[x][y] == MapConstants::WALL_TILE_VALUE || tiles[x][y] == MapConstants::SHOP_TILE_VALUE || tiles[x][y] == MapConstants::TREASURE_TILE_VALUE || tiles[x][y] == MapConstants::PROTECTED_EMPTY_TILE_VALUE) {
                                isChunkProtectedOrSpecial = true;
                                break;
                            }
                        }
                        if (isChunkProtectedOrSpecial) break;
                    }
                    if (isChunkProtectedOrSpecial) {
                        std::lock_guard<std::mutex> lock(processedTilesMutex);
                        for (int y = startY; y < chunkEndY; ++y) {
                            for (int x = startX; x < chunkEndX; ++x) {
                                processedTiles[x][y] = true;
                                skippedCount++;
                            }
                        }
                        startY += chunkSpacingDist(gen);
                        continue;
                    }
                    int chunkCreatable = 0, chunkDeletable = 0;
                    for (int y = startY; y < chunkEndY; ++y) {
                        for (int x = startX; x < chunkEndX; ++x) {
                            if (isNonEditable(tiles[x][y]) || tiles[x][y] == MapConstants::CHEST_TILE_VALUE || tiles[x][y] == MapConstants::WALL_TILE_VALUE || tiles[x][y] == MapConstants::SHOP_TILE_VALUE || tiles[x][y] == MapConstants::TREASURE_TILE_VALUE || tiles[x][y] == MapConstants::PROTECTED_EMPTY_TILE_VALUE) {
                                continue;
                            }
                            if (tiles[x][y] != TILE_ID_SOLID && tiles[x][y] != TILE_ID_PLATFORM) {
                                chunkCreatable++;
                            } else {
                                chunkDeletable++;
                            }
                        }
                    }
                    bool doCreate = false, doDelete = false;
                    if (shouldChunkBeAlive && runningCreated + chunkCreatable <= maxBalanced) {
                        doCreate = true;
                    } else if (!shouldChunkBeAlive && runningDeleted + chunkDeletable <= maxBalanced) {
                        doDelete = true;
                    }
                    for (int y = startY; y < chunkEndY; ++y) {
                        for (int x = startX; x < chunkEndX; ++x) {
                            {
                                std::lock_guard<std::mutex> lock(processedTilesMutex);
                                processedTiles[x][y] = true;
                            }
                            if (isNonEditable(tiles[x][y]) || tiles[x][y] == MapConstants::CHEST_TILE_VALUE || tiles[x][y] == MapConstants::WALL_TILE_VALUE || tiles[x][y] == MapConstants::SHOP_TILE_VALUE || tiles[x][y] == MapConstants::TREASURE_TILE_VALUE || tiles[x][y] == MapConstants::PROTECTED_EMPTY_TILE_VALUE) {
                                nextTiles[x][y] = tiles[x][y];
                                skippedCount++;
                                continue;
                            }
                            bool tileIsCurrentlySolidOrPlatform = (tiles[x][y] == TILE_ID_SOLID || tiles[x][y] == TILE_ID_PLATFORM);
                            if (doCreate && !tileIsCurrentlySolidOrPlatform) {
                                nextTiles[x][y] = TILE_HIGHLIGHT_CREATE;
                                transitionTimers[x][y] = 0.0f;
                                createdCount++;
                                runningCreated++;
                            } else if (doDelete && tileIsCurrentlySolidOrPlatform) {
                                nextTiles[x][y] = TILE_HIGHLIGHT_DELETE;
                                transitionTimers[x][y] = 0.0f;
                                deletedCount++;
                                runningDeleted++;
                            } else {
                                nextTiles[x][y] = tiles[x][y];
                                skippedCount++;
                            }
                        }
                    }
                    startY += currentChunkSizeY + chunkSpacingDist(gen);
                }
                startX += chunkSize;
            }
        });
    }
    for (auto& th : threads) th.join();
    tiles = nextTiles;
    printf("[ConwayAutomata] Created: %d, Deleted: %d, Skipped: %d\n", createdCount.load(), deletedCount.load(), skippedCount.load());
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
                            transitionTimers[x][y] = 0.0f;
                        } else {
                            transitionTimers[x][y] = timer;
                        }
                    } else if (tiles[x][y] == TILE_HIGHLIGHT_DELETE) {
                        float timer = transitionTimers[x][y] + dt;
                        if (timer >= HIGHLIGHT_TIME) {
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
