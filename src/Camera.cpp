#include "Camera.hpp"

GameCamera::GameCamera(int screenWidth, int screenHeight, const Player& p)
    : player(p) {
    cam.target = player.getPosition();
    cam.offset = { (float)screenWidth / 2, (float)screenHeight / 2 };
    cam.rotation = 0.0f;
    cam.zoom = 1.0f;
}

void GameCamera::update() {
    Vector2 playerPos = player.getPosition();
    cam.target = playerPos;
}

Camera2D GameCamera::getCamera() const {
    return cam;
}
