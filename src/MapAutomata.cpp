#include "Map.hpp"
#include <random>
#include <algorithm>

void Map::applyConwayAutomata() {
    std::vector<std::vector<int>> nextTiles = tiles;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> chunkSizeDist(6, 12);
    std::uniform_int_distribution<> chunkYSizeDist(1, 2);
    std::uniform_int_distribution<> shouldChunkBeAliveDist(0, 9);
    std::uniform_int_distribution<> newTileTypeDist(0, 1);
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
                    if (isConwayProtected[x][y] || tiles[x][y] == 2 || tiles[x][y] == 3) {
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

            bool shouldChunkBeAlive = (shouldChunkBeAliveDist(gen) == 0);

            for (int y = startY; y < chunkEndY; ++y) {
                for (int x = startX; x < chunkEndX; ++x) {
                    processedTiles[x][y] = true;

                    if (tiles[x][y] == 2 || tiles[x][y] == 3 ||
                        tiles[x][y] == 4 || tiles[x][y] == 5 || tiles[x][y] == 7 ||
                        tiles[x][y] == TILE_HIGHLIGHT_CREATE || tiles[x][y] == TILE_HIGHLIGHT_DELETE) {
                        nextTiles[x][y] = tiles[x][y];
                        continue;
                    }

                    bool tileIsCurrentlySolidOrPlatform = (tiles[x][y] == 1 || tiles[x][y] == 6);

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
    for (int x = 1; x < width - 1; ++x) {
        for (int y = 1; y < height - 1; ++y) {
            if (isConwayProtected[x][y]) {
                if (isOriginalSolid[x][y]) {
                    tiles[x][y] = 1;
                } else {
                    tiles[x][y] = 0;
                }
                transitionTimers[x][y] = 0.0f;
                continue;
            }

            if (tiles[x][y] == TILE_HIGHLIGHT_CREATE) {
                transitionTimers[x][y] += dt;
                if (transitionTimers[x][y] >= HIGHLIGHT_TIME) {
                    if (GetRandomValue(0, 1) == 0) {
                        tiles[x][y] = 4;
                    } else {
                        tiles[x][y] = 7;
                    }
                    transitionTimers[x][y] = 0.0f;
                }
            } else if (tiles[x][y] == TILE_HIGHLIGHT_DELETE) {
                transitionTimers[x][y] += dt;
                if (transitionTimers[x][y] >= HIGHLIGHT_TIME) {
                    tiles[x][y] = 5;
                    transitionTimers[x][y] = 0.0f;
                }
            }
            else if (tiles[x][y] == 4 || tiles[x][y] == 7) {
                transitionTimers[x][y] += dt;
                if (transitionTimers[x][y] >= GLITCH_TIME) {
                    if (tiles[x][y] == 4) {
                        tiles[x][y] = 1;
                    }
                    else if (tiles[x][y] == 7) {
                        tiles[x][y] = 6;
                    }
                    transitionTimers[x][y] = 0.0f;
                }
            } else if (tiles[x][y] == 5) {
                transitionTimers[x][y] += dt;
                if (transitionTimers[x][y] >= GLITCH_TIME) {
                    tiles[x][y] = 0;
                    transitionTimers[x][y] = 0.0f;
                }
            } else {
                transitionTimers[x][y] = 0.0f;
            }
        }
    }
}
