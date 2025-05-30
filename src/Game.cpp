#include "../include/Game.hpp" // Your Game class header
#include "raylib.h"
#include "Camera.hpp" // Assuming this is your GameCamera header
#include "FishEyeGradient.hpp" // <-- NEW: Include your fisheye gradient header!

const int screenWidth = 1280;
const int screenHeight = 720;

Game::Game() {
    InitWindow(screenWidth, screenHeight, "Dead Cells Cpp Clone");
    SetTargetFPS(60);

  
    Color innerCyan = { 0, 128, 128, 255 };

    Color outerPrussianBlue = { 0, 49, 83, 255 };


    fisheyeBackground = CreateFisheyeGradient(screenWidth, screenHeight, innerCyan, outerPrussianBlue);

    map = std::make_unique<Map>(500, 300); // Construct map first
    player = std::make_unique<Player>(*map); // Then player, passing map by reference
    camera = std::make_unique<GameCamera>(screenWidth, screenHeight, *player);
}

Game::~Game() {
    // <-- NEW: Unload the fisheye background texture when the game closes
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
        ClearBackground(BLACK); // You can keep this or remove it, as the gradient will cover it

        // <-- NEW: Draw the fisheye gradient background FIRST!
        // It should cover the entire screen (0,0) with its full size.
        DrawTexture(fisheyeBackground, 0, 0, WHITE);

        // Begin 2D mode for camera transformations (for your game world)
        BeginMode2D(camera->getCamera());

        // Draw your map and player on top of the background
        map->draw();
        player->draw();

        EndMode2D(); // End 2D mode
        EndDrawing(); // End drawing frame
    }
}
