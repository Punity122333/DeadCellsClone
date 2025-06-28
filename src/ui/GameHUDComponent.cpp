#include "ui/GameHUDComponent.hpp"
#include "Game.hpp"
#include <raylib.h>

namespace UI {
    GameHUDComponent::GameHUDComponent(int w, int h) {
        screenWidth = w;
        screenHeight = h;
    }

    void GameHUDComponent::update(float dt) {
    }

    UIAction GameHUDComponent::draw() {
        return UIAction::NONE;
    }

    void GameHUDComponent::handleInput() {
    }

    void GameHUDComponent::drawHUD(const Player& player, GameState currentState) {
        float healthRatio = player.getHealth() / player.getMaxHealth();
        int barX = screenWidth - static_cast<int>(HEALTH_BAR_WIDTH) - HEALTH_BAR_MARGIN;
        int barY = HEALTH_BAR_Y_OFFSET;

        DrawRectangle(barX, barY, static_cast<int>(HEALTH_BAR_WIDTH), static_cast<int>(HEALTH_BAR_HEIGHT), DARKGRAY);
        DrawRectangle(barX, barY, static_cast<int>(HEALTH_BAR_WIDTH * healthRatio), static_cast<int>(HEALTH_BAR_HEIGHT), RED);
        DrawRectangleLines(barX, barY, static_cast<int>(HEALTH_BAR_WIDTH), static_cast<int>(HEALTH_BAR_HEIGHT), BLACK);
        DrawText("HEALTH", barX, barY - HEALTH_TEXT_SIZE, HEALTH_TEXT_SIZE, WHITE);

        if (currentState == GameState::GAME_OVER) {
            DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.7f));
            const char* gameOverText = "GAME OVER";
            int gameOverTextWidth = MeasureText(gameOverText, GAME_OVER_FONT_SIZE);
            DrawText(gameOverText, screenWidth / 2 - gameOverTextWidth / 2, screenHeight / 2 - 40, GAME_OVER_FONT_SIZE, RED);
            const char* restartText = "Press R to Restart";
            int restartTextWidth = MeasureText(restartText, RESTART_FONT_SIZE);
            DrawText(restartText, screenWidth / 2 - restartTextWidth / 2, screenHeight / 2 + 60, RESTART_FONT_SIZE, WHITE);
        }
    }
}
