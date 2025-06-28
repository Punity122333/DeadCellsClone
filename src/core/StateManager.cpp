#include "core/StateManager.hpp"
#include <raylib.h>
#include <algorithm>
#include <iostream>

namespace Core {

StateManager::StateManager() 
    : m_debugMode(false)
{
}

void StateManager::registerState(const std::string& stateId, StateFactory factory) {
    m_stateFactories[stateId] = std::move(factory);
}

void StateManager::pushState(const std::string& stateId, bool pauseCurrent) {
    if (!m_stateStack.empty() && pauseCurrent) {
        m_stateStack.back().isPaused = true;
        m_stateStack.back().state->onPause();
    }

    auto newState = createState(stateId);
    if (newState) {
        StateInfo info;
        info.state = std::move(newState);
        info.isPaused = false;
        info.stateId = stateId;
        
        m_stateStack.emplace_back(std::move(info));
        m_stateStack.back().state->onEnter();
        
        if (m_debugMode) {
            std::cout << "[StateManager] Pushed state: " << stateId << std::endl;
        }
        
        triggerTransition("", stateId);
    }
}

void StateManager::popState() {
    if (m_stateStack.empty()) {
        return;
    }

    std::string fromState = m_stateStack.back().stateId;
    m_stateStack.back().state->onExit();
    m_stateStack.pop_back();

    if (!m_stateStack.empty()) {
        m_stateStack.back().isPaused = false;
        m_stateStack.back().state->onResume();
        
        if (m_debugMode) {
            std::cout << "[StateManager] Popped state: " << fromState 
                      << ", resumed: " << m_stateStack.back().stateId << std::endl;
        }
        
        triggerTransition(fromState, m_stateStack.back().stateId);
    } else {
        if (m_debugMode) {
            std::cout << "[StateManager] Popped state: " << fromState 
                      << ", stack now empty" << std::endl;
        }
        
        triggerTransition(fromState, "");
    }
}

void StateManager::changeState(const std::string& stateId) {
    std::string fromState = "";
    if (!m_stateStack.empty()) {
        fromState = m_stateStack.back().stateId;
    }
    
    clearStates();
    pushState(stateId, false);
    
    triggerTransition(fromState, stateId);
}

void StateManager::clearStates() {
    while (!m_stateStack.empty()) {
        m_stateStack.back().state->onExit();
        m_stateStack.pop_back();
    }
    
    if (m_debugMode) {
        std::cout << "[StateManager] Cleared all states" << std::endl;
    }
}

void StateManager::update(float deltaTime) {
    if (m_stateStack.empty()) {
        return;
    }

    std::vector<StateInfo*> statesToUpdate;
    
    bool blocked = false;
    for (auto it = m_stateStack.rbegin(); it != m_stateStack.rend(); ++it) {
        StateInfo& stateInfo = *it;
        
        if (!blocked && !stateInfo.isPaused) {
            statesToUpdate.push_back(&stateInfo);
        }
        
        if (stateInfo.state->isBlocking()) {
            blocked = true;
        }
    }
    
    std::reverse(statesToUpdate.begin(), statesToUpdate.end());
    for (StateInfo* stateInfo : statesToUpdate) {
        stateInfo->state->update(deltaTime);
    }
}

void StateManager::render(float interpolation) {
    if (m_stateStack.empty()) {
        return;
    }

    std::vector<StateInfo*> statesToRender;
    
    bool blocked = false;
    for (auto it = m_stateStack.rbegin(); it != m_stateStack.rend(); ++it) {
        StateInfo& stateInfo = *it;
        
        if (!blocked) {
            statesToRender.push_back(&stateInfo);
        }
        
        if (stateInfo.state->isRenderBlocking()) {
            blocked = true;
        }
    }
    
    std::reverse(statesToRender.begin(), statesToRender.end());
    for (StateInfo* stateInfo : statesToRender) {
        stateInfo->state->render(interpolation);
    }
}

void StateManager::handleInput(float deltaTime) {
    if (m_stateStack.empty()) {
        return;
    }

    if (!m_stateStack.back().isPaused) {
        m_stateStack.back().state->handleInput(deltaTime);
    }
}

GameState* StateManager::getCurrentState() const {
    if (m_stateStack.empty()) {
        return nullptr;
    }
    return m_stateStack.back().state.get();
}

GameState* StateManager::getStateAtLevel(size_t level) const {
    if (level >= m_stateStack.size()) {
        return nullptr;
    }

    size_t index = m_stateStack.size() - 1 - level;
    return m_stateStack[index].state.get();
}

bool StateManager::hasState(const std::string& stateId) const {
    for (const auto& stateInfo : m_stateStack) {
        if (stateInfo.stateId == stateId) {
            return true;
        }
    }
    return false;
}

size_t StateManager::getStackSize() const {
    return m_stateStack.size();
}

void StateManager::setTransitionCallback(TransitionCallback callback) {
    m_transitionCallback = std::move(callback);
}

void StateManager::setDebugMode(bool enabled) {
    m_debugMode = enabled;
}

std::string StateManager::getStackDebugInfo() const {
    std::string info = "State Stack (top to bottom):\n";
    
    if (m_stateStack.empty()) {
        info += "  (empty)\n";
        return info;
    }
    
    for (auto it = m_stateStack.rbegin(); it != m_stateStack.rend(); ++it) {
        const StateInfo& stateInfo = *it;
        int level = std::distance(m_stateStack.rbegin(), it);
        info += "  [" + std::to_string(level) + "] " + stateInfo.stateId;
        if (stateInfo.isPaused) {
            info += " (PAUSED)";
        }
        info += "\n";
    }
    
    return info;
}

void StateManager::triggerTransition(const std::string& from, const std::string& to) {
    if (m_transitionCallback) {
        m_transitionCallback(from, to);
    }
}

std::unique_ptr<GameState> StateManager::createState(const std::string& stateId) {
    auto it = m_stateFactories.find(stateId);
    if (it != m_stateFactories.end()) {
        return it->second();
    }
    
    if (m_debugMode) {
        std::cerr << "[StateManager] ERROR: No factory registered for state: " << stateId << std::endl;
    }
    
    return nullptr;
}

} 
