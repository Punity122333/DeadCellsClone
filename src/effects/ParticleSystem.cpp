#include "effects/ParticleSystem.hpp"
#include <cmath>
#include <cstdlib>
#include <thread>
#include <algorithm>

ParticleSystem& ParticleSystem::getInstance() {
    static ParticleSystem instance;
    return instance;
}

void ParticleSystem::createExplosion(Vector2 position, int count, Color color, float duration, float speed) {
    std::lock_guard<std::mutex> lock(particleMutex);
    for (int i = 0; i < count; ++i) {
        Particle particle;
        particle.position = position;
        
        // Random angle for explosion
        float angle = ((float)rand() / RAND_MAX) * 2.0f * PI;
        float velocityMagnitude = speed * (0.5f + ((float)rand() / RAND_MAX) * 0.5f);
        
        particle.velocity.x = cos(angle) * velocityMagnitude;
        particle.velocity.y = sin(angle) * velocityMagnitude;
        
        particle.color = color;
        particle.life = duration;
        particle.maxLife = duration;
        particle.size = 2.0f + ((float)rand() / RAND_MAX) * 3.0f;
        
        particles.push_back(particle);
    }
}

void ParticleSystem::createDustParticle(Vector2 position, Vector2 velocity, float lifetime) {
    std::lock_guard<std::mutex> lock(particleMutex);
    Particle particle;
    particle.position = position;
    particle.velocity = velocity;
    particle.color = { 200, 200, 180, 180 };
    particle.life = lifetime;
    particle.maxLife = lifetime;
    particle.size = 4.0f;
    particles.push_back(particle);
}

void ParticleSystem::createExplosionParticles(Vector2 position, int count, Color baseColor) {
    std::lock_guard<std::mutex> lock(particleMutex);
    for (int i = 0; i < count; ++i) {
        // Random angle for explosion
        float angle = ((float)rand() / RAND_MAX) * 2.0f * PI;
        float speed = 50.0f + ((float)rand() / RAND_MAX) * 100.0f;
        
        Vector2 velocity = {
            (float)(cos(angle) * speed),
            (float)(sin(angle) * speed)
        };
        
        float lifetime = 0.3f + ((float)rand() / RAND_MAX) * 0.4f;
        float size = 1.0f + ((float)rand() / RAND_MAX) * 3.0f;
        
        Color particleColor = baseColor;
        // Add some variation to the color
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
        
        particles.push_back(particle);
    }
}

void ParticleSystem::update(float deltaTime) {
    std::lock_guard<std::mutex> lock(particleMutex);
    
    if (particles.empty()) return;
    
    // Multithreaded particle update
    size_t numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 2;
    
    if (particles.size() < numThreads * 10) {
        // For small particle counts, use single thread to avoid overhead
        for (auto& particle : particles) {
            particle.life -= deltaTime;
            
            // Update position
            particle.position.x += particle.velocity.x * deltaTime;
            particle.position.y += particle.velocity.y * deltaTime;
            
            // Apply gravity and friction
            particle.velocity.y += 200.0f * deltaTime; // gravity
            particle.velocity.x *= 0.98f; // friction
            particle.velocity.y *= 0.98f;
            
            // Fade out over time
            float alpha = particle.life / particle.maxLife;
            particle.color.a = (unsigned char)(255 * alpha);
        }
    } else {
        // Use multithreading for large particle counts
        size_t particlesPerThread = particles.size() / numThreads;
        std::vector<std::thread> threads;
        
        for (size_t t = 0; t < numThreads; ++t) {
            size_t start = t * particlesPerThread;
            size_t end = (t == numThreads - 1) ? particles.size() : start + particlesPerThread;
            
            threads.emplace_back([&, start, end, deltaTime]() {
                for (size_t i = start; i < end; ++i) {
                    auto& particle = particles[i];
                    particle.life -= deltaTime;
                    
                    // Update position
                    particle.position.x += particle.velocity.x * deltaTime;
                    particle.position.y += particle.velocity.y * deltaTime;
                    
                    // Apply gravity and friction
                    particle.velocity.y += 200.0f * deltaTime; // gravity
                    particle.velocity.x *= 0.98f; // friction
                    particle.velocity.y *= 0.98f;
                    
                    // Fade out over time
                    float alpha = particle.life / particle.maxLife;
                    particle.color.a = (unsigned char)(255 * alpha);
                }
            });
        }
        
        // Wait for all threads to complete
        for (auto& thread : threads) {
            thread.join();
        }
    }
    
    // Remove dead particles
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
}
