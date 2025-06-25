#pragma once

#include "Player.hpp"

enum class GameState;

class GameUI {
public:
    GameUI(int screenWidth, int screenHeight);
    void draw(const Player& player, int screenWidth, int screenHeight, GameState currentState);
};
