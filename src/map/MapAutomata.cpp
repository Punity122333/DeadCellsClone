#include "map/Map.hpp"
#include <random>
#include <algorithm>

namespace {
    constexpr int MIN_CONWAY_CHUNK_SIZE_X = 6;
    constexpr int MAX_CONWAY_CHUNK_SIZE_X = 12;
    constexpr int MIN_CONWAY_CHUNK_SIZE_Y = 1;
    constexpr int MAX_CONWAY_CHUNK_SIZE_Y = 2;
    constexpr int CHUNK_ALIVE_ROLL_MAX = 9; 
    constexpr int CHUNK_ALIVE_SUCCESS_ROLL = 0;

    constexpr int TILE_ID_LADDER = 2;
    constexpr int TILE_ID_ROPE = 3;
    constexpr int TILE_ID_TEMP_CREATE_A = 4; 
    constexpr int TILE_ID_TEMP_DELETE = 5;   
    constexpr int TILE_ID_TEMP_CREATE_B = 8; 
    constexpr int TILE_ID_SOLID = 1;
    constexpr int TILE_ID_PLATFORM = 6;
    constexpr int TILE_ID_EMPTY = 0;
}

void Map::applyConwayAutomata() {
    std::vector<std::vector<int>> nextTiles = tiles;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> chunkSizeDist(MIN_CONWAY_CHUNK_SIZE_X, MAX_CONWAY_CHUNK_SIZE_X);
    std::uniform_int_distribution<> chunkYSizeDist(MIN_CONWAY_CHUNK_SIZE_Y, MAX_CONWAY_CHUNK_SIZE_Y);
    std::uniform_int_distribution<> shouldChunkBeAliveDist(0, CHUNK_ALIVE_ROLL_MAX);
    std::vector<std::vector<bool>> processedTiles(width, std::vector<bool>(height, false));

    for (int startX = 0; startX < width; ++startX) {
        for (int startY = 0; startY < height; ++startY) {
            if (processedTiles[startX][startY]) {
                continue;
            }

            const int currentChunkSizeX = chunkSizeDist(gen);
            const int currentChunkSizeY = chunkYSizeDist(gen);
            const int chunkEndX = std::min(startX + currentChunkSizeX, width);
            const int chunkEndY = std::min(startY + currentChunkSizeY, height);

            bool isChunkProtectedOrSpecial = false;
            for (int y = startY; y < chunkEndY; ++y) {
                for (int x = startX; x < chunkEndX; ++x) {
                    if (isConwayProtected[x][y] || tiles[x][y] == TILE_ID_LADDER || tiles[x][y] == TILE_ID_ROPE || tiles[x][y] == 7) {
                        isChunkProtectedOrSpecial = true;
                        break;
                    }
                }
                if (isChunkProtectedOrSpecial) break;
            }

            if (isChunkProtectedOrSpecial) {
                for (int y = startY; y < chunkEndY; ++y) {
                    for (int x = startX; x < chunkEndX; ++x) {
                        processedTiles[x][y] = true;
                    }
                }
                continue;
            }

            bool shouldChunkBeAlive = (shouldChunkBeAliveDist(gen) == CHUNK_ALIVE_SUCCESS_ROLL);

            for (int y = startY; y < chunkEndY; ++y) {
                for (int x = startX; x < chunkEndX; ++x) {
                    processedTiles[x][y] = true;

                    if (tiles[x][y] == TILE_ID_LADDER || tiles[x][y] == TILE_ID_ROPE ||
                        tiles[x][y] == TILE_ID_TEMP_CREATE_A || tiles[x][y] == TILE_ID_TEMP_DELETE || tiles[x][y] == TILE_ID_TEMP_CREATE_B ||
                        tiles[x][y] == TILE_HIGHLIGHT_CREATE || tiles[x][y] == TILE_HIGHLIGHT_DELETE || tiles[x][y] == 7) {
                        nextTiles[x][y] = tiles[x][y];
                        continue;
                    }

                    bool tileIsCurrentlySolidOrPlatform = (tiles[x][y] == TILE_ID_SOLID || tiles[x][y] == TILE_ID_PLATFORM);

                    if (shouldChunkBeAlive) {
                        if (!tileIsCurrentlySolidOrPlatform) {
                            nextTiles[x][y] = TILE_HIGHLIGHT_CREATE;
                            transitionTimers[x][y] = 0.0f;
                        } else {
                            nextTiles[x][y] = tiles[x][y];
                        }
                    } else {
                        if (tileIsCurrentlySolidOrPlatform) {
                            nextTiles[x][y] = TILE_HIGHLIGHT_DELETE;
                            transitionTimers[x][y] = 0.0f;
                        } else {
                            nextTiles[x][y] = tiles[x][y];
                        }
                    }
                }
            }
        }
    }
    tiles = nextTiles;
}

void Map::updateTransitions(float dt) {
    std::uniform_int_distribution<> tileChoiceDist(0, 1); // For replacing GetRandomValue
    std::random_device rd; // Need a random device for the generator
    std::mt19937 gen(rd()); // And a generator

    for (int x = 1; x < width - 1; ++x) {
        for (int y = 1; y < height - 1; ++y) {
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
                transitionTimers[x][y] += dt;
                if (transitionTimers[x][y] >= HIGHLIGHT_TIME) {
                    if (tileChoiceDist(gen) == 0) { // Replaced GetRandomValue
                        tiles[x][y] = TILE_ID_TEMP_CREATE_A;
                    } else {
                        tiles[x][y] = TILE_ID_TEMP_CREATE_B;
                    }
                    transitionTimers[x][y] = 0.0f;
                }
            } else if (tiles[x][y] == TILE_HIGHLIGHT_DELETE) {
                transitionTimers[x][y] += dt;
                if (transitionTimers[x][y] >= HIGHLIGHT_TIME) {
                    tiles[x][y] = TILE_ID_TEMP_DELETE;
                    transitionTimers[x][y] = 0.0f;
                }
            }
            else if (tiles[x][y] == TILE_ID_TEMP_CREATE_A || tiles[x][y] == TILE_ID_TEMP_CREATE_B) {
                transitionTimers[x][y] += dt;
                if (transitionTimers[x][y] >= GLITCH_TIME) {
                    if (tiles[x][y] == TILE_ID_TEMP_CREATE_A) {
                        tiles[x][y] = TILE_ID_SOLID;
                    }
                    else if (tiles[x][y] == TILE_ID_TEMP_CREATE_B) {
                        tiles[x][y] = TILE_ID_PLATFORM;
                    }
                    transitionTimers[x][y] = 0.0f;
                }
            } else if (tiles[x][y] == TILE_ID_TEMP_DELETE) {
                transitionTimers[x][y] += dt;
                if (transitionTimers[x][y] >= GLITCH_TIME) {
                    tiles[x][y] = TILE_ID_EMPTY;
                    transitionTimers[x][y] = 0.0f;
                }
            } else {
                transitionTimers[x][y] = 0.0f;
            }
        }
    }
}
