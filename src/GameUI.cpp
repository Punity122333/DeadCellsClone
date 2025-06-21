#include "GameUI.hpp"
#include "Game.hpp"
#include "raylib.h"

GameUI::GameUI(int screenWidth, int screenHeight) {
}

void GameUI::draw(const Player& player, int screenWidth, int screenHeight, GameState currentState) {
    float barWidth = 300.0f;
    float barHeight = 24.0f;
    float healthRatio = player.getHealth() / player.getMaxHealth();
    int barX = screenWidth - (int)barWidth - 40;
    int barY = 30;

    DrawRectangle(barX, barY, (int)barWidth, (int)barHeight, DARKGRAY);
    DrawRectangle(barX, barY, (int)(barWidth * healthRatio), (int)barHeight, RED);
    DrawRectangleLines(barX, barY, (int)barWidth, (int)barHeight, BLACK);
    DrawText("HEALTH", barX, barY - 22, 22, WHITE);

    if (currentState == GameState::GAME_OVER) {
        DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.7f));
        const char* gameOverText = "GAME OVER";
        int gameOverTextWidth = MeasureText(gameOverText, 80);
        DrawText(gameOverText, screenWidth / 2 - gameOverTextWidth / 2, screenHeight / 2 - 40, 80, RED);
        const char* restartText = "Press R to Restart";
        int restartTextWidth = MeasureText(restartText, 40);
        DrawText(restartText, screenWidth / 2 - restartTextWidth / 2, screenHeight / 2 + 60, 40, WHITE);
    }
}
