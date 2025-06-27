#pragma once
#include <raylib.h>
#include <vector>
#include <mutex>

struct Particle {
    Vector2 position;
    Vector2 velocity;
    Color color;
    float life;
    float maxLife;
    float size;
};

class ParticleSystem {
public:
    static ParticleSystem& getInstance();
    
    void createExplosion(Vector2 position, int count, Color color, float duration, float speed = 100.0f);
    void update(float deltaTime);
    void draw();
    void clear();
    
    // Thread-safe particle creation methods
    void createDustParticle(Vector2 position, Vector2 velocity, float lifetime);
    void createExplosionParticles(Vector2 position, int count, Color baseColor);

private:
    std::vector<Particle> particles;
    std::mutex particleMutex;
    ParticleSystem() = default;
    
    // Delete copy constructor and assignment operator
    ParticleSystem(const ParticleSystem&) = delete;
    ParticleSystem& operator=(const ParticleSystem&) = delete;
};
