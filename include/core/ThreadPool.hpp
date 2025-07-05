#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>
#include <atomic>

class ThreadPool {
public:
    ThreadPool(size_t numThreads = 0);
    ~ThreadPool();

    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) 
        -> std::future<typename std::result_of<F(Args...)>::type>;

    void wait();
    void shutdown();
    size_t size() const { return workers.size(); }
    bool isShutdown() const { return stop.load(std::memory_order_acquire); }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    
    std::mutex queueMutex;
    std::condition_variable condition;
    std::condition_variable finished;
    std::atomic<size_t> activeTasks{0};
    std::atomic<bool> stop{false};
};

inline ThreadPool::ThreadPool(size_t numThreads) {
    if (numThreads == 0) {
        numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0) numThreads = 2;
    }
    
    for (size_t i = 0; i < numThreads; ++i) {
        workers.emplace_back([this] {
            for (;;) {
                std::function<void()> task;
                
                {
                    std::unique_lock<std::mutex> lock(this->queueMutex);
                    this->condition.wait(lock, [this] { 
                        return this->stop.load(std::memory_order_acquire) || !this->tasks.empty(); 
                    });
                    
                    if (this->stop.load(std::memory_order_acquire) && this->tasks.empty()) {
                        return;
                    }
                    
                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                    ++activeTasks;
                }
                
                try {
                    task();
                } catch (...) {
                }
                
                {
                    std::lock_guard<std::mutex> lock(this->queueMutex);
                    --activeTasks;
                }
                this->finished.notify_one();
            }
        });
    }
}

template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args) 
    -> std::future<typename std::result_of<F(Args...)>::type> {
    using return_type = typename std::result_of<F(Args...)>::type;
    
    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
    
    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        
        if (stop.load(std::memory_order_acquire)) {
            throw std::runtime_error("enqueue on stopped ThreadPool");
        }
        
        tasks.emplace([task](){ (*task)(); });
    }
    condition.notify_one();
    return res;
}

inline void ThreadPool::wait() {
    std::unique_lock<std::mutex> lock(queueMutex);
    finished.wait(lock, [this] { return tasks.empty() && activeTasks == 0; });
}

inline void ThreadPool::shutdown() {
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        stop.store(true, std::memory_order_release);
    }
    condition.notify_all();
    for (std::thread &worker: workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

inline ThreadPool::~ThreadPool() {
    shutdown();
}

#endif // THREAD_POOL_HPP
