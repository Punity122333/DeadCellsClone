#include "Camera.hpp"
#include "map/Map.hpp"
#include <raylib.h>
#include <raymath.h>
#include <cmath>

GameCamera::GameCamera(int screenWidth, int screenHeight, const Player& p) {
    cam.target = p.getPosition();
    cam.offset = { (float)screenWidth / 2, (float)screenHeight / 2 };
    cam.rotation = 0.0f;
    cam.zoom = 1.0f;
}

void GameCamera::update(const Player& player, const Map& map, float deltaTime) {
    Vector2 playerPos = player.getPosition();
    cam.target = playerPos;

    updateScreenshake(deltaTime);
}

Camera2D GameCamera::getCamera() const {
    return cam;
}

void GameCamera::addScreenshake(float intensity, float duration) {
    if (intensity > shakeIntensity) {
        shakeIntensity = intensity;
        shakeDuration = duration;
        shakeTimer = duration;
    }
}

Vector2 GameCamera::getShakeOffset() const {
    return shakeOffset;
}

float GameCamera::getShakeIntensity() const {
    return shakeIntensity;
}

void GameCamera::updateScreenshake(float deltaTime) {
    if (shakeTimer > 0.0f) {
        shakeTimer -= deltaTime;

        float normalizedTime = shakeTimer / shakeDuration;
        float currentIntensity = shakeIntensity * normalizedTime;

        float angle = GetRandomValue(0, 360) * DEG2RAD;
        float magnitude = currentIntensity * GetRandomValue(50, 100) / 100.0f;
        
        shakeOffset.x = cosf(angle) * magnitude * 0.01f; 
        shakeOffset.y = sinf(angle) * magnitude * 0.01f;

        cam.offset.x += shakeOffset.x * 100.0f;
        cam.offset.y += shakeOffset.y * 100.0f;
    } else {

        shakeTimer = 0.0f;
        shakeIntensity = 0.0f;
        shakeOffset = {0.0f, 0.0f};
    }
}
