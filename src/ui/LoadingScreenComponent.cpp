#include "ui/LoadingScreenComponent.hpp"
#include <raylib.h>
#include <cmath>
#include <chrono>

#ifndef CYAN
    #define CYAN CLITERAL(Color){ 0, 255, 255, 255 }
#endif

namespace UI {
    const Color LoadingScreenComponent::COLOR_TITLE = SKYBLUE;
    const Color LoadingScreenComponent::COLOR_LOADING_BAR_BG = {50, 50, 50, 255};
    const Color LoadingScreenComponent::COLOR_LOADING_BAR_FILL = {100, 200, 255, 255};
    const Color LoadingScreenComponent::COLOR_LOADING_TEXT = WHITE;

    LoadingScreenComponent::LoadingScreenComponent(int w, int h) {
        screenWidth = w;
        screenHeight = h;
        
        for (int i = 0; i < 2; i++) {
            bgData[i].hLinePositions.resize(NUM_HORIZONTAL_LINES);
            bgData[i].vLinePositions.resize(NUM_VERTICAL_LINES);
            bgData[i].hLineColors.resize(NUM_HORIZONTAL_LINES);
            bgData[i].vLineColors.resize(NUM_VERTICAL_LINES);
        }
        
        bgAnimThread = std::thread(&LoadingScreenComponent::updateBackgroundAnimations, this);
    }

    LoadingScreenComponent::~LoadingScreenComponent() {
        // Stop the thread first before any data cleanup
        threadRunning.store(false, std::memory_order_release);
        
        if (bgAnimThread.joinable()) {
            try {
                bgAnimThread.join();
            } catch (...) {
                // If join fails, detach as last resort
                bgAnimThread.detach();
            }
        }
        
        // Now it's safe to let vectors be destroyed automatically
    }

