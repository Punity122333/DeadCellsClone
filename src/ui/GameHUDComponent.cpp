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
    }
}
