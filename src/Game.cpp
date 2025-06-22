#include "Game.hpp"
#include "raylib.h"
#include "Camera.hpp"
#include "FishEyeGradient.hpp"
#include "Spawner.hpp" 
#include "GameUI.hpp"
#include "TitleScreenUI.hpp"
#include <algorithm>
#include <filesystem>

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

    int numTiles = 0;
    for (int i = 0; ; ++i) {
        char path_buffer[64];
        snprintf(path_buffer, sizeof(path_buffer), "../resources/tiles/tile%03d.png", i);
        if (!std::filesystem::exists(path_buffer)) break;
        numTiles++;
    }
    for (int i = 0; i < numTiles; ++i) {
        char path_buffer[64];
        snprintf(path_buffer, sizeof(path_buffer), "../resources/tiles/tile%03d.png", i);
        tileTextures.push_back(LoadTexture(path_buffer));
    }

    map = std::make_unique<Map>(500, 300, tileTextures);
    player = std::make_unique<Player>(*map);
    camera = std::make_unique<GameCamera>(screenWidth, screenHeight, *player);

    spawner.spawnEnemiesInRooms(*map, scrapHounds); 

    sceneTexture = LoadRenderTexture(screenWidth, screenHeight);

    bloomShader = LoadShader(0, "../shader/bloom.fs");
    chromaticAberrationShader = LoadShader(0, "../shader/chromatic_aberration.fs");
    activeShader = &bloomShader;
    currentState = GameState::TITLE;

    gameUI = std::make_unique<GameUI>(screenWidth, screenHeight);
    titleScreenUI = std::make_unique<TitleScreenUI>(screenWidth, screenHeight);
}

Game::~Game() {
    UnloadTexture(fisheyeBackground);
    UnloadShader(bloomShader);
    UnloadShader(chromaticAberrationShader);
    for (const auto& texture : tileTextures) {
        UnloadTexture(texture);
    }
    camera.reset();
    player.reset();
    map.reset();
    CloseWindow();
}

void Game::resetGame() {
    map = std::make_unique<Map>(500, 300, tileTextures);
    player = std::make_unique<Player>(*map);
    camera = std::make_unique<GameCamera>(screenWidth, screenHeight, *player);
    scrapHounds.clear();
    spawner.spawnEnemiesInRooms(*map, scrapHounds);
    currentState = GameState::PLAYING;
}

void Game::run() {
    float automataTimer = 0.0f;
    const float automataInterval = 5.0f; // Normal interval
    
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        static float fadeAlpha = 0.0f;
        static bool fadingToPlay = false;
        if (currentState == GameState::TITLE) {
            titleScreenUI->update(dt);
            BeginDrawing();
            ClearBackground(BLACK);
            int action = titleScreenUI->draw();
            if (fadingToPlay) {
                fadeAlpha += dt * 1.5f;
                if (fadeAlpha >= 1.0f) {
                    fadeAlpha = 1.0f;
                    resetGame();
                    fadingToPlay = false;
                    fadeAlpha = 0.0f;
                    EndDrawing();
                    continue;
                }
                DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, fadeAlpha));
            } else if (action == 1) {
                fadingToPlay = true;
            }
            if (action == 3) {
                EndDrawing();
                break;
            }
            EndDrawing();
            continue;
        }

        if (currentState == GameState::PLAYING) {
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

            if (player->getHealth() <= 0) {
                currentState = GameState::GAME_OVER;
            }

        } else if (currentState == GameState::GAME_OVER) {
            if (IsKeyPressed(KEY_R)) {
                resetGame();
            }
        }
        
        BeginTextureMode(sceneTexture);
        ClearBackground(BLACK);
        DrawTexture(fisheyeBackground, 0, 0, WHITE);
        BeginMode2D(camera->getCamera());
        map->draw(camera->getCamera());
        player->draw();

        Vector2 topLeftWorld = GetScreenToWorld2D({0.0f, 0.0f}, camera->getCamera());
        Vector2 bottomRightWorld = GetScreenToWorld2D({(float)screenWidth, (float)screenHeight}, camera->getCamera());

        Rectangle cameraViewWorld = {
            topLeftWorld.x,
            topLeftWorld.y,
            bottomRightWorld.x - topLeftWorld.x,
            bottomRightWorld.y - topLeftWorld.y
        };

        for (const auto& enemy : scrapHounds) {
            if (enemy.isAlive()) {
                Rectangle enemyRect = { 
                    enemy.getPosition().x, 
                    enemy.getPosition().y, 
                    32.0f, 
                    32.0f  
                }; 
                if (CheckCollisionRecs(enemyRect, cameraViewWorld)) {
                    enemy.draw();
                    if (IsKeyDown(KEY_TAB)) { 
                        DrawRectangleLines((int)enemy.getPosition().x, (int)enemy.getPosition().y, 32, 32, RED);
                    }
                }
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
        gameUI->draw(*player, screenWidth, screenHeight, currentState);

        EndDrawing();
    }
}