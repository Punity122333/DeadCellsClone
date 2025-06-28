#ifndef CORE_STATEMANAGER_HPP
#define CORE_STATEMANAGER_HPP

#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <functional>

namespace Core {

class GameState {
public:
    virtual ~GameState() = default;
    virtual void onEnter() {}
    virtual void onExit() {}
    virtual void onPause() {}
    virtual void onResume() {}
    virtual void update(float deltaTime) = 0;
    virtual void render(float interpolation) = 0;
    virtual void handleInput(float deltaTime) {}
    virtual bool isBlocking() const { return true; }
    virtual bool isRenderBlocking() const { return true; }
    virtual const std::string& getStateId() const = 0;
};

class StateManager {
public:
    using StateFactory = std::function<std::unique_ptr<GameState>()>;
    using TransitionCallback = std::function<void(const std::string& from, const std::string& to)>;

    StateManager();
    ~StateManager() = default;

    void registerState(const std::string& stateId, StateFactory factory);
    void pushState(const std::string& stateId, bool pauseCurrent = true);
    void popState();
    void changeState(const std::string& stateId);
    void clearStates();
    void update(float deltaTime);
    void render(float interpolation);
    void handleInput(float deltaTime);
    GameState* getCurrentState() const;
    GameState* getStateAtLevel(size_t level) const;
    bool hasState(const std::string& stateId) const;
    size_t getStackSize() const;
    void setTransitionCallback(TransitionCallback callback);
    void setDebugMode(bool enabled);
    std::string getStackDebugInfo() const;

private:
    struct StateInfo {
        std::unique_ptr<GameState> state;
        bool isPaused;
        std::string stateId;
        
        StateInfo() = default;
        StateInfo(const StateInfo&) = delete;
        StateInfo& operator=(const StateInfo&) = delete;
        StateInfo(StateInfo&&) = default;
        StateInfo& operator=(StateInfo&&) = default;
    };

    std::vector<StateInfo> m_stateStack;
    std::unordered_map<std::string, StateFactory> m_stateFactories;
    TransitionCallback m_transitionCallback;
    bool m_debugMode;

    void triggerTransition(const std::string& from, const std::string& to);
    std::unique_ptr<GameState> createState(const std::string& stateId);
};

} // namespace Core

#endif // CORE_STATEMANAGER_HPP
