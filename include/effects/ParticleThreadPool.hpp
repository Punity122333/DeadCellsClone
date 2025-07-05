#pragma once
#include <memory>
#include <mutex>
#include <atomic>
#include "core/ThreadPool.hpp"

class ParticleThreadPool {
public:
    static ParticleThreadPool& getInstance();
    ThreadPool& getPool();
    void shutdown();
    void waitForAll();
    bool isShutdown() const;
    
private:
    ParticleThreadPool();
    ~ParticleThreadPool();
    ParticleThreadPool(const ParticleThreadPool&) = delete;
    ParticleThreadPool& operator=(const ParticleThreadPool&) = delete;
    
    std::unique_ptr<ThreadPool> pool;
    mutable std::mutex poolMutex;
    std::atomic<bool> shutdownFlag;
};
