#pragma once

#include "ui/UIController.hpp"
#include <raylib.h>
#include <atomic>
#include <thread>
#include <vector>

namespace UI {
    class LoadingScreenComponent : public UIComponent {
    public:
        LoadingScreenComponent(int w, int h);
        ~LoadingScreenComponent();

        void update(float dt) override;
        UIAction draw() override;
        void handleInput() override;
        void reset() override;

        void setProgress(float progress);
        bool isLoadingComplete() const;

    private:
        void updateBackgroundAnimations();

        // Loading bar constants
        static constexpr int LOADING_BAR_WIDTH = 500;
        static constexpr int LOADING_BAR_HEIGHT = 20;
        static constexpr int LOADING_TEXT_SIZE = 28;
        static constexpr int TITLE_FONT_SIZE = 48;
        
        // Animation constants
        static constexpr int NUM_HORIZONTAL_LINES = 12;
        static constexpr int NUM_VERTICAL_LINES = 8;

        struct BackgroundData {
            std::vector<float> hLinePositions;
            std::vector<float> vLinePositions;
            std::vector<Color> hLineColors;
            std::vector<Color> vLineColors;
            std::atomic<bool> ready{false};
        };

        float titleAnimTime = 0.0f;
        std::atomic<float> barAnimTime{0.0f};
        std::atomic<float> loadingProgress{0.0f};
        std::atomic<bool> loadingComplete{false};
        float timeAt100 = 0.0f; // Track time spent at 100%
        
        // Screen dimensions (immutable after construction)
        int screenWidth;
        int screenHeight;
        
        // Background animation
        BackgroundData bgData[2];
        int currentBgBuffer = 0;
        std::thread bgAnimThread;
        std::atomic<bool> threadRunning{true};

        // Colors matching title screen style
        static const Color COLOR_TITLE;
        static const Color COLOR_LOADING_BAR_BG;
        static const Color COLOR_LOADING_BAR_FILL;
        static const Color COLOR_LOADING_TEXT;
    };
}
