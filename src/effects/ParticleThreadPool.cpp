#include "effects/ParticleThreadPool.hpp"
#include <thread>
#include <algorithm>

ParticleThreadPool& ParticleThreadPool::getInstance() {
    static ParticleThreadPool instance;
    return instance;
}

ParticleThreadPool::ParticleThreadPool() : shutdownFlag(false) {
    size_t numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 2;
    numThreads = std::min(numThreads, static_cast<size_t>(4));
    pool = std::make_unique<ThreadPool>(numThreads);
}

ParticleThreadPool::~ParticleThreadPool() {
    shutdown();
}

ThreadPool& ParticleThreadPool::getPool() {
    std::lock_guard<std::mutex> lock(poolMutex);
    if (shutdownFlag.load(std::memory_order_acquire)) {
        throw std::runtime_error("ParticleThreadPool is shut down");
    }
    return *pool;
}

void ParticleThreadPool::shutdown() {
    std::lock_guard<std::mutex> lock(poolMutex);
    if (!shutdownFlag.exchange(true, std::memory_order_acq_rel)) {
        if (pool) {
            pool->wait();
            pool.reset();
        }
    }
}

void ParticleThreadPool::waitForAll() {
    std::lock_guard<std::mutex> lock(poolMutex);
    if (pool && !shutdownFlag.load(std::memory_order_acquire)) {
        pool->wait();
    }
}

bool ParticleThreadPool::isShutdown() const {
    return shutdownFlag.load(std::memory_order_acquire);
}
