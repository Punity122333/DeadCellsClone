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
    
    // Debug output to see what position the camera is getting
    static int frameCount = 0;
    frameCount++;
    if (frameCount % 60 == 0) {  // Print every 60 frames
        FILE* debugFile = fopen("camera_debug.txt", "a");
        if (debugFile) {
            fprintf(debugFile, "Camera update: Player position=(%.1f,%.1f), setting target to=(%.1f,%.1f)\n", 
                   playerPos.x, playerPos.y, cam.target.x, cam.target.y);
            fclose(debugFile);
        }
    }
}

Camera2D GameCamera::getCamera() const {
    return cam;
}
