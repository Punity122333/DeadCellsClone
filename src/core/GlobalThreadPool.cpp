#include "core/GlobalThreadPool.hpp"
#include <thread>
#include <algorithm>

GlobalThreadPool& GlobalThreadPool::getInstance() {
    static GlobalThreadPool instance;
    return instance;
}

GlobalThreadPool::GlobalThreadPool() : shutdownFlag(false) {
    size_t numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4;
    numThreads = std::max(static_cast<size_t>(2), std::min(numThreads, static_cast<size_t>(8)));
    mainPool = std::make_unique<ThreadPool>(numThreads);
}

GlobalThreadPool::~GlobalThreadPool() {
    shutdown();
}

ThreadPool& GlobalThreadPool::getMainPool() {
    std::lock_guard<std::mutex> lock(poolMutex);
    if (shutdownFlag.load(std::memory_order_acquire)) {
        throw std::runtime_error("GlobalThreadPool is shut down");
    }
    return *mainPool;
}

void GlobalThreadPool::shutdown() {
    std::lock_guard<std::mutex> lock(poolMutex);
    if (!shutdownFlag.exchange(true, std::memory_order_acq_rel)) {
        if (mainPool) {
            mainPool->wait();
            mainPool.reset();
        }
    }
}

void GlobalThreadPool::waitForAll() {
    std::lock_guard<std::mutex> lock(poolMutex);
    if (mainPool && !shutdownFlag.load(std::memory_order_acquire)) {
        mainPool->wait();
    }
}

bool GlobalThreadPool::isShutdown() const {
    return shutdownFlag.load(std::memory_order_acquire);
}
