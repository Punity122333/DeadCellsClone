#include "map/Map.hpp"
#include <cstdio>
#include <vector>
#include <random> 
#include <future>
#include <thread>

constexpr int CHUNK_SIZE = 16;


Map::Map(int w, int h) :
    width(w),
    height(h),
    tiles(w, std::vector<int>(h, 0)),
    transitionTimers(w, std::vector<float>(h, 0.0f)),
    isOriginalSolid(w, std::vector<bool>(h, false)),
    isConwayProtected(w, std::vector<bool>(h, false))
{
    std::random_device rd;
    std::mt19937 gen(rd());

    
    size_t numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 2;
    size_t chunkSize = (width + numThreads - 1) / numThreads;
    std::vector<std::future<void>> futures;
    for (size_t t = 0; t < numThreads; ++t) {
        size_t start = t * chunkSize;
        size_t end = std::min(start + chunkSize, (size_t)width);
        futures.push_back(std::async(std::launch::async, [&, start, end]() {
            for (size_t x = start; x < end; ++x) {
                for (int y = 0; y < height; ++y) {
                    tiles[x][y] = 0;
                    isOriginalSolid[x][y] = false;
                    isConwayProtected[x][y] = false;
                }
            }
        }));
    }
    for (auto& f : futures) f.get();

    int numTiles = 0;
    {
        for (int i = 0; ; ++i) {
            char path[64];
            snprintf(path, sizeof(path), "../resources/tiles/tile%03d.png", i);
            FILE* f = fopen(path, "r");
            if (!f) break;
            fclose(f);
            numTiles++;
        }
    }
    for (int i = 0; i < numTiles; ++i) {
        char path[64];
        snprintf(path, sizeof(path), "../resources/tiles/tile%03d.png", i);
        tileTextures.push_back(LoadTexture(path));
    }

    for (int x = 0; x < width; x++) {
        tiles[x][height - 1] = 1; isOriginalSolid[x][height - 1] = true;
        tiles[x][0] = 1;         isOriginalSolid[x][0] = true;
    }
    for (int y = 0; y < height; y++) {
        tiles[width - 1][y] = 1; isOriginalSolid[width - 1][y] = true;
    }

    generateRoomsAndConnections(gen);

    chunks.clear();
    for (int cx = 0; cx < width; cx += CHUNK_SIZE) {
        for (int cy = 0; cy < height; cy += CHUNK_SIZE) {
            Chunk chunk;
            chunk.startX = cx;
            chunk.startY = cy;
            chunk.endX = std::min(cx + CHUNK_SIZE, width) - 1;
            chunk.endY = std::min(cy + CHUNK_SIZE, height) - 1;
            chunks.push_back(chunk);
        }
    }

    
    std::vector<std::future<void>> conwayFutures;
    chunkSize = (width + numThreads - 1) / numThreads;
    for (size_t t = 0; t < numThreads; ++t) {
        size_t start = t * chunkSize;
        size_t end = std::min(start + chunkSize, (size_t)width);
        conwayFutures.push_back(std::async(std::launch::async, [&, start, end]() {
            for (size_t x = start; x < end; ++x) {
                for (int y = 0; y < height; ++y) {
                    if (isOriginalSolid[x][y]) {
                        for (int dx = -2; dx <= 2; ++dx) {
                            for (int dy = -2; dy <= 2; ++dy) {
                                int nx = x + dx;
                                int ny = y + dy;
                                if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                                    isConwayProtected[nx][ny] = true;
                                }
                            }
                        }
                    }
                }
            }
        }));
    }
    for (auto& f : conwayFutures) f.get();
}

void Map::placeBorders() {
}

int Map::getHeight() const {
    return height;
}

Map::~Map() {
    for (const auto& texture : tileTextures) {
        UnloadTexture(texture);
    }
}