#pragma once
#include "raylib.h"
#include "Player.hpp"

class GameCamera {
public:
    GameCamera(int screenWidth, int screenHeight, const Player& player);
    void update(const Player& player, const Map& map, float deltaTime);
    Camera2D getCamera() const;
    
    // Screenshake functionality
    void addScreenshake(float intensity, float duration);
    Vector2 getShakeOffset() const;
    float getShakeIntensity() const;

private:
    Camera2D cam;
    
    // Screenshake properties
    float shakeTimer = 0.0f;
    float shakeDuration = 0.0f;
    float shakeIntensity = 0.0f;
    Vector2 shakeOffset = {0.0f, 0.0f};
    
    void updateScreenshake(float deltaTime);
};
