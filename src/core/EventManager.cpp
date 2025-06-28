#include "core/EventManager.hpp"
#include <iostream>
#include <chrono>

namespace Core {

EventManager::EventManager() 
    : m_nextSubscriptionId(1)
    , m_eventsQueued(0)
    , m_eventsProcessed(0)
    , m_loggingEnabled(false)
    , m_totalProcessingTime(0.0f)
    , m_processingSamples(0)
{
}

void EventManager::processEvents(int maxEvents) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto startTime = std::chrono::steady_clock::now();
    
    int processed = 0;
    while (!m_eventQueue.empty() && (maxEvents == 0 || processed < maxEvents)) {
        QueuedEvent queuedEvent = std::move(const_cast<QueuedEvent&>(m_eventQueue.top()));
        m_eventQueue.pop();
        
        // Apply event filter if set
        if (m_eventFilter && !m_eventFilter(*queuedEvent.event)) {
            continue;
        }
        
        auto eventType = queuedEvent.event->getType();
        auto typeIt = m_subscribers.find(eventType);
        
        if (typeIt != m_subscribers.end()) {
            for (auto& [id, handler] : typeIt->second) {
                try {
                    handler(*queuedEvent.event);
                } catch (const std::exception& e) {
                    if (m_errorCallback) {
                        m_errorCallback(std::string("Event handler error: ") + e.what());
                    }
                }
            }
        }
        
        if (m_loggingEnabled) {
            std::cout << "[EventManager] Processed event type: " << eventType.name() << std::endl;
        }
        
        m_eventsProcessed++;
        processed++;
    }
    
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration<float, std::milli>(endTime - startTime).count();
    
    // Update performance metrics
    m_totalProcessingTime += duration;
    m_processingSamples++;
}

void EventManager::clearQueue() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Clear the priority queue
    std::priority_queue<QueuedEvent> empty;
    m_eventQueue.swap(empty);
    
    if (m_loggingEnabled) {
        std::cout << "[EventManager] Cleared event queue" << std::endl;
    }
}

void EventManager::setEventFilter(EventFilter filter) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_eventFilter = std::move(filter);
}

void EventManager::setErrorCallback(std::function<void(const std::string&)> callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_errorCallback = std::move(callback);
}

void EventManager::setLoggingEnabled(bool enabled) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_loggingEnabled = enabled;
}

EventManager::EventStats EventManager::getStats() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    EventStats stats;
    stats.eventsQueued = m_eventsQueued;
    stats.eventsProcessed = m_eventsProcessed;
    stats.queueSize = m_eventQueue.size();
    
    // Count total subscribers
    for (const auto& [type, handlers] : m_subscribers) {
        stats.subscriberCount += handlers.size();
    }
    
    // Calculate average processing time
    if (m_processingSamples > 0) {
        stats.averageProcessingTime = m_totalProcessingTime / m_processingSamples;
    }
    
    return stats;
}

void EventManager::clearAllSubscribers() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_subscribers.clear();
    
    if (m_loggingEnabled) {
        std::cout << "[EventManager] Cleared all subscribers" << std::endl;
    }
}

} // namespace Core
