#include "Game.hpp"
#include "raylib.h"
#include "Camera.hpp"
#include "FishEyeGradient.hpp"
#include <algorithm>

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

    Vector2 playerSpawn = player->getPosition();
    int spawned = 0;
    int tryCount = 0;
    while (spawned < 5 && tryCount < 50) {
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

    bloomShader = LoadShader(0, "../shader/bloom.fs");
    chromaticAberrationShader = LoadShader(0, "../shader/chromatic_aberration.fs");
    activeShader = &bloomShader;
}

Game::~Game() {
    UnloadTexture(fisheyeBackground);
    UnloadShader(bloomShader);
    UnloadShader(chromaticAberrationShader);
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
        player->update(dt, *map, camera->getCamera(), scrapHounds);
        camera->update();
        player->checkWeaponHits(scrapHounds);

        scrapHounds.erase(
            std::remove_if(scrapHounds.begin(), scrapHounds.end(),
                [](const ScrapHound& enemy) { return !enemy.isAlive(); }),
            scrapHounds.end()
        );

        for (auto& enemy : scrapHounds) {
            enemy.update(*map, player->getPosition(), dt);
            
            if (enemy.isAlive()) {
                Vector2 enemyPos = enemy.getPosition();
                Rectangle enemyRect = { enemyPos.x, enemyPos.y, 32, 32 };
                
                Vector2 playerPos = player->getPosition();
                Rectangle playerRect = { playerPos.x, playerPos.y, 32, 32 };
                
                if (CheckCollisionRecs(enemyRect, playerRect) && player->canTakeDamage()) {
                    player->takeDamage(5);
                }
            }
        }
        
        BeginTextureMode(sceneTexture);
        ClearBackground(BLACK);
        DrawTexture(fisheyeBackground, 0, 0, WHITE);
        BeginMode2D(camera->getCamera());
        map->draw(camera->getCamera());
        player->draw();

        for (const auto& enemy : scrapHounds) {
            enemy.draw();
            
            if (IsKeyDown(KEY_TAB)) {
                Vector2 enemyPos = enemy.getPosition();
                DrawRectangleLines((int)enemyPos.x, (int)enemyPos.y, 32, 32, RED);
            }
        }
        
        if (IsKeyDown(KEY_TAB) && player->isAttacking()) {
            Rectangle swordHitbox = player->getSwordHitbox();
            DrawRectangleRec(swordHitbox, ColorAlpha(GREEN, 0.5f));
        }
        
        EndMode2D();
        EndTextureMode();
        
        BeginDrawing();
        ClearBackground(BLACK);
        BeginShaderMode(*activeShader);

        Rectangle src = { 0, 0, (float)sceneTexture.texture.width, -(float)sceneTexture.texture.height };
        Rectangle dst = { 0, 0, (float)screenWidth, (float)screenHeight };
        Vector2 origin = { 0, 0 };
        DrawTexturePro(sceneTexture.texture, src, dst, origin, 0.0f, WHITE);

        EndShaderMode();
        float barWidth = 300.0f;
        float barHeight = 24.0f;
        float healthRatio = player->getHealth() / player->getMaxHealth();
        int barX = screenWidth - (int)barWidth - 40;
        int barY = 30;

        DrawRectangle(barX, barY, (int)barWidth, (int)barHeight, DARKGRAY);
        DrawRectangle(barX, barY, (int)(barWidth * healthRatio), (int)barHeight, RED);
        DrawRectangleLines(barX, barY, (int)barWidth, (int)barHeight, BLACK);
        DrawText("HEALTH", barX, barY - 22, 22, WHITE);
        EndDrawing();
    }
}