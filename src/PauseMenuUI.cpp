#include "PauseMenuUI.hpp"
#include "raylib.h"
#include <cmath>
#include <string>
#include <thread>
#include <atomic>

#ifndef CYAN
    #define CYAN CLITERAL(Color){ 0, 255, 255, 255 }
#endif

static const char* BUTTON_LABELS[] = {"RESUME", "SETTINGS", "QUIT TO MENU"};
static const Color COLOR_TITLE = ORANGE;
static const Color COLOR_BUTTON_HOVER = LIME;
static const Color COLOR_BUTTON_RESUME = GREEN;
static const Color COLOR_BUTTON_SETTINGS = VIOLET;
static const Color COLOR_BUTTON_QUIT = RED;
static const Color COLOR_SELECTION = YELLOW;

PauseMenuUI::PauseMenuUI(int w, int h) : screenWidth(w), screenHeight(h) {
    for (int i = 0; i < 2; i++) {
        bgData[i].hLinePositions.resize(NUM_HORIZONTAL_LINES);
        bgData[i].vLinePositions.resize(NUM_VERTICAL_LINES);
        bgData[i].hLineColors.resize(NUM_HORIZONTAL_LINES);
        bgData[i].vLineColors.resize(NUM_VERTICAL_LINES);
    }
    bgAnimThread = std::thread(&PauseMenuUI::updateBackgroundAnimations, this);
}

PauseMenuUI::~PauseMenuUI() {
    threadRunning = false;
    if (bgAnimThread.joinable()) {
        bgAnimThread.join();
    }
}

void PauseMenuUI::updateBackgroundAnimations() {
    float localTitleAnimTime = 0.0f;
    while (threadRunning) {
        int nextBuffer = 1 - currentBgBuffer;
        BackgroundData& data = bgData[nextBuffer];
        for (int i = 0; i < NUM_HORIZONTAL_LINES; ++i) {
            float y = fmodf(localTitleAnimTime * 60 + i * 50, (float)screenHeight);
            Color c = ColorAlpha(DARKPURPLE, 0.08f + 0.08f * sinf(localTitleAnimTime + i));
            data.hLinePositions[i] = y;
            data.hLineColors[i] = c;
        }
        for (int i = 0; i < NUM_VERTICAL_LINES; ++i) {
            float x = fmodf(localTitleAnimTime * 40 + i * 120, (float)screenWidth);
            Color c = ColorAlpha(ORANGE, 0.06f + 0.06f * cosf(localTitleAnimTime + i));
            data.vLinePositions[i] = x;
            data.vLineColors[i] = c;
        }
        data.ready = true;
        localTitleAnimTime += 0.016f;
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

void PauseMenuUI::update(float dt) {
    titleAnimTime += dt;
    buttonAnimTime += dt;
    inputCooldownTimer -= dt;
    
    if (inputCooldownTimer <= 0.0f) {
        if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
            selectedButton = (selectedButton - 1 + NUM_BUTTONS) % NUM_BUTTONS;
            inputCooldownTimer = INPUT_COOLDOWN;
        } else if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
            selectedButton = (selectedButton + 1) % NUM_BUTTONS;
            inputCooldownTimer = INPUT_COOLDOWN;
        }
    }
    
    if (bgData[1 - currentBgBuffer].ready) {
        currentBgBuffer = 1 - currentBgBuffer;
        bgData[currentBgBuffer].ready = false;
    }
}

