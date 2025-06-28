#pragma once

#include "ui/UIController.hpp"
#include <raylib.h>
#include <atomic>
#include <thread>
#include <vector>

namespace UI {
    class PauseMenuComponent : public UIComponent {
    public:
        PauseMenuComponent(int w, int h);
        ~PauseMenuComponent();
        
        void update(float dt) override;
        UIAction draw() override;
        void handleInput() override;
        void reset() override;
        
    private:
        static constexpr int NUM_HORIZONTAL_LINES = 20;
        static constexpr int NUM_VERTICAL_LINES = 10;
        static constexpr int NUM_BUTTONS = 3;
        static constexpr int TITLE_FONT_SIZE = 70;
        static constexpr int BUTTON_FONT_SIZE = 36;
        static constexpr int BUTTON_WIDTH = 340;
        static constexpr int BUTTON_HEIGHT = 64;
        static constexpr float INPUT_COOLDOWN = 0.2f;
        
        struct BackgroundData {
            std::vector<float> hLinePositions;
            std::vector<float> vLinePositions;
            std::vector<Color> hLineColors;
            std::vector<Color> vLineColors;
            std::atomic<bool> ready{false};
        };
        
        void updateBackgroundAnimations();
        
        BackgroundData bgData[2];
        int currentBgBuffer = 0;
        std::thread bgAnimThread;
        std::atomic<bool> threadRunning{true};
        
        float titleAnimTime = 0.0f;
        float buttonAnimTime = 0.0f;
        int selectedButton = 0;
        float inputCooldownTimer = 0.0f;
        
        static const char* BUTTON_LABELS[NUM_BUTTONS];
    };
}
