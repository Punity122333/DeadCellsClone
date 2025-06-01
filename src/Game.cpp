#include "../include/Game.hpp"
#include "raylib.h"
#include "Camera.hpp"
#include "FishEyeGradient.hpp"

const int screenWidth = 1920;
const int screenHeight = 1080;

Game::Game() {
    InitWindow(screenWidth, screenHeight, "Dead Cells Cpp Clone");
    SetTargetFPS(60);

    Image icon = LoadImage("../resources/icon/CellularAutomata.png");
    ImageFormat(&icon, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8); 
    SetWindowIcon(icon);
    UnloadImage(icon);

    Color innerCyan = { 0, 80, 80, 255 };
    Color outerPrussianBlue = { 0, 30, 50, 255 };

    fisheyeBackground = CreateFisheyeGradient(screenWidth, screenHeight, innerCyan, outerPrussianBlue);

    map = std::make_unique<Map>(500, 300);
    player = std::make_unique<Player>(*map);
    camera = std::make_unique<GameCamera>(screenWidth, screenHeight, *player);

    // Spawn 5 ScrapHounds at random empty spawns
    Vector2 playerSpawn = player->getPosition();
    int spawned = 0;
    int tryCount = 0;
    while (spawned < 5 && tryCount < 50) {
        // Try random offsets within a 5-tile radius
        int dx = GetRandomValue(-5, 5);
        int dy = GetRandomValue(-3, 3);
        int tx = (int)(playerSpawn.x / 32) + dx;
        int ty = (int)(playerSpawn.y / 32) + dy;
        if (map->isTileEmpty(tx, ty) && !(dx == 0 && dy == 0)) {
            scrapHounds.emplace_back(Vector2{ tx * 32.0f, ty * 32.0f });
            spawned++;
        }
        tryCount++;
    }

    sceneTexture = LoadRenderTexture(screenWidth, screenHeight);

    // --- Load bloom fragment shader ---
    bloomShader = LoadShader(0, "../shader/bloom.fs");
    // ----------------------------------

}

Game::~Game() {
    UnloadTexture(fisheyeBackground);

    
    UnloadShader(bloomShader);

    camera.reset();
    player.reset();
    map.reset();
    CloseWindow();
}

void Game::run() {
    float automataTimer = 0.0f;
    const float automataInterval = 5.0f;
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        automataTimer += dt;

        if (automataTimer >= automataInterval) {
            map->applyConwayAutomata();
            automataTimer = 0.0f;
        }

        map->updateTransitions(dt);
        player->update(dt, *map);
        camera->update();

        for (auto& enemy : scrapHounds) {
            enemy.update(*map, player->getPosition(), dt);
        }

        
        BeginTextureMode(sceneTexture);
        ClearBackground(BLACK);
        DrawTexture(fisheyeBackground, 0, 0, WHITE);
        BeginMode2D(camera->getCamera());
        map->draw();
        player->draw();

        for (const auto& enemy : scrapHounds) {
            enemy.draw();
        }
        EndMode2D();
        EndTextureMode();

        
        BeginDrawing();
        ClearBackground(BLACK);
        BeginShaderMode(bloomShader);

        Rectangle src = { 0, 0, (float)sceneTexture.texture.width, -(float)sceneTexture.texture.height };
        Rectangle dst = { 0, 0, (float)screenWidth, (float)screenHeight };
        Vector2 origin = { 0, 0 };
        DrawTexturePro(sceneTexture.texture, src, dst, origin, 0.0f, WHITE);

        EndShaderMode();
        EndDrawing();
    }
}