int PauseMenuUI::draw() {
    DrawRectangle(0, 0, screenWidth, screenHeight, ColorAlpha(BLACK, 0.7f));
    
    const BackgroundData& data = bgData[currentBgBuffer];
    for (int i = 0; i < NUM_HORIZONTAL_LINES; ++i) {
        DrawLine(0, (int)data.hLinePositions[i], screenWidth, (int)data.hLinePositions[i], data.hLineColors[i]);
    }
    for (int i = 0; i < NUM_VERTICAL_LINES; ++i) {
        DrawLine((int)data.vLinePositions[i], 0, (int)data.vLinePositions[i], screenHeight, data.vLineColors[i]);
    }
    
    static const std::string TITLE_TEXT = "PAUSED";
    static constexpr float TITLE_Y_POS = 140.0f;
    static constexpr float TITLE_Y_AMPLITUDE = 8.0f;
    static constexpr float GLOW_FREQUENCY = 2.0f;
    static constexpr float GLOW_BASE_SIZE = 3.0f;
    
    float glow = 0.5f + 0.5f * sinf(titleAnimTime * GLOW_FREQUENCY);
    int titleWidth = MeasureText(TITLE_TEXT.c_str(), TITLE_FONT_SIZE);
    Vector2 titlePos = {
        screenWidth / 2.0f - titleWidth / 2.0f, 
        TITLE_Y_POS + TITLE_Y_AMPLITUDE * sinf(titleAnimTime)
    };
    for (int i = 0; i < NUM_GLOW_EFFECTS; ++i) {
        float angle = i * 3.14159265f / 3.0f + titleAnimTime;
        float dx = cosf(angle) * (GLOW_BASE_SIZE + GLOW_BASE_SIZE * glow);
        float dy = sinf(angle) * (GLOW_BASE_SIZE + GLOW_BASE_SIZE * glow);
        DrawText(TITLE_TEXT.c_str(), (int)(titlePos.x + dx), (int)(titlePos.y + dy), TITLE_FONT_SIZE, Fade(ORANGE, 0.15f));
    }
    DrawText(TITLE_TEXT.c_str(), (int)titlePos.x, (int)titlePos.y, TITLE_FONT_SIZE, COLOR_TITLE);
    
    int startY = (int)titlePos.y + TITLE_FONT_SIZE + PADDING_BELOW_TITLE;
    int mouseX = GetMouseX(), mouseY = GetMouseY();
    int action = 0;
    
    static constexpr float BUTTON_CORNER_RADIUS = 0.35f;
    static constexpr int BUTTON_CORNER_SEGMENTS = 16;
    static constexpr float BUTTON_PULSE_AMP = 0.08f;
    static constexpr float BUTTON_PULSE_FREQ = 3.0f;
    static constexpr float BUTTON_PULSE_PHASE = 1.2f;
    static constexpr int GLOW_LAYERS = 6;
    static constexpr float GLOW_STEP = 2.0f;
    static constexpr float GLOW_ALPHA_STEP = 0.12f;
    
    for (int i = 0; i < NUM_BUTTONS; ++i) {
        int bx = screenWidth / 2 - BUTTON_WIDTH / 2;
        int by = startY + i * (BUTTON_HEIGHT + BUTTON_SPACING);
        Rectangle btnRect = {(float)bx, (float)by, (float)BUTTON_WIDTH, (float)BUTTON_HEIGHT};
        bool hovered = CheckCollisionPointRec({(float)mouseX, (float)mouseY}, btnRect);
        
        Color base;
        if (hovered) {
            base = COLOR_BUTTON_HOVER;
        } else {
            switch (i) {
                case 0: base = COLOR_BUTTON_RESUME; break;
                case 1: base = COLOR_BUTTON_SETTINGS; break;
                case 2: base = COLOR_BUTTON_QUIT; break;
            }
        }
        
        Color glowC = ColorAlpha(base, hovered ? 0.35f : 0.18f);
        for (int g = GLOW_LAYERS; g > 0; --g) {
            Rectangle glowRect = {
                btnRect.x - g * GLOW_STEP, 
                btnRect.y - g * GLOW_STEP, 
                btnRect.width + g * GLOW_STEP * 2, 
                btnRect.height + g * GLOW_STEP * 2
            };
            DrawRectangleRounded(glowRect, BUTTON_CORNER_RADIUS, 
                BUTTON_CORNER_SEGMENTS, ColorAlpha(glowC, GLOW_ALPHA_STEP * g));
        }
        DrawRectangleRounded(btnRect, BUTTON_CORNER_RADIUS, BUTTON_CORNER_SEGMENTS, ColorAlpha(base, 0.85f));
        
        int textW = MeasureText(BUTTON_LABELS[i], BUTTON_FONT_SIZE);
        DrawText(BUTTON_LABELS[i], 
                bx + BUTTON_WIDTH / 2 - textW / 2, 
                by + BUTTON_HEIGHT / 2 - BUTTON_FONT_SIZE / 2, 
                BUTTON_FONT_SIZE, WHITE);
        
        if (hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            action = i + 1;
        }
    }
    
    for (int i = 0; i < NUM_BUTTONS; ++i) {
        if (i == selectedButton) {
            int bx = screenWidth / 2 - BUTTON_WIDTH / 2;
            int by = startY + i * (BUTTON_HEIGHT + BUTTON_SPACING);
            Rectangle selectedRect = {(float)bx, (float)by, (float)BUTTON_WIDTH, (float)BUTTON_HEIGHT};
            float selPulse = 0.5f + 0.5f * sinf(buttonAnimTime * BUTTON_PULSE_FREQ);
            DrawRectangleLinesEx(selectedRect, SELECTION_BORDER_WIDTH, Fade(COLOR_SELECTION, selPulse));
        }
    }
    
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        action = selectedButton + 1;
    }
    
    static constexpr int SCANLINE_SPACING = 6;
    static constexpr int SCANLINE_HEIGHT = 2;
    static constexpr float SCANLINE_ALPHA = 0.08f;
    for (int y = 0; y < screenHeight; y += SCANLINE_SPACING) {
        DrawRectangle(0, y, screenWidth, SCANLINE_HEIGHT, ColorAlpha(BLACK, SCANLINE_ALPHA));
    }
    
    return action;
}
