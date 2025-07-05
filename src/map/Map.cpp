#include "map/Map.hpp"
#include "map/RoomGenerator.hpp"
#include "core/GlobalThreadPool.hpp"
#include <cstdio>
#include <vector>
#include <random> 
#include <future>
#include <thread>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif


namespace MapConstants {
    constexpr int BORDER_TILE_VALUE = 1;
    constexpr int CHUNK_SIZE = 16;
    constexpr size_t MAX_PARTICLES = 160;
    constexpr float GRAVITY_MULTIPLIER = 200.0f;
    constexpr float DAMPING = 0.98f;
    constexpr int POP_PARTICLE_COUNT = 5;
    constexpr int SUCTION_PARTICLE_COUNT = 4;
    constexpr float POP_SPEED_MIN = 50.0f;
    constexpr float POP_SPEED_MAX = 120.0f;
    constexpr float POP_SIZE_MIN = 1.0f;
    constexpr float POP_SIZE_MAX = 2.5f;
    constexpr float POP_LIFE_MIN = 0.2f;
    constexpr float POP_LIFE_MAX = 0.6f;
    constexpr float SUCTION_RADIUS_MIN = 12.0f;
    constexpr float SUCTION_RADIUS_MAX = 20.0f;
    constexpr float SUCTION_SIZE_MIN = 0.5f;
    constexpr float SUCTION_SIZE_MAX = 1.5f;
    constexpr float SUCTION_LIFE_MIN = 0.4f;
    constexpr float SUCTION_LIFE_MAX = 0.8f;
    constexpr float SUCTION_VEL_MULT = 1.8f;
}

using namespace MapConstants;

constexpr int CHUNK_SIZE = 16;



