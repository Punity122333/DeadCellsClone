#pragma once
#include <raylib.h>
#include <thread>
#include <vector>
#include <atomic>

class TitleScreenUI {
public:
    TitleScreenUI(int w, int h);
    ~TitleScreenUI();
    void update(float dt);
    int draw();
private:
    static constexpr int NUM_HORIZONTAL_LINES = 20;
    static constexpr int NUM_VERTICAL_LINES = 10;
    static constexpr int NUM_GLOW_EFFECTS = 6;
    static constexpr int NUM_BUTTONS = 3;
    static constexpr int TITLE_FONT_SIZE = 90;
    static constexpr int BUTTON_FONT_SIZE = 36;
    static constexpr int BUTTON_WIDTH = 340;
    static constexpr int BUTTON_HEIGHT = 64;
    static constexpr int PADDING_BELOW_TITLE = 80;
    static constexpr int BUTTON_SPACING = 30;
    static constexpr float INPUT_COOLDOWN = 0.2f;
    static constexpr float SELECTION_BORDER_WIDTH = 4.0f;
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
    int screenWidth, screenHeight;
    float titleAnimTime = 0.0f;
    float buttonAnimTime = 0.0f;
    int selectedButton = 0;
    float inputCooldownTimer = 0.0f;
};
