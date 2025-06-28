#pragma once

#include "ui/UIController.hpp"
#include "Player.hpp"
#include <raylib.h>

namespace UI {
    class GameHUDComponent : public UIComponent {
    public:
        GameHUDComponent(int w, int h);
        ~GameHUDComponent() = default;
        
        void update(float dt) override;
        UIAction draw() override;
        void handleInput() override;
        void drawHUD(const Player& player, GameState currentState);
        
    private:
        static constexpr float HEALTH_BAR_WIDTH = 300.0f;
        static constexpr float HEALTH_BAR_HEIGHT = 24.0f;
        static constexpr int HEALTH_BAR_MARGIN = 40;
        static constexpr int HEALTH_BAR_Y_OFFSET = 30;
        static constexpr int HEALTH_TEXT_SIZE = 22;
        static constexpr int GAME_OVER_FONT_SIZE = 80;
        static constexpr int RESTART_FONT_SIZE = 40;
    };
}