Map::Map(int w, int h, const std::vector<Texture2D>& loadedTileTextures, ProgressCallback progressCallback) :
    width(w),
    height(h),
    tiles(w, std::vector<int>(h, 0)),
    transitionTimers(w, std::vector<float>(h, 0.0f)),
    cooldownMap(w, std::vector<int>(h, 0)),
    isOriginalSolid(w, std::vector<bool>(h, false)),
    isConwayProtected(w, std::vector<bool>(h, false)),
    tileTextures(loadedTileTextures) 
{
    try {
        if (progressCallback) progressCallback(0.0f);
        std::random_device rd;
        std::mt19937 gen(rd());        
        size_t numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0) numThreads = 2;
        
        size_t totalTiles = width * height;
        size_t tilesPerThread = (totalTiles + numThreads - 1) / numThreads;
        std::vector<std::future<void>> futures;
        
        try {
            for (size_t t = 0; t < numThreads; ++t) {
                size_t startTile = t * tilesPerThread;
                size_t endTile = std::min(startTile + tilesPerThread, totalTiles);
                
                if (startTile < endTile) {
                    futures.push_back(GlobalThreadPool::getInstance().getMainPool().enqueue([&, startTile, endTile]() {
                        for (size_t tileIdx = startTile; tileIdx < endTile; ++tileIdx) {
                            int x = tileIdx % width;
                            int y = tileIdx / width;
                            tiles[x][y] = 0;
                            isOriginalSolid[x][y] = false;
                            isConwayProtected[x][y] = false;
                        }
                    }));
                }
            }
            for (auto& f : futures) f.get();
        } catch (...) {

            for (int x = 0; x < width; ++x) {
                for (int y = 0; y < height; ++y) {
                    tiles[x][y] = 0;
                    isOriginalSolid[x][y] = false;
                    isConwayProtected[x][y] = false;
                }
            }
        }

        if (progressCallback) progressCallback(0.15f);


        std::vector<std::future<void>> borderFutures;

        borderFutures.push_back(GlobalThreadPool::getInstance().getMainPool().enqueue([&]() {
            for (int x = 0; x < width; x++) {
                tiles[x][height - 1] = BORDER_TILE_VALUE; 
                isOriginalSolid[x][height - 1] = true;
                tiles[x][0] = BORDER_TILE_VALUE;         
                isOriginalSolid[x][0] = true;
            }
        }));
 
        borderFutures.push_back(GlobalThreadPool::getInstance().getMainPool().enqueue([&]() {
            for (int y = 0; y < height; y++) {
                tiles[width - 1][y] = BORDER_TILE_VALUE; 
                isOriginalSolid[width - 1][y] = true;
                tiles[0][y] = BORDER_TILE_VALUE;
                isOriginalSolid[0][y] = true;
            }
        }));

        for (auto& future : borderFutures) {
            future.get();
        }

        if (progressCallback) progressCallback(0.25f);


        chunks.clear();
        int totalChunks = ((width + CHUNK_SIZE - 1) / CHUNK_SIZE) * ((height + CHUNK_SIZE - 1) / CHUNK_SIZE);
        chunks.reserve(totalChunks);
        
        for (int cx = 0; cx < width; cx += CHUNK_SIZE) {
            for (int cy = 0; cy < height; cy += CHUNK_SIZE) {
                chunks.emplace_back(Chunk{
                    cx,
                    cy,
                    std::min(cx + CHUNK_SIZE, width) - 1,
                    std::min(cy + CHUNK_SIZE, height) - 1
                });
            }
        }

        if (progressCallback) progressCallback(0.35f);

        
        std::vector<std::future<void>> conwayFutures;
        
        try {
            for (size_t t = 0; t < numThreads; ++t) {
                size_t startTile = t * tilesPerThread;
                size_t endTile = std::min(startTile + tilesPerThread, totalTiles);
                
                if (startTile < endTile) {
                    conwayFutures.push_back(GlobalThreadPool::getInstance().getMainPool().enqueue([&, startTile, endTile]() {
                        for (size_t tileIdx = startTile; tileIdx < endTile; ++tileIdx) {
                            int x = tileIdx % width;
                            int y = tileIdx / width;
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
                    }));
                }
            }
            for (auto& f : conwayFutures) f.get();
        } catch (...) {

            for (int x = 0; x < width; ++x) {
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
        }
        

        if (progressCallback) progressCallback(0.55f);

        printf("[Map] Starting room generation...\n");
        RoomGenerator::generateRoomsAndConnections(*this, gen, progressCallback);
        printf("[Map] Room generation complete\n");

        if (progressCallback) progressCallback(1.0f);
        printf("[Map] Map generation fully complete\n");
    } catch (...) {

        if (progressCallback) progressCallback(1.0f);
        printf("[Map] Map generation failed with exception\n");
        throw; 
    }
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

void Map::updateParticles(float dt, Vector2 playerPosition) {
    std::lock_guard<std::mutex> lock(particlesMutex);
    
    if (particles.size() > MAX_PARTICLES) {
        std::sort(particles.begin(), particles.end(), [&playerPosition](const auto& a, const auto& b) {
            float distA = (a.position.x - playerPosition.x) * (a.position.x - playerPosition.x) + 
                         (a.position.y - playerPosition.y) * (a.position.y - playerPosition.y);
            float distB = (b.position.x - playerPosition.x) * (b.position.x - playerPosition.x) + 
                         (b.position.y - playerPosition.y) * (b.position.y - playerPosition.y);
            return distA < distB;
        });
        particles.erase(particles.begin() + MAX_PARTICLES, particles.end());
    }
    
    const float gravity = GRAVITY_MULTIPLIER * dt;
    const float damping = DAMPING;
    
    for (auto it = particles.begin(); it != particles.end();) {
        it->life -= dt;
        if (it->life <= 0) {
            it = particles.erase(it);
        } else {
            it->position.x += it->velocity.x * dt;
            it->position.y += it->velocity.y * dt;
            it->velocity.x *= damping;
            it->velocity.y = it->velocity.y * damping + gravity;
            float alpha = it->life / it->maxLife;
            it->color.a = (unsigned char)(255 * alpha);
            ++it;
        }
    }
}

void Map::createPopEffect(Vector2 position) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * M_PI);
    std::uniform_real_distribution<float> speedDist(POP_SPEED_MIN, POP_SPEED_MAX);
    std::uniform_real_distribution<float> sizeDist(POP_SIZE_MIN, POP_SIZE_MAX);
    std::uniform_real_distribution<float> lifeDist(POP_LIFE_MIN, POP_LIFE_MAX);
    
    static const Color colors[] = {
        {255, 100, 100, 255},
        {100, 255, 100, 255},
        {100, 100, 255, 255},
        {255, 255, 100, 255},
        {255, 100, 255, 255}
    };
    
    std::lock_guard<std::mutex> lock(particlesMutex);
    particles.reserve(particles.size() + POP_PARTICLE_COUNT);
    for (int i = 0; i < POP_PARTICLE_COUNT; ++i) {
        float angle = angleDist(gen);
        float speed = speedDist(gen);
        Vector2 velocity = {cosf(angle) * speed, sinf(angle) * speed};
        float size = sizeDist(gen);
        float life = lifeDist(gen);
        Color color = colors[i];
        particles.emplace_back(position, velocity, color, life, size);
    }
}

void Map::createSuctionEffect(Vector2 position) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * M_PI);
    std::uniform_real_distribution<float> radiusDist(SUCTION_RADIUS_MIN, SUCTION_RADIUS_MAX);
    std::uniform_real_distribution<float> sizeDist(SUCTION_SIZE_MIN, SUCTION_SIZE_MAX);
    std::uniform_real_distribution<float> lifeDist(SUCTION_LIFE_MIN, SUCTION_LIFE_MAX);
    
    static const Color colors[] = {
        {200, 50, 50, 255},
        {50, 50, 200, 255},
        {200, 200, 50, 255},
        {200, 50, 200, 255}
    };
    
    std::lock_guard<std::mutex> lock(particlesMutex);
    particles.reserve(particles.size() + SUCTION_PARTICLE_COUNT);
    for (int i = 0; i < SUCTION_PARTICLE_COUNT; ++i) {
        float angle = angleDist(gen);
        float radius = radiusDist(gen);
        Vector2 startPos = {
            position.x + cosf(angle) * radius,
            position.y + sinf(angle) * radius
        };
        Vector2 velocity = {
            (position.x - startPos.x) * SUCTION_VEL_MULT,
            (position.y - startPos.y) * SUCTION_VEL_MULT
        };
        float size = sizeDist(gen);
        float life = lifeDist(gen);
        Color color = colors[i];
        particles.emplace_back(startPos, velocity, color, life, size);
    }
}