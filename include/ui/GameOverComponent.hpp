#pragma once

#include "ui/UIController.hpp"
#include <raylib.h>

namespace UI {
    class GameOverComponent : public UIComponent {
    public:
        GameOverComponent(int w, int h);
        ~GameOverComponent() = default;
        
        void update(float dt) override;
        UIAction draw() override;
        void handleInput() override;
        void reset() override;
        
    private:
        static constexpr int GAME_OVER_FONT_SIZE = 80;
        static constexpr int RESTART_FONT_SIZE = 40;
        static constexpr int MENU_FONT_SIZE = 30;
        
        float fadeAlpha;
        float textPulse;
        bool canRestart;
    };
}
