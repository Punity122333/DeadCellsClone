#include "ui/TitleScreenComponent.hpp"
#include <raylib.h>
#include <cmath>
#include <cstring>

#ifndef CYAN
    #define CYAN CLITERAL(Color){ 0, 255, 255, 255 }
#endif

namespace UI {
    const char* TitleScreenComponent::BUTTON_LABELS[NUM_BUTTONS] = {"PLAY", "SETTINGS", "QUIT"};
    
    static const Color COLOR_TITLE = SKYBLUE;
    static const Color COLOR_BUTTON_HOVER = LIME;
    static const Color COLOR_BUTTON_PLAY = SKYBLUE;
    static const Color COLOR_BUTTON_SETTINGS = VIOLET;
    static const Color COLOR_BUTTON_QUIT = RED;
    static const Color COLOR_SELECTION = YELLOW;

    TitleScreenComponent::TitleScreenComponent(int w, int h) {
        screenWidth = w;
        screenHeight = h;
        
        for (int i = 0; i < 2; i++) {
            bgData[i].hLinePositions.resize(NUM_HORIZONTAL_LINES);
            bgData[i].vLinePositions.resize(NUM_VERTICAL_LINES);
            bgData[i].hLineColors.resize(NUM_HORIZONTAL_LINES);
            bgData[i].vLineColors.resize(NUM_VERTICAL_LINES);
        }
        
        bgAnimThread = std::thread(&TitleScreenComponent::updateBackgroundAnimations, this);
    }

    TitleScreenComponent::~TitleScreenComponent() {
        threadRunning = false;
        if (bgAnimThread.joinable()) {
            bgAnimThread.join();
        }
    }

