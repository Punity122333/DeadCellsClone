#include "Game.hpp"
#include "raylib.h"
#include "Camera.hpp"
#include "FishEyeGradient.hpp"
#include "Spawner.hpp" 
#include "ui/UIController.hpp"
#include "effects/ParticleSystem.hpp"
#include "core/Core.hpp"
#include <algorithm>
#include <filesystem>

const int screenWidth = 1920;
const int screenHeight = 1080;

namespace GamePaths {
    constexpr const char* Icon = "../resources/icon/CellularAutomata.png";
    constexpr const char* Tile = "../resources/tiles/tile%03d.png";
    constexpr const char* BloomShader = "../shader/bloom.fs";
    constexpr const char* ChromaticAberrationShader = "../shader/chromatic_aberration.fs";
}

Game::Game() 
    : automataTimer(0.0f)
    , fadeAlpha(0.0f)
    , fadingToPlay(false)
{
    InitWindow(screenWidth, screenHeight, "Cellular Automata");
    SetExitKey(KEY_F4);

    gameLoop = std::make_unique<Core::GameLoop>(60, 60);
    gameLoop->setUpdateCallback([this](float deltaTime) { update(deltaTime); });
    gameLoop->setRenderCallback([this](float interpolation) { render(interpolation); });

    auto& resourceManager = Core::GetResourceManager();
    
    Image icon = LoadImage(GamePaths::Icon);
    ImageFormat(&icon, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8); 
    SetWindowIcon(icon);
    UnloadImage(icon);

    initializeResources();
    
    currentState = GameState::TITLE;
    uiController = std::make_unique<UI::UIController>(screenWidth, screenHeight);

    auto& inputManager = Core::GetInputManager();
    inputManager.registerCallback([this](const Core::InputEvent& event) {
        if (event.action == Core::InputAction::PAUSE && event.pressed) {
            if (currentState == GameState::PLAYING) {
                currentState = GameState::PAUSED;
            } else if (currentState == GameState::PAUSED) {
                currentState = GameState::PLAYING;
            }
        }
    });
}

