#include "../include/Game.hpp"
#include "raylib.h"
#include "Camera.hpp"
#include "FishEyeGradient.hpp"

const int screenWidth = 1280;
const int screenHeight = 720;

Game::Game() {
    InitWindow(screenWidth, screenHeight, "Dead Cells Cpp Clone");
    SetTargetFPS(60);

    Color innerCyan = { 0, 80, 80, 255 };
    Color outerPrussianBlue = { 0, 30, 50, 255 };

    fisheyeBackground = CreateFisheyeGradient(screenWidth, screenHeight, innerCyan, outerPrussianBlue);

    map = std::make_unique<Map>(500, 300);
    player = std::make_unique<Player>(*map);
    camera = std::make_unique<GameCamera>(screenWidth, screenHeight, *player);
}

Game::~Game() {
    UnloadTexture(fisheyeBackground);

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

        DrawTexture(fisheyeBackground, 0, 0, WHITE);

        BeginMode2D(camera->getCamera());

        map->draw();
        player->draw();

        EndMode2D();
        EndDrawing();
    }
}
