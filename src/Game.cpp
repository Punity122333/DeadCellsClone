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
        player->update(dt, *map);
        camera->update();
        
        // Check for sword collisions with enemies
        if (player->isSwordAttacking()) {
            Rectangle swordHitbox = player->getSwordHitbox();
            
            for (auto& enemy : scrapHounds) {
                if (enemy.isAlive()) {  // Only check collision with alive enemies
                    Vector2 enemyPos = enemy.getPosition();
                    Rectangle enemyRect = { enemyPos.x, enemyPos.y, 32, 32 };
                    
                    if (CheckCollisionRecs(swordHitbox, enemyRect)) {
                        // Enemy hit by sword!
                        enemy.takeDamage(10); // Apply 10 damage to enemy
                        
                        // Apply knockback direction based on player position
                        Vector2 playerPos = player->getPosition();
                        float knockbackDirX = enemyPos.x < playerPos.x ? -1.0f : 1.0f;
                        enemy.applyKnockback({knockbackDirX * 200.0f, -100.0f});
                        
                        // Play hit sound (if implemented)
                        // PlaySound(enemyHitSound);
                    }
                }
            }
        }

        // Clean up dead enemies
        scrapHounds.erase(
            std::remove_if(scrapHounds.begin(), scrapHounds.end(),
                [](const ScrapHound& enemy) { return !enemy.isAlive(); }),
            scrapHounds.end()
        );

        // Update enemies and check for player collision
        for (auto& enemy : scrapHounds) {
            enemy.update(*map, player->getPosition(), dt);
            
            // Check for enemy collisions with player (player takes damage)
            if (enemy.isAlive()) {
                Vector2 enemyPos = enemy.getPosition();
                Rectangle enemyRect = { enemyPos.x, enemyPos.y, 32, 32 };
                
                Vector2 playerPos = player->getPosition();
                Rectangle playerRect = { playerPos.x, playerPos.y, 32, 32 };
                
                if (CheckCollisionRecs(enemyRect, playerRect) && player->canTakeDamage()) {
                    player->takeDamage(5);  // Player takes 5 damage from enemy contact
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
            
            // Debug visualization (optional)
            if (IsKeyDown(KEY_TAB)) {  // Hold TAB to see hitboxes
                Vector2 enemyPos = enemy.getPosition();
                DrawRectangleLines((int)enemyPos.x, (int)enemyPos.y, 32, 32, RED);
            }
        }
        
        // Debug: visualize sword hitbox when active
        if (IsKeyDown(KEY_TAB) && player->isSwordAttacking()) {
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
        int barX = screenWidth - (int)barWidth - 40; // 40px from right edge
        int barY = 30; // 30px from top

        DrawRectangle(barX, barY, (int)barWidth, (int)barHeight, DARKGRAY); // background
        DrawRectangle(barX, barY, (int)(barWidth * healthRatio), (int)barHeight, RED); // health
        DrawRectangleLines(barX, barY, (int)barWidth, (int)barHeight, BLACK); // border
        DrawText("HEALTH", barX, barY - 22, 22, WHITE);
        EndDrawing();
    }
}