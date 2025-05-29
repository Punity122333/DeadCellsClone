#include "../include/Game.hpp"
#include "raylib.h"
#include "Camera.hpp"
Game::Game() {
    InitWindow(screenWidth, screenHeight, "Dead Cells Cpp Clone");
    SetTargetFPS(60);

    map = std::make_unique<Map>(500, 300); // Construct map first
    player = std::make_unique<Player>(*map); // Then player, passing map by reference
    camera = std::make_unique<GameCamera>(screenWidth, screenHeight, *player); // Then camera
}

Game::~Game() {
    // Make sure to destroy player and camera before map (unique_ptrs are destroyed in reverse order of declaration)
    camera.reset();
    player.reset();
    map.reset();
    CloseWindow();
}

void Game::run() {
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        player->update(dt, *map);
        camera->update();

        BeginDrawing();
        ClearBackground(BLACK);
        BeginMode2D(camera->getCamera());

        map->draw();
        player->draw();

        EndMode2D();
        EndDrawing();
    }
}