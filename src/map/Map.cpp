#include "map/Map.hpp"
#include "map/RoomGenerator.hpp"
#include <cstdio>
#include <vector>
#include <random> 
#include <future>
#include <thread>
#include <filesystem>

namespace {
    constexpr int BORDER_TILE_VALUE = 1;
}

constexpr int CHUNK_SIZE = 16;


// Modified constructor
Map::Map(int w, int h, const std::vector<Texture2D>& loadedTileTextures) :
    width(w),
    height(h),
    tiles(w, std::vector<int>(h, 0)),
    transitionTimers(w, std::vector<float>(h, 0.0f)),
    isOriginalSolid(w, std::vector<bool>(h, false)),
    isConwayProtected(w, std::vector<bool>(h, false)),
    tileTextures(loadedTileTextures) // Assign pre-loaded textures
{
    std::random_device rd;
    std::mt19937 gen(rd());

    size_t numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 2;
    size_t calculated_chunk_size = (width + numThreads - 1) / numThreads;
    std::vector<std::future<void>> futures;
    for (size_t t = 0; t < numThreads; ++t) {
        size_t start = t * calculated_chunk_size;
        size_t end = std::min(start + calculated_chunk_size, (size_t)width);
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

    // REMOVE the texture loading loop from here
    /*
    int numTiles = 0;
    {
        for (int i = 0; ; ++i) {
            char path_buffer[64];
            snprintf(path_buffer, sizeof(path_buffer), "../resources/tiles/tile%03d.png", i);
            if (!std::filesystem::exists(path_buffer)) break;
            numTiles++;
        }
    }
    for (int i = 0; i < numTiles; ++i) {
        char path_buffer[64];
        snprintf(path_buffer, sizeof(path_buffer), "../resources/tiles/tile%03d.png", i);
        tileTextures.push_back(LoadTexture(path_buffer));
    }
    */

    for (int x = 0; x < width; x++) {
        tiles[x][height - 1] = BORDER_TILE_VALUE; isOriginalSolid[x][height - 1] = true;
        tiles[x][0] = BORDER_TILE_VALUE;         isOriginalSolid[x][0] = true;
    }
    for (int y = 0; y < height; y++) {
        tiles[width - 1][y] = BORDER_TILE_VALUE; isOriginalSolid[width - 1][y] = true;
    }

    

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
    for (size_t t = 0; t < numThreads; ++t) {
        size_t start = t * calculated_chunk_size;
        size_t end = std::min(start + calculated_chunk_size, (size_t)width);
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

    RoomGenerator::generateRoomsAndConnections(*this, gen);
}

void Map::placeBorders() {
}

const std::vector<Room>& Map::getGeneratedRooms() const {
    return generatedRooms;
}

int Map::getHeight() const {
    return height;
}

int Map::getWidth() const {
    return width;
}

Map::~Map() {
    
}