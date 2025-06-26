#include "map/Map.hpp"
#include "map/RoomGenerator.hpp"
#include <cstdio>
#include <vector>
#include <random> 
#include <future>
#include <thread>
#include <cmath>
#include <algorithm>


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
    cooldownMap(w, std::vector<int>(h, 0)),
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

void Map::updateParticles(float dt, Vector2 playerPosition) {
    std::lock_guard<std::mutex> lock(particlesMutex);
    
    constexpr size_t MAX_PARTICLES = 350;
    
    // First, sort particles by distance to player (closest first) before applying cap
    if (particles.size() > MAX_PARTICLES) {
        std::sort(particles.begin(), particles.end(), [&playerPosition](const auto& a, const auto& b) {
            float distA = (a.position.x - playerPosition.x) * (a.position.x - playerPosition.x) + 
                         (a.position.y - playerPosition.y) * (a.position.y - playerPosition.y);
            float distB = (b.position.x - playerPosition.x) * (b.position.x - playerPosition.x) + 
                         (b.position.y - playerPosition.y) * (b.position.y - playerPosition.y);
            return distA < distB;
        });
        
        // Keep only the closest particles to the player
        particles.erase(particles.begin() + MAX_PARTICLES, particles.end());
    }
    
    const float gravity = 200.0f * dt;
    const float damping = 0.98f;
    
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
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * PI);
    std::uniform_real_distribution<float> speedDist(50.0f, 120.0f);
    std::uniform_real_distribution<float> sizeDist(1.0f, 2.5f);
    std::uniform_real_distribution<float> lifeDist(0.2f, 0.6f);
    
    static const Color colors[] = {
        {255, 100, 100, 255},
        {100, 255, 100, 255},
        {100, 100, 255, 255},
        {255, 255, 100, 255},
        {255, 100, 255, 255}
    };
    
    std::lock_guard<std::mutex> lock(particlesMutex);
    particles.reserve(particles.size() + 5);
    for (int i = 0; i < 5; ++i) {
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
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * PI);
    std::uniform_real_distribution<float> radiusDist(12.0f, 20.0f);
    std::uniform_real_distribution<float> sizeDist(0.5f, 1.5f);
    std::uniform_real_distribution<float> lifeDist(0.4f, 0.8f);
    
    static const Color colors[] = {
        {200, 50, 50, 255},
        {50, 50, 200, 255},
        {200, 200, 50, 255},
        {200, 50, 200, 255}
    };
    
    std::lock_guard<std::mutex> lock(particlesMutex);
    particles.reserve(particles.size() + 4);
    for (int i = 0; i < 4; ++i) {
        float angle = angleDist(gen);
        float radius = radiusDist(gen);
        Vector2 startPos = {
            position.x + cosf(angle) * radius,
            position.y + sinf(angle) * radius
        };
        Vector2 velocity = {
            (position.x - startPos.x) * 1.8f,
            (position.y - startPos.y) * 1.8f
        };
        float size = sizeDist(gen);
        float life = lifeDist(gen);
        Color color = colors[i];
        
        particles.emplace_back(startPos, velocity, color, life, size);
    }
}