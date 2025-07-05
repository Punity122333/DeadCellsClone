#include "effects/ParticleSystem.hpp"
#include "effects/ParticleThreadPool.hpp"
#include <cmath>
#include <cstdlib>
#include <thread>
#include <algorithm>
#include <future>

ParticleSystem::ParticleSystem() {
}

ParticleSystem& ParticleSystem::getInstance() {
    static ParticleSystem instance;
    return instance;
}

void ParticleSystem::createExplosion(Vector2 position, int count, Color color, float duration, float speed) {
    auto future = ParticleThreadPool::getInstance().getPool().enqueue([=]() {
        std::vector<Particle> newParticles;
        newParticles.reserve(count);
        
        for (int i = 0; i < count; ++i) {
            Particle particle;
            particle.position = position;
            
            float angle = ((float)rand() / RAND_MAX) * 2.0f * M_PI;
            float velocityMagnitude = speed * (0.5f + ((float)rand() / RAND_MAX) * 0.5f);
            
            particle.velocity.x = cos(angle) * velocityMagnitude;
            particle.velocity.y = sin(angle) * velocityMagnitude;
            
            particle.color = color;
            particle.life = duration;
            particle.maxLife = duration;
            particle.size = 2.0f + ((float)rand() / RAND_MAX) * 3.0f;
            
            newParticles.push_back(particle);
        }
        
        {
            std::lock_guard<std::mutex> lock(pendingMutex);
            pendingParticles.push(std::move(newParticles));
        }
    });
}

void ParticleSystem::createDustParticle(Vector2 position, Vector2 velocity, float lifetime) {
    auto future = ParticleThreadPool::getInstance().getPool().enqueue([=]() {
        std::vector<Particle> newParticles;
        newParticles.reserve(1);
        
        Particle particle;
        particle.position = position;
        particle.velocity = velocity;
        particle.color = { 200, 200, 180, 180 };
        particle.life = lifetime;
        particle.maxLife = lifetime;
        particle.size = 4.0f;
        
        newParticles.push_back(particle);
        
        {
            std::lock_guard<std::mutex> lock(pendingMutex);
            pendingParticles.push(std::move(newParticles));
        }
    });
}

void ParticleSystem::createExplosionParticles(Vector2 position, int count, Color baseColor) {
    auto future = ParticleThreadPool::getInstance().getPool().enqueue([=]() {
        std::vector<Particle> newParticles;
        newParticles.reserve(count);
        
        for (int i = 0; i < count; ++i) {
            float angle = ((float)rand() / RAND_MAX) * 2.0f * M_PI;
            float speed = 50.0f + ((float)rand() / RAND_MAX) * 100.0f;
            
            Vector2 velocity = {
                (float)(cos(angle) * speed),
                (float)(sin(angle) * speed)
            };
            
            float lifetime = 0.3f + ((float)rand() / RAND_MAX) * 0.4f;
            float size = 1.0f + ((float)rand() / RAND_MAX) * 3.0f;
            
            Color particleColor = baseColor;
            particleColor.r = (unsigned char)std::max(0, std::min(255, (int)particleColor.r + (rand() % 40) - 20));
            particleColor.g = (unsigned char)std::max(0, std::min(255, (int)particleColor.g + (rand() % 40) - 20));
            particleColor.b = (unsigned char)std::max(0, std::min(255, (int)particleColor.b + (rand() % 40) - 20));
            
            Particle particle;
            particle.position = position;
            particle.velocity = velocity;
            particle.color = particleColor;
            particle.life = lifetime;
            particle.maxLife = lifetime;
            particle.size = size;
            
            newParticles.push_back(particle);
        }
        
        {
            std::lock_guard<std::mutex> lock(pendingMutex);
            pendingParticles.push(std::move(newParticles));
        }
    });
}

void ParticleSystem::processPendingParticles() {
    std::lock_guard<std::mutex> pendingLock(pendingMutex);
    while (!pendingParticles.empty()) {
        auto& newParticles = pendingParticles.front();
        particles.insert(particles.end(), newParticles.begin(), newParticles.end());
        pendingParticles.pop();
    }
}

void ParticleSystem::updateParticleRange(size_t start, size_t end, float deltaTime) {
    for (size_t i = start; i < end; ++i) {
        auto& particle = particles[i];
        particle.life -= deltaTime;
        
        particle.position.x += particle.velocity.x * deltaTime;
        particle.position.y += particle.velocity.y * deltaTime;
        
        particle.velocity.y += 200.0f * deltaTime;
        particle.velocity.x *= 0.98f;
        particle.velocity.y *= 0.98f;
        
        float alpha = particle.life / particle.maxLife;
        particle.color.a = (unsigned char)(255 * alpha);
    }
}

void ParticleSystem::update(float deltaTime) {
    processPendingParticles();
    
    if (particles.empty()) return;
    
    size_t numThreads = std::min(static_cast<size_t>(std::thread::hardware_concurrency()), particles.size() / 100 + 1);
    
    if (particles.size() < 500 || numThreads <= 1) {
        updateParticleRange(0, particles.size(), deltaTime);
    } else {
        size_t particlesPerThread = particles.size() / numThreads;
        std::vector<std::future<void>> futures;
        
        for (size_t t = 0; t < numThreads; ++t) {
            size_t start = t * particlesPerThread;
            size_t end = (t == numThreads - 1) ? particles.size() : start + particlesPerThread;
            
            futures.push_back(ParticleThreadPool::getInstance().getPool().enqueue([this, start, end, deltaTime]() {
                updateParticleRange(start, end, deltaTime);
            }));
        }
        
        for (auto& future : futures) {
            future.wait();
        }
    }
    
    particles.erase(
        std::remove_if(particles.begin(), particles.end(),
            [](const Particle& p) { return p.life <= 0.0f; }),
        particles.end()
    );
}

void ParticleSystem::draw() {
    std::lock_guard<std::mutex> lock(particleMutex);
    for (const auto& particle : particles) {
        DrawCircleV(particle.position, particle.size, particle.color);
    }
}

void ParticleSystem::clear() {
    std::lock_guard<std::mutex> lock(particleMutex);
    particles.clear();
    
    std::lock_guard<std::mutex> pendingLock(pendingMutex);
    while (!pendingParticles.empty()) {
        pendingParticles.pop();
    }
}
