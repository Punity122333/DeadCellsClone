#include "ui/GameOverComponent.hpp"
#include <raylib.h>
#include <cmath>

namespace UI {
    GameOverComponent::GameOverComponent(int w, int h) 
        : fadeAlpha(0.0f), textPulse(0.0f), canRestart(false) {
        screenWidth = w;
        screenHeight = h;
    }

    void GameOverComponent::update(float dt) {
        if (fadeAlpha < 0.8f) {
            fadeAlpha += dt * 2.0f;
            if (fadeAlpha >= 0.8f) {
                fadeAlpha = 0.8f;
                canRestart = true;
            }
        }
        
        textPulse += dt * 3.0f;
    }

    UIAction GameOverComponent::draw() {
        DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, fadeAlpha));
        
        const char* gameOverText = "GAME OVER";
        int gameOverTextWidth = MeasureText(gameOverText, GAME_OVER_FONT_SIZE);
        DrawText(gameOverText, screenWidth / 2 - gameOverTextWidth / 2, screenHeight / 2 - 80, GAME_OVER_FONT_SIZE, RED);
        
        if (canRestart) {
            float pulse = 0.5f + 0.5f * sin(textPulse);
            Color restartColor = {255, 255, 255, (unsigned char)(155 + 100 * pulse)};
            
            const char* restartText = "Press R to Restart";
            int restartTextWidth = MeasureText(restartText, RESTART_FONT_SIZE);
            DrawText(restartText, screenWidth / 2 - restartTextWidth / 2, screenHeight / 2 + 20, RESTART_FONT_SIZE, restartColor);
            
            const char* menuText = "Press ESC for Main Menu";
            int menuTextWidth = MeasureText(menuText, MENU_FONT_SIZE);
            DrawText(menuText, screenWidth / 2 - menuTextWidth / 2, screenHeight / 2 + 80, MENU_FONT_SIZE, LIGHTGRAY);
            
            if (IsKeyPressed(KEY_R)) {
                printf("[GameOverComponent] R key pressed - triggering restart\n");
                return UIAction::RESTART;
            }
            if (IsKeyPressed(KEY_ESCAPE)) {
                printf("[GameOverComponent] ESC key pressed - quitting to menu\n");
                return UIAction::QUIT_TO_MENU;
            }
        }
        
        return UIAction::NONE;
    }

    void GameOverComponent::handleInput() {
    }

    void GameOverComponent::reset() {
        fadeAlpha = 0.0f;
        textPulse = 0.0f;
        canRestart = false;
    }
}
