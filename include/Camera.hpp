#pragma once
#include "raylib.h"
#include "Player.hpp"

class GameCamera {
public:
    GameCamera(int screenWidth, int screenHeight, const Player& player);
    void update(const Player& player, const Map& map, float deltaTime);
    Camera2D getCamera() const;

private:
    Camera2D cam;
};
