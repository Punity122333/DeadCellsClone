#pragma once
#include <memory>
#include <mutex>
#include <atomic>
#include "core/ThreadPool.hpp"

class GlobalThreadPool {
public:
    static GlobalThreadPool& getInstance();
    ThreadPool& getMainPool();
    void shutdown();
    void waitForAll();
    bool isShutdown() const;
    
private:
    GlobalThreadPool();
    ~GlobalThreadPool();
    GlobalThreadPool(const GlobalThreadPool&) = delete;
    GlobalThreadPool& operator=(const GlobalThreadPool&) = delete;
    
    std::unique_ptr<ThreadPool> mainPool;
    mutable std::mutex poolMutex;
    std::atomic<bool> shutdownFlag;
};
