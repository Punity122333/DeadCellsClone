#pragma once
#include "raylib.h"
#include "Player.hpp"

class GameCamera {
public:
    GameCamera(int screenWidth, int screenHeight, const Player& player);
    void update();
    Camera2D getCamera() const;

private:
    Camera2D cam;
    const Player& player;
};
