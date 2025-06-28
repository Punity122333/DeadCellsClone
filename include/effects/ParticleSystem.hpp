#pragma once
#include <raylib.h>
#include <vector>
#include <mutex>
#include <atomic>
#include <queue>
#include <memory>
#include "effects/ThreadPool.hpp"

struct Particle {
    Vector2 position;
    Vector2 velocity;
    Color color;
    float life;
    float maxLife;
    float size;
};

struct ParticleCreationJob {
    std::vector<Particle> particles;
    std::function<void(std::vector<Particle>&)> creationFunction;
};

class ParticleSystem {
public:
    static ParticleSystem& getInstance();
    
    void createExplosion(Vector2 position, int count, Color color, float duration, float speed = 100.0f);
    void update(float deltaTime);
    void draw();
    void clear();
    
    void createDustParticle(Vector2 position, Vector2 velocity, float lifetime);
    void createExplosionParticles(Vector2 position, int count, Color baseColor);

private:
    std::vector<Particle> particles;
    std::queue<std::vector<Particle>> pendingParticles;
    std::mutex particleMutex;
    std::mutex pendingMutex;
    std::unique_ptr<ThreadPool> threadPool;
    
    ParticleSystem();
    
    ParticleSystem(const ParticleSystem&) = delete;
    ParticleSystem& operator=(const ParticleSystem&) = delete;
    
    void processPendingParticles();
    void updateParticleRange(size_t start, size_t end, float deltaTime);
};