    void LoadingScreenComponent::updateBackgroundAnimations() {
        float localAnimTime = 0.0f;
        
        while (threadRunning.load(std::memory_order_acquire)) {
            localAnimTime += 0.016f;
            
            int nextBuffer = 1 - currentBgBuffer;
            auto& data = bgData[nextBuffer];
            
            for (int i = 0; i < NUM_HORIZONTAL_LINES; ++i) {
                data.hLinePositions[i] = std::sin(localAnimTime * 0.8f + i * 0.4f) * 50.0f + 
                                        static_cast<float>(screenHeight) * (i + 1) / static_cast<float>(NUM_HORIZONTAL_LINES + 1);
                
                float intensity = 0.3f + 0.4f * std::sin(localAnimTime * 1.2f + i * 0.6f);
                data.hLineColors[i] = {
                    static_cast<unsigned char>(50 + intensity * 100),
                    static_cast<unsigned char>(150 + intensity * 105),
                    static_cast<unsigned char>(200 + intensity * 55),
                    static_cast<unsigned char>(60 + intensity * 120)
                };
            }
            
            for (int i = 0; i < NUM_VERTICAL_LINES; ++i) {
                data.vLinePositions[i] = std::cos(localAnimTime * 0.6f + i * 0.5f) * 30.0f + 
                                        static_cast<float>(screenWidth) * (i + 1) / static_cast<float>(NUM_VERTICAL_LINES + 1);
                
                float intensity = 0.2f + 0.3f * std::cos(localAnimTime * 1.5f + i * 0.8f);
                data.vLineColors[i] = {
                    static_cast<unsigned char>(30 + intensity * 80),
                    static_cast<unsigned char>(100 + intensity * 100),
                    static_cast<unsigned char>(180 + intensity * 75),
                    static_cast<unsigned char>(40 + intensity * 100)
                };
            }
            
            data.ready.store(true, std::memory_order_release);
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }

    void LoadingScreenComponent::update(float dt) {
        titleAnimTime += dt;
        barAnimTime.store(barAnimTime.load(std::memory_order_relaxed) + dt, std::memory_order_relaxed);
        
        if (bgData[1 - currentBgBuffer].ready.load(std::memory_order_acquire)) {
            currentBgBuffer = 1 - currentBgBuffer;
            bgData[currentBgBuffer].ready.store(false, std::memory_order_release);
        }
        
        // Check if we've been stuck at 100% for too long
        float currentProgress = loadingProgress.load(std::memory_order_acquire);
        if (currentProgress >= 1.0f && !loadingComplete.load(std::memory_order_relaxed)) {
            // Force completion if we've been at 100% for more than 2 seconds
            timeAt100 += dt;
            if (timeAt100 > 2.0f) {
                loadingComplete.store(true, std::memory_order_release);
                timeAt100 = 0.0f;
            }
        } else {
            timeAt100 = 0.0f; // Reset if not at 100%
        }
    }

    UIAction LoadingScreenComponent::draw() {
        // Draw animated background (same style as title screen)
        const BackgroundData& data = bgData[currentBgBuffer];
        
        for (int i = 0; i < NUM_HORIZONTAL_LINES; ++i) {
            DrawLine(0, static_cast<int>(data.hLinePositions[i]), screenWidth, 
                    static_cast<int>(data.hLinePositions[i]), data.hLineColors[i]);
        }
        
        for (int i = 0; i < NUM_VERTICAL_LINES; ++i) {
            DrawLine(static_cast<int>(data.vLinePositions[i]), 0, 
                    static_cast<int>(data.vLinePositions[i]), screenHeight, data.vLineColors[i]);
        }
        
        // Draw title with glow effect
        static const char* TITLE_TEXT = "LOADING AUTOMATA...";
        static constexpr float TITLE_Y_POS = 400.0f;
        static constexpr float TITLE_Y_AMPLITUDE = 10.0f;
        static constexpr float GLOW_FREQUENCY = 1.2f;
        static constexpr float GLOW_BASE_SIZE = 6.0f;
        
        float glow = 0.7f + 0.3f * std::sin(titleAnimTime * GLOW_FREQUENCY);
        int titleWidth = MeasureText(TITLE_TEXT, TITLE_FONT_SIZE);
        
        float titleY = TITLE_Y_POS + TITLE_Y_AMPLITUDE * std::sin(titleAnimTime * 1.0f);
        int titleX = screenWidth / 2 - titleWidth / 2;
        
        // Draw glow effect
        for (int i = 0; i < static_cast<int>(GLOW_BASE_SIZE + glow * 2.0f); ++i) {
            DrawText(TITLE_TEXT, titleX + i, static_cast<int>(titleY) + i, TITLE_FONT_SIZE, 
                    ColorAlpha(COLOR_TITLE, 0.08f));
        }
        DrawText(TITLE_TEXT, titleX, static_cast<int>(titleY), TITLE_FONT_SIZE, COLOR_TITLE);
        
        // Draw loading bar
        float barY = titleY + 120.0f;
        int barX = screenWidth / 2 - LOADING_BAR_WIDTH / 2;
        
        // Background
        DrawRectangle(barX - 4, static_cast<int>(barY) - 4, LOADING_BAR_WIDTH + 8, LOADING_BAR_HEIGHT + 8, 
                     ColorAlpha(WHITE, 0.3f));
        DrawRectangle(barX, static_cast<int>(barY), LOADING_BAR_WIDTH, LOADING_BAR_HEIGHT, COLOR_LOADING_BAR_BG);
        
        // Progress fill with animation
        float progress = loadingProgress.load(std::memory_order_relaxed);
        int fillWidth = static_cast<int>(LOADING_BAR_WIDTH * progress);
        
        if (fillWidth > 0) {
            // Add a pulsing effect to the loading bar
            float pulse = 0.8f + 0.2f * std::sin(barAnimTime.load(std::memory_order_relaxed) * 4.0f);
            Color fillColor = {
                static_cast<unsigned char>(COLOR_LOADING_BAR_FILL.r * pulse),
                static_cast<unsigned char>(COLOR_LOADING_BAR_FILL.g * pulse),
                static_cast<unsigned char>(COLOR_LOADING_BAR_FILL.b * pulse),
                COLOR_LOADING_BAR_FILL.a
            };
            
            DrawRectangle(barX, static_cast<int>(barY), fillWidth, LOADING_BAR_HEIGHT, fillColor);
        }
        
        // Border
        DrawRectangleLines(barX, static_cast<int>(barY), LOADING_BAR_WIDTH, LOADING_BAR_HEIGHT, WHITE);
        
        // Progress percentage
        char progressText[32];
        snprintf(progressText, sizeof(progressText), "%.0f%%", progress * 100.0f);
        int progressTextWidth = MeasureText(progressText, LOADING_TEXT_SIZE);
        DrawText(progressText, screenWidth / 2 - progressTextWidth / 2, 
                static_cast<int>(barY) + LOADING_BAR_HEIGHT + 20, LOADING_TEXT_SIZE, COLOR_LOADING_TEXT);

        return UIAction::NONE;
    }

    void LoadingScreenComponent::handleInput() {
        // No input handling needed during loading
    }

    void LoadingScreenComponent::reset() {
        titleAnimTime = 0.0f;
        barAnimTime.store(0.0f, std::memory_order_relaxed);
        loadingProgress.store(0.0f, std::memory_order_release);
        loadingComplete.store(false, std::memory_order_release);
        timeAt100 = 0.0f;
    }

    void LoadingScreenComponent::setProgress(float progress) {
        loadingProgress.store(progress, std::memory_order_relaxed);
        if (progress >= 1.0f) {
            loadingComplete.store(true, std::memory_order_release);
        }
    }

    bool LoadingScreenComponent::isLoadingComplete() const {
        return loadingComplete.load(std::memory_order_acquire);
    }
}
