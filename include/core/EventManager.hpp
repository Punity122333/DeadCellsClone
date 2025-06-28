#ifndef CORE_EVENTMANAGER_HPP
#define CORE_EVENTMANAGER_HPP

#include <functional>
#include <unordered_map>
#include <queue>
#include <typeindex>
#include <memory>
#include <mutex>
#include <string>
#include <chrono>

namespace Core {

class Event {
public:
    virtual ~Event() = default;
    virtual std::type_index getType() const = 0;
};

template<typename T>
class TypedEvent : public Event {
public:
    std::type_index getType() const override {
        return std::type_index(typeid(T));
    }
};


struct PlayerHealthChangedEvent : public TypedEvent<PlayerHealthChangedEvent> {
    int oldHealth;
    int newHealth;
    int maxHealth;
};

struct EnemyDefeatedEvent : public TypedEvent<EnemyDefeatedEvent> {
    std::string enemyType;
    float positionX;
    float positionY;
    int scoreValue;
};

struct WeaponSwitchedEvent : public TypedEvent<WeaponSwitchedEvent> {
    int oldWeaponIndex;
    int newWeaponIndex;
    std::string weaponName;
};

struct LevelCompletedEvent : public TypedEvent<LevelCompletedEvent> {
    int levelIndex;
    float completionTime;
    int enemiesDefeated;
};

struct GameStateChangedEvent : public TypedEvent<GameStateChangedEvent> {
    std::string previousState;
    std::string newState;
};

struct AudioEvent : public TypedEvent<AudioEvent> {
    enum Type { PLAY_SOUND, PLAY_MUSIC, STOP_MUSIC, SET_VOLUME };
    Type type;
    std::string audioPath;
    float volume = 1.0f;
    bool loop = false;
};


class EventManager {
public:
    using EventHandler = std::function<void(const Event&)>;
    using EventFilter = std::function<bool(const Event&)>;

    EventManager();
    ~EventManager() = default;

    
    template<typename T>
    int subscribe(std::function<void(const T&)> handler) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        int id = m_nextSubscriptionId++;
        auto typeIndex = std::type_index(typeid(T));
        
        auto wrappedHandler = [handler](const Event& event) {
            const T& typedEvent = static_cast<const T&>(event);
            handler(typedEvent);
        };
        
        m_subscribers[typeIndex][id] = wrappedHandler;
        return id;
    }

    
    template<typename T>
    void unsubscribe(int subscriptionId) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto typeIndex = std::type_index(typeid(T));
        auto typeIt = m_subscribers.find(typeIndex);
        if (typeIt != m_subscribers.end()) {
            typeIt->second.erase(subscriptionId);
        }
    }

    
    template<typename T>
    void queueEvent(const T& event, int priority = 0) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        auto eventCopy = std::make_unique<T>(event);
        QueuedEvent queuedEvent{
            .event = std::move(eventCopy),
            .priority = priority,
            .timestamp = std::chrono::steady_clock::now()
        };
        
        m_eventQueue.push(std::move(queuedEvent));
        m_eventsQueued++;
    }

    
    template<typename T>
    void dispatchEvent(const T& event) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        auto typeIndex = std::type_index(typeid(T));
        auto typeIt = m_subscribers.find(typeIndex);
        
        if (typeIt != m_subscribers.end()) {
            for (auto& [id, handler] : typeIt->second) {
                try {
                    handler(event);
                    m_eventsProcessed++;
                } catch (const std::exception& e) {
                    // Log error but continue processing
                    if (m_errorCallback) {
                        m_errorCallback(std::string("Event handler error: ") + e.what());
                    }
                }
            }
        }
    }

    
    void processEvents(int maxEvents = 0);

    
    void clearQueue();

    
    void setEventFilter(EventFilter filter);

    
    void setErrorCallback(std::function<void(const std::string&)> callback);

    
    void setLoggingEnabled(bool enabled);

    
    struct EventStats {
        size_t eventsQueued = 0;
        size_t eventsProcessed = 0;
        size_t subscriberCount = 0;
        size_t queueSize = 0;
        float averageProcessingTime = 0.0f;
    };
    
    EventStats getStats() const;

    
    void clearAllSubscribers();

   
    template<typename T>
    size_t getSubscriberCount() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto typeIndex = std::type_index(typeid(T));
        auto typeIt = m_subscribers.find(typeIndex);
        return typeIt != m_subscribers.end() ? typeIt->second.size() : 0;
    }

private:
    struct QueuedEvent {
        std::unique_ptr<Event> event;
        int priority;
        std::chrono::steady_clock::time_point timestamp;
        
        bool operator<(const QueuedEvent& other) const {
            return priority < other.priority; 
        }
    };

    mutable std::mutex m_mutex;
    std::unordered_map<std::type_index, std::unordered_map<int, EventHandler>> m_subscribers;
    std::priority_queue<QueuedEvent> m_eventQueue;
    
    int m_nextSubscriptionId;
    size_t m_eventsQueued;
    size_t m_eventsProcessed;
    bool m_loggingEnabled;
    
    EventFilter m_eventFilter;
    std::function<void(const std::string&)> m_errorCallback;
    
    std::chrono::steady_clock::time_point m_lastProcessTime;
    float m_totalProcessingTime;
    size_t m_processingSamples;
};

EventManager& GetEventManager();

} 
#endif 
