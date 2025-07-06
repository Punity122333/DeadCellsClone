#include "ui/PauseMenuComponent.hpp"
#include <raylib.h>
#include <cmath>
#include <cstring>

#ifndef CYAN
    #define CYAN CLITERAL(Color){ 0, 255, 255, 255 }
#endif

namespace UI {
    const char* PauseMenuComponent::BUTTON_LABELS[NUM_BUTTONS] = {"RESUME", "SETTINGS", "QUIT TO MENU"};
    
    static const Color COLOR_TITLE = ORANGE;
    static const Color COLOR_BUTTON_HOVER = LIME;
    static const Color COLOR_BUTTON_RESUME = GREEN;
    static const Color COLOR_BUTTON_SETTINGS = VIOLET;
    static const Color COLOR_BUTTON_QUIT = RED;
    static const Color COLOR_SELECTION = YELLOW;

    PauseMenuComponent::PauseMenuComponent(int w, int h) {
        screenWidth = w;
        screenHeight = h;
        
        for (int i = 0; i < 2; i++) {
            bgData[i].hLinePositions.resize(NUM_HORIZONTAL_LINES);
            bgData[i].vLinePositions.resize(NUM_VERTICAL_LINES);
            bgData[i].hLineColors.resize(NUM_HORIZONTAL_LINES);
            bgData[i].vLineColors.resize(NUM_VERTICAL_LINES);
        }
        
        bgAnimThread = std::thread(&PauseMenuComponent::updateBackgroundAnimations, this);
    }

    PauseMenuComponent::~PauseMenuComponent() {
        threadRunning = false;
        if (bgAnimThread.joinable()) {
            bgAnimThread.join();
        }
    }

    void PauseMenuComponent::updateBackgroundAnimations() {
        float localTitleAnimTime = 0.0f;
        
        while (threadRunning) {
            localTitleAnimTime += 0.016f;
            
            int nextBuffer = 1 - currentBgBuffer;
            auto& data = bgData[nextBuffer];
            
            for (int i = 0; i < NUM_HORIZONTAL_LINES; ++i) {
                data.hLinePositions[i] = std::sin(localTitleAnimTime * 0.6f + i * 0.5f) * 40.0f + static_cast<float>(screenHeight) * (i + 1) / (NUM_HORIZONTAL_LINES + 1);
                
                float intensity = 0.4f + 0.3f * std::sin(localTitleAnimTime * 1.0f + i * 0.7f);
                data.hLineColors[i] = {
                    static_cast<unsigned char>(80 + intensity * 80),
                    static_cast<unsigned char>(40 + intensity * 60),
                    static_cast<unsigned char>(20 + intensity * 40),
                    static_cast<unsigned char>(80 + intensity * 100)
                };
            }
            
            for (int i = 0; i < NUM_VERTICAL_LINES; ++i) {
                data.vLinePositions[i] = std::cos(localTitleAnimTime * 0.7f + i * 0.6f) * 25.0f + static_cast<float>(screenWidth) * (i + 1) / static_cast<float>(NUM_VERTICAL_LINES + 1);
                
                float intensity = 0.3f + 0.4f * std::cos(localTitleAnimTime * 1.3f + i * 0.9f);
                data.vLineColors[i] = {
                    static_cast<unsigned char>(60 + intensity * 70),
                    static_cast<unsigned char>(30 + intensity * 50),
                    static_cast<unsigned char>(10 + intensity * 30),
                    static_cast<unsigned char>(60 + intensity * 80)
                };
            }
            
            data.ready = true;
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }

    void PauseMenuComponent::update(float dt) {
        titleAnimTime += dt;
        buttonAnimTime += dt;
        inputCooldownTimer -= dt;
        
        handleInput();
        
        if (bgData[1 - currentBgBuffer].ready) {
            currentBgBuffer = 1 - currentBgBuffer;
            bgData[currentBgBuffer].ready = false;
        }
    }

    void PauseMenuComponent::handleInput() {
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

    UIAction PauseMenuComponent::draw() {
        DrawRectangle(0, 0, screenWidth, screenHeight, ColorAlpha(BLACK, 0.7f));
        
        const BackgroundData& data = bgData[currentBgBuffer];
        
        for (int i = 0; i < NUM_HORIZONTAL_LINES; ++i) {
            DrawLine(0, static_cast<int>(data.hLinePositions[i]), screenWidth, static_cast<int>(data.hLinePositions[i]), data.hLineColors[i]);
        }
        
        for (int i = 0; i < NUM_VERTICAL_LINES; ++i) {
            DrawLine(static_cast<int>(data.vLinePositions[i]), 0, static_cast<int>(data.vLinePositions[i]), screenHeight, data.vLineColors[i]);
        }
        
        static const char* TITLE_TEXT = "PAUSED";
        static constexpr float TITLE_Y_POS = 400.0f;
        static constexpr float TITLE_Y_AMPLITUDE = 8.0f;
        static constexpr float GLOW_FREQUENCY = 1.0f;
        static constexpr float GLOW_BASE_SIZE = 2.0f;
        
        float glow = 0.7f + 0.3f * std::sin(titleAnimTime * GLOW_FREQUENCY);
        int titleWidth = MeasureText(TITLE_TEXT, TITLE_FONT_SIZE);
        
        float titleY = TITLE_Y_POS + TITLE_Y_AMPLITUDE * std::sin(titleAnimTime * 1.2f);
        int titleX = screenWidth / 2 - titleWidth / 2;
        
        for (int i = 0; i < static_cast<int>(GLOW_BASE_SIZE + glow * 2.0f); ++i) {
            DrawText(TITLE_TEXT, titleX + i, static_cast<int>(titleY) + i, TITLE_FONT_SIZE, ColorAlpha(COLOR_TITLE, 0.12f));
        }
        DrawText(TITLE_TEXT, titleX, static_cast<int>(titleY), TITLE_FONT_SIZE, COLOR_TITLE);
        
        float buttonStartY = titleY + 120.0f;
        static const Color buttonColors[] = { COLOR_BUTTON_RESUME, COLOR_BUTTON_SETTINGS, COLOR_BUTTON_QUIT };
        
        for (int i = 0; i < NUM_BUTTONS; ++i) {
            float buttonY = buttonStartY + i * (BUTTON_HEIGHT + 30);
            int buttonX = screenWidth / 2 - BUTTON_WIDTH / 2;
            
            Color buttonColor = buttonColors[i];
            if (i == selectedButton) {
                float pulse = 0.85f + 0.15f * std::sin(buttonAnimTime * 3.0f);
                DrawRectangle(buttonX - 4, static_cast<int>(buttonY) - 4, BUTTON_WIDTH + 8, BUTTON_HEIGHT + 8, ColorAlpha(COLOR_SELECTION, pulse));
            }
            
            DrawRectangle(buttonX, static_cast<int>(buttonY), BUTTON_WIDTH, BUTTON_HEIGHT, ColorAlpha(buttonColor, 0.4f));
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

    void PauseMenuComponent::reset() {
        titleAnimTime = 0.0f;
        buttonAnimTime = 0.0f;
        selectedButton = 0;
        inputCooldownTimer = 0.0f;
    }
}