Game::~Game() {
    auto& resourceManager = Core::GetResourceManager();
    
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

void Game::initializeResources() {
    Color innerCyan = { 0, 80, 80, 255 };
    Color outerPrussianBlue = { 0, 30, 50, 255 };
    fisheyeBackground = CreateFisheyeGradient(screenWidth, screenHeight, innerCyan, outerPrussianBlue);

    int numTiles = 0;
    for (int i = 0; ; ++i) {
        char path_buffer[64];
        snprintf(path_buffer, sizeof(path_buffer), GamePaths::Tile, i);
        if (!std::filesystem::exists(path_buffer)) break;
        numTiles++;
    }
    for (int i = 0; i < numTiles; ++i) {
        char path_buffer[64];
        snprintf(path_buffer, sizeof(path_buffer), GamePaths::Tile, i);
        tileTextures.push_back(LoadTexture(path_buffer));
    }

    map = std::make_unique<Map>(500, 300, tileTextures);
    player = std::make_unique<Player>(*map);
    camera = std::make_unique<GameCamera>(screenWidth, screenHeight, *player);

    spawner.spawnEnemiesInRooms(*map, scrapHounds, automatons);

    sceneTexture = LoadRenderTexture(screenWidth, screenHeight);
    bloomShader = LoadShader(0, GamePaths::BloomShader);
    chromaticAberrationShader = LoadShader(0, GamePaths::ChromaticAberrationShader);
    activeShader = &bloomShader;
}

void Game::resetGame() {
    map = std::make_unique<Map>(500, 300, tileTextures);
    player = std::make_unique<Player>(*map);
    camera = std::make_unique<GameCamera>(screenWidth, screenHeight, *player);
    scrapHounds.clear();
    automatons.clear();
    spawner.spawnEnemiesInRooms(*map, scrapHounds, automatons); 
    currentState = GameState::PLAYING;
    automataTimer = 0.0f;
    fadeAlpha = 0.0f;
    fadingToPlay = false;
}

void Game::run() {
    gameLoop->run();
}

void Game::update(float deltaTime) {
    auto& inputManager = Core::GetInputManager();
    auto& eventManager = Core::GetEventManager();
    
    inputManager.update(deltaTime);
    eventManager.processEvents();
    
    if (currentState == GameState::TITLE) {
        uiController->update(deltaTime, currentState);
        return;
    }

    if (currentState == GameState::PLAYING) {
        automataTimer += deltaTime;

        if (automataTimer >= automataInterval) {
            map->applyConwayAutomata();
            automataTimer = 0.0f;
        }

        map->updateTransitions(deltaTime);
        map->updateParticles(deltaTime, player->getPosition());
        ParticleSystem::getInstance().update(deltaTime);
        player->update(deltaTime, *map, camera->getCamera(), scrapHounds, automatons);
        camera->update();
        player->checkWeaponHits(scrapHounds, automatons);

        scrapHounds.erase(
            std::remove_if(scrapHounds.begin(), scrapHounds.end(),
                [](const ScrapHound& enemy) { return !enemy.isAlive(); }),
            scrapHounds.end()
        );

        automatons.erase(
            std::remove_if(automatons.begin(), automatons.end(),
                [](const Automaton& enemy) { return !enemy.isAlive(); }),
            automatons.end()
        );

        for (auto& enemy : scrapHounds) {
            enemy.update(*map, player->getPosition(), deltaTime);
            
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

        for (auto& automaton : automatons) {
            automaton.update(*map, player->getPosition(), deltaTime);
            automaton.checkProjectileCollisions(*player);
            
            if (automaton.isAlive()) {
                Rectangle automatonRect = automaton.getHitbox();
                
                Vector2 playerPos = player->getPosition();
                Rectangle playerRect = { playerPos.x, playerPos.y, 32, 32 };
                
                if (CheckCollisionRecs(automatonRect, playerRect) && player->canTakeDamage()) {
                    player->takeDamage(10);
                }
            }
        }

        if (player->getHealth() <= 0) {
            currentState = GameState::GAME_OVER;
        }

    } else if (currentState == GameState::PAUSED) {
        uiController->update(deltaTime, currentState);
    } else if (currentState == GameState::GAME_OVER) {
        if (inputManager.isActionPressed(Core::InputAction::INTERACT)) {
            resetGame();
        }
    }
}

void Game::render(float interpolation) {
    if (currentState == GameState::TITLE) {
        BeginDrawing();
        ClearBackground(BLACK);
        UI::UIAction action = uiController->draw(currentState);
        
        if (fadingToPlay) {
            fadeAlpha += GetFrameTime() * 1.5f;
            if (fadeAlpha >= 1.0f) {
                fadeAlpha = 1.0f;
                resetGame();
                fadingToPlay = false;
                fadeAlpha = 0.0f;
            }
            DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, fadeAlpha));
        } else if (action == UI::UIAction::PLAY) {
            fadingToPlay = true;
        }
        
        if (action == UI::UIAction::QUIT) {
            gameLoop->stop();
        }
        EndDrawing();
        return;
    }
    
    if (currentState == GameState::PLAYING || currentState == GameState::PAUSED) {
        uiController->update(GetFrameTime(), currentState, player.get());
        
        BeginTextureMode(sceneTexture);
        ClearBackground(BLACK);
        DrawTexture(fisheyeBackground, 0, 0, WHITE);
        BeginMode2D(camera->getCamera());
        map->draw(camera->getCamera());
        player->draw();
        ParticleSystem::getInstance().draw();

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
        
        for (const auto& automaton : automatons) {
            if (automaton.isAlive()) {
                Rectangle automatonRect = automaton.getHitbox();
                if (CheckCollisionRecs(automatonRect, cameraViewWorld)) {
                    automaton.draw();
                    if (IsKeyDown(KEY_TAB)) { 
                        DrawRectangleLines((int)automatonRect.x, (int)automatonRect.y, (int)automatonRect.width, (int)automatonRect.height, BLUE);
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
        
        if (currentState == GameState::PAUSED) {
            UI::UIAction action = uiController->draw(currentState);
            if (action == UI::UIAction::RESUME) {
                currentState = GameState::PLAYING;
            } else if (action == UI::UIAction::QUIT_TO_MENU) {
                currentState = GameState::TITLE;
            }
        } else {
            uiController->draw(currentState, player.get());
        }

        EndDrawing();
    }
}