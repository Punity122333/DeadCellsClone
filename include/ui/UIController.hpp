#pragma once

#include "Player.hpp"
#include <raylib.h>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <vector>

enum class GameState;

namespace UI {
    enum class ComponentType {
        TITLE_SCREEN,
        GAME_HUD,
        PAUSE_MENU,
        GAME_OVER,
        SETTINGS
    };

    enum class UIAction {
        NONE = 0,
        PLAY = 1,
        SETTINGS = 2,
        QUIT = 3,
        RESUME = 1,
        QUIT_TO_MENU = 3,
        RESTART = 4
    };

    struct UIDrawData {
        std::atomic<bool> ready{false};
        std::vector<float> animationParams;
        std::vector<Color> colors;
        float deltaTime = 0.0f;
        UIAction lastAction = UIAction::NONE;
    };

    class UIComponent {
    public:
        virtual ~UIComponent() = default;
        virtual void update(float dt) = 0;
        virtual UIAction draw() = 0;
        virtual void handleInput() = 0;
        virtual void reset() {}
    protected:
        int screenWidth, screenHeight;
        std::atomic<bool> active{false};
    };

    class UIController {
    public:
        UIController(int width, int height);
        ~UIController();
        
        void update(float dt, GameState currentState, const Player* player = nullptr);
        UIAction draw(GameState currentState, const Player* player = nullptr);
        void switchToComponent(ComponentType type);
        void reset();
        
    private:
        void initializeComponents();
        void updateAnimations();
        void handleBackgroundThread();
        
        int screenWidth, screenHeight;
        std::unordered_map<ComponentType, std::unique_ptr<UIComponent>> components;
        ComponentType activeComponent;
        
        std::thread backgroundThread;
        std::atomic<bool> threadRunning{true};
        std::mutex dataMutex;
        std::condition_variable dataCondition;
        
        UIDrawData drawData[2];
        int currentBuffer = 0;
        
        float globalAnimTime = 0.0f;
        std::atomic<bool> needsUpdate{false};
    };
}