    void TitleScreenComponent::updateBackgroundAnimations() {
        float localTitleAnimTime = 0.0f;
        
        while (threadRunning) {
            localTitleAnimTime += 0.016f;
            
            int nextBuffer = 1 - currentBgBuffer;
            auto& data = bgData[nextBuffer];
            
            for (int i = 0; i < NUM_HORIZONTAL_LINES; ++i) {
                data.hLinePositions[i] = std::sin(localTitleAnimTime * 0.8f + i * 0.4f) * 50.0f + static_cast<float>(screenHeight) * (i + 1) / static_cast<float>(NUM_HORIZONTAL_LINES + 1);
                
                float intensity = 0.3f + 0.4f * std::sin(localTitleAnimTime * 1.2f + i * 0.6f);
                data.hLineColors[i] = {
                    static_cast<unsigned char>(50 + intensity * 100),
                    static_cast<unsigned char>(150 + intensity * 105),
                    static_cast<unsigned char>(200 + intensity * 55),
                    static_cast<unsigned char>(60 + intensity * 120)
                };
            }
            
            for (int i = 0; i < NUM_VERTICAL_LINES; ++i) {
                data.vLinePositions[i] = std::cos(localTitleAnimTime * 0.6f + i * 0.5f) * 30.0f + static_cast<float>(screenWidth) * (i + 1) / static_cast<float>(NUM_VERTICAL_LINES + 1);
                
                float intensity = 0.2f + 0.3f * std::cos(localTitleAnimTime * 1.5f + i * 0.8f);
                data.vLineColors[i] = {
                    static_cast<unsigned char>(30 + intensity * 80),
                    static_cast<unsigned char>(100 + intensity * 100),
                    static_cast<unsigned char>(180 + intensity * 75),
                    static_cast<unsigned char>(40 + intensity * 100)
                };
            }
            
            data.ready = true;
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }

    void TitleScreenComponent::update(float dt) {
        titleAnimTime += dt;
        buttonAnimTime += dt;
        
        if (inputCooldownTimer > 0) {
            inputCooldownTimer -= dt;
        }
        
        handleInput();
        
        if (bgData[1 - currentBgBuffer].ready) {
            currentBgBuffer = 1 - currentBgBuffer;
            bgData[currentBgBuffer].ready = false;
        }
    }

    void TitleScreenComponent::handleInput() {
        if (inputCooldownTimer <= 0.0f) {
            if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
                selectedButton = (selectedButton - 1 + NUM_BUTTONS) % NUM_BUTTONS;
                inputCooldownTimer = INPUT_COOLDOWN;
            } else if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
                selectedButton = (selectedButton + 1) % NUM_BUTTONS;
                inputCooldownTimer = INPUT_COOLDOWN;
            }
        }
    }

    UIAction TitleScreenComponent::draw() {
        int nextBuffer = 1 - currentBgBuffer;
        if (bgData[nextBuffer].ready.load()) {
            currentBgBuffer = nextBuffer;
            bgData[currentBgBuffer].ready = false;
        }
        
        const BackgroundData& data = bgData[currentBgBuffer];
        
        for (int i = 0; i < NUM_HORIZONTAL_LINES; ++i) {
            DrawLine(0, static_cast<int>(data.hLinePositions[i]), screenWidth, static_cast<int>(data.hLinePositions[i]), data.hLineColors[i]);
        }
        
        for (int i = 0; i < NUM_VERTICAL_LINES; ++i) {
            DrawLine(static_cast<int>(data.vLinePositions[i]), 0, static_cast<int>(data.vLinePositions[i]), screenHeight, data.vLineColors[i]);
        }
        
        static const char* TITLE_TEXT = "Cellular Automata";
        static constexpr float TITLE_Y_POS = 350.0f;
        static constexpr float TITLE_Y_AMPLITUDE = 10.0f;
        static constexpr float GLOW_FREQUENCY = 1.2f;
        static constexpr float GLOW_BASE_SIZE = 6.0f;
        
        float glow = 0.7f + 0.3f * std::sin(titleAnimTime * GLOW_FREQUENCY);
        int titleWidth = MeasureText(TITLE_TEXT, TITLE_FONT_SIZE);
        
        float titleY = TITLE_Y_POS + TITLE_Y_AMPLITUDE * std::sin(titleAnimTime * 1.0f);
        int titleX = screenWidth / 2 - titleWidth / 2;
        
        for (int i = 0; i < static_cast<int>(GLOW_BASE_SIZE + glow * 2.0f); ++i) {
            DrawText(TITLE_TEXT, titleX + i, static_cast<int>(titleY) + i, TITLE_FONT_SIZE, ColorAlpha(COLOR_TITLE, 0.08f));
        }
        DrawText(TITLE_TEXT, titleX, static_cast<int>(titleY), TITLE_FONT_SIZE, COLOR_TITLE);
        
        float buttonStartY = titleY + 150.0f;
        static const Color buttonColors[] = { COLOR_BUTTON_PLAY, COLOR_BUTTON_SETTINGS, COLOR_BUTTON_QUIT };
        
        for (int i = 0; i < NUM_BUTTONS; ++i) {
            float buttonY = buttonStartY + i * (BUTTON_HEIGHT + 30);
            int buttonX = screenWidth / 2 - BUTTON_WIDTH / 2;
            
            Color buttonColor = buttonColors[i];
            if (i == selectedButton) {
                float pulse = 0.85f + 0.15f * std::sin(buttonAnimTime * 2.5f);
                DrawRectangle(buttonX - 4, static_cast<int>(buttonY) - 4, BUTTON_WIDTH + 8, BUTTON_HEIGHT + 8, ColorAlpha(COLOR_SELECTION, pulse));
            }
            
            DrawRectangle(buttonX, static_cast<int>(buttonY), BUTTON_WIDTH, BUTTON_HEIGHT, ColorAlpha(buttonColor, 0.3f));
            DrawRectangleLines(buttonX, static_cast<int>(buttonY), BUTTON_WIDTH, BUTTON_HEIGHT, buttonColor);
            
            int textWidth = MeasureText(BUTTON_LABELS[i], BUTTON_FONT_SIZE);
            int textX = buttonX + BUTTON_WIDTH / 2 - textWidth / 2;
            int textY = static_cast<int>(buttonY) + BUTTON_HEIGHT / 2 - BUTTON_FONT_SIZE / 2;
            
            DrawText(BUTTON_LABELS[i], textX, textY, BUTTON_FONT_SIZE, WHITE);
        }
        
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
            return static_cast<UIAction>(selectedButton + 1);
        }
        
        return UIAction::NONE;
    }

    void TitleScreenComponent::reset() {
        titleAnimTime = 0.0f;
        buttonAnimTime = 0.0f;
        selectedButton = 0;
        inputCooldownTimer = 0.0f;
    }
}
