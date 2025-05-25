#include "../include/Game.hpp"
#include "raylib.h"

Game::Game():map(100, 30), player(map), camera(screenWidth, screenHeight, player) {
    InitWindow(screenWidth, screenHeight, "Dead Cells Cpp Clone");
    SetTargetFPS(60);
}

Game::~Game() {
    CloseWindow();
}

void Game::run() {
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        player.update(dt, map);
        camera.update();
        
        BeginDrawing();
        ClearBackground(BLACK);
        BeginMode2D(camera.getCamera());
        
        map.draw();
        player.draw();

        EndMode2D();
        EndDrawing();
    }
}