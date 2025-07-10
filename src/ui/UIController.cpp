#include "ui/UIController.hpp"
#include "ui/TitleScreenComponent.hpp"
#include "ui/PauseMenuComponent.hpp"
#include "ui/GameHUDComponent.hpp"
#include "ui/GameOverComponent.hpp"
#include "ui/LoadingScreenComponent.hpp"
#include "Game.hpp"

#include <chrono>

namespace UI {
    UIController::UIController(int width, int height) 
        : screenWidth(width), screenHeight(height), activeComponent(ComponentType::LOADING_SCREEN) {
        initializeComponents();
        backgroundThread = std::thread(&UIController::handleBackgroundThread, this);
    }

    UIController::~UIController() {
        threadRunning = false;
        dataCondition.notify_all();
        if (backgroundThread.joinable()) {
            backgroundThread.join();
        }
    }

    void UIController::initializeComponents() {
        components[ComponentType::LOADING_SCREEN] = std::make_unique<LoadingScreenComponent>(screenWidth, screenHeight);
        components[ComponentType::TITLE_SCREEN] = std::make_unique<TitleScreenComponent>(screenWidth, screenHeight);
        components[ComponentType::PAUSE_MENU] = std::make_unique<PauseMenuComponent>(screenWidth, screenHeight);
        components[ComponentType::GAME_HUD] = std::make_unique<GameHUDComponent>(screenWidth, screenHeight);
        components[ComponentType::GAME_OVER] = std::make_unique<GameOverComponent>(screenWidth, screenHeight);
    }

    void UIController::update(float dt, GameState currentState, const Player* player) {
        globalAnimTime += dt;
        needsUpdate = true;
        
        ComponentType targetComponent;
        switch (currentState) {
            case GameState::LOADING:
                targetComponent = ComponentType::LOADING_SCREEN;
                break;
            case GameState::TITLE:
                targetComponent = ComponentType::TITLE_SCREEN;
                break;
            case GameState::PLAYING:
                targetComponent = ComponentType::GAME_HUD;
                break;
            case GameState::GAME_OVER:
                targetComponent = ComponentType::GAME_OVER;
                break;
            case GameState::PAUSED:
                targetComponent = ComponentType::PAUSE_MENU;
                break;
            default:
                targetComponent = ComponentType::LOADING_SCREEN;
                break;
        }
        
        if (targetComponent != activeComponent) {
            printf("[UIController] Switching from component %d to %d\n", static_cast<int>(activeComponent), static_cast<int>(targetComponent));
            switchToComponent(targetComponent);
        }
        
        if (components[activeComponent]) {
            components[activeComponent]->update(dt);
        }
        
        lastDeltaTime = dt; 
        
        dataCondition.notify_one();
    }

    UIAction UIController::draw(GameState currentState, const Player* player, const Map* map) {
        UIAction action = UIAction::NONE;
        
        if (components[activeComponent]) {
            if (activeComponent == ComponentType::GAME_HUD && player && map) {
                static_cast<GameHUDComponent*>(components[activeComponent].get())->drawHUD(*player, *map, currentState, lastDeltaTime);
            } else if (activeComponent == ComponentType::GAME_HUD && player) {
                static_cast<GameHUDComponent*>(components[activeComponent].get())->drawHUD(*player, currentState);
            } else {
                action = components[activeComponent]->draw();
            }
        }
        
        return action;
    }

    void UIController::switchToComponent(ComponentType type) {
        if (components.find(type) != components.end()) {
            activeComponent = type;
            if (components[activeComponent]) {
                components[activeComponent]->reset();
            }
        }
    }

    void UIController::reset() {
        for (auto& [type, component] : components) {
            if (component) {
                component->reset();
            }
        }
        globalAnimTime = 0.0f;
    }

    void UIController::handleBackgroundThread() {
        while (threadRunning) {
            std::unique_lock<std::mutex> lock(dataMutex);
            dataCondition.wait(lock, [this] { return needsUpdate.load() || !threadRunning.load(); });
            
            if (!threadRunning) break;
            
            updateAnimations();
            needsUpdate = false;
            
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }

    void UIController::updateAnimations() {
        int nextBuffer = 1 - currentBuffer;
        auto& data = drawData[nextBuffer];
        
        data.animationParams.clear();
        data.colors.clear();
        
        float time = globalAnimTime;
        
        for (int i = 0; i < 20; ++i) {
            data.animationParams.push_back(std::sin(time * 0.5f + i * 0.3f));
        }
        
        for (int i = 0; i < 10; ++i) {
            float intensity = 0.5f + 0.5f * std::sin(time * 2.0f + i * 0.7f);
            data.colors.push_back({
                static_cast<unsigned char>(50 + intensity * 50),
                static_cast<unsigned char>(150 + intensity * 50),
                static_cast<unsigned char>(200 + intensity * 55),
                static_cast<unsigned char>(100 + intensity * 155)
            });
        }
        
        data.deltaTime = GetFrameTime();
        data.ready = true;
        
        currentBuffer = nextBuffer;
    }
    
    void UIController::setLoadingProgress(float progress) {
        if (components.find(ComponentType::LOADING_SCREEN) != components.end()) {
            auto* loadingScreen = static_cast<LoadingScreenComponent*>(components[ComponentType::LOADING_SCREEN].get());
            loadingScreen->setProgress(progress);
        }
    }
    
    LoadingScreenComponent* UIController::getLoadingScreen() {
        auto it = components.find(ComponentType::LOADING_SCREEN);
        if (it != components.end()) {
            return static_cast<LoadingScreenComponent*>(it->second.get());
        }
        return nullptr;
    }
}
