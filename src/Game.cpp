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
    
    // Set up resource search paths
    resourceManager.setSearchPaths({"../resources/", "./resources/", "../shader/", "./shader/"});
    
    // Enable hot-reloading in debug builds
    #ifdef DEBUG
    resourceManager.setHotReloadEnabled(true);
    #endif
    
    // Load icon using ResourceManager
    auto iconHandle = resourceManager.loadTexture(GamePaths::Icon);
    if (iconHandle.isValid()) {
        Image icon = LoadImageFromTexture(*iconHandle.get());
        ImageFormat(&icon, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8); 
        SetWindowIcon(icon);
        UnloadImage(icon);
    }

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
    
    // ResourceManager handles cleanup automatically
    camera.reset();
    player.reset();
    map.reset();
    CloseWindow();
}

void Game::initializeResources() {
    auto& resourceManager = Core::GetResourceManager();
    
    Color innerCyan = { 0, 80, 80, 255 };
    Color outerPrussianBlue = { 0, 30, 50, 255 };
    fisheyeBackground = CreateFisheyeGradient(screenWidth, screenHeight, innerCyan, outerPrussianBlue);

    // Load tile textures using ResourceManager
    std::vector<std::string> tilePaths;
    int numTiles = 0;
    for (int i = 0; ; ++i) {
        char path_buffer[64];
        snprintf(path_buffer, sizeof(path_buffer), GamePaths::Tile, i);
        if (!std::filesystem::exists(path_buffer)) break;
        tilePaths.push_back(path_buffer);
        numTiles++;
    }
    
    // Preload all tile textures at once
    resourceManager.preloadBatch(tilePaths);
    
    // Get the loaded textures
    tileTextureHandles.clear();
    tileTextures.clear();
    for (const auto& path : tilePaths) {
        auto handle = resourceManager.getTexture(path);
        if (handle.isValid()) {
            tileTextureHandles.push_back(handle);
            tileTextures.push_back(*handle.get());
        }
    }

    map = std::make_unique<Map>(500, 300, tileTextures);
    player = std::make_unique<Player>(*map);
    camera = std::make_unique<GameCamera>(screenWidth, screenHeight, *player);

    spawner.spawnEnemiesInRooms(*map, scrapHounds, automatons);

    sceneTexture = LoadRenderTexture(screenWidth, screenHeight);
    
    // Load shaders using ResourceManager
    bloomShaderHandle = resourceManager.loadShader("", GamePaths::BloomShader);
    chromaticAberrationShaderHandle = resourceManager.loadShader("", GamePaths::ChromaticAberrationShader);
    
    if (bloomShaderHandle.isValid()) {
        bloomShader = *bloomShaderHandle.get();
        activeShader = bloomShaderHandle.get();
    }
    if (chromaticAberrationShaderHandle.isValid()) {
        chromaticAberrationShader = *chromaticAberrationShaderHandle.get();
    }
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

void Game::showResourceStats() {
    auto& resourceManager = Core::GetResourceManager();
    auto stats = resourceManager.getMemoryStats();
    
    // This could be displayed in a debug overlay or console
    // For now, we'll just track the stats for potential use
    // In a real implementation, you might render this on screen
}

void Game::update(float deltaTime) {
    auto& inputManager = Core::GetInputManager();
    auto& eventManager = Core::GetEventManager();
    auto& resourceManager = Core::GetResourceManager();
    
    inputManager.update(deltaTime);
    eventManager.processEvents();
    
    // Check for hot-reload in debug builds
    #ifdef DEBUG
    resourceManager.checkForHotReload();
    #endif
    
    // Display resource statistics when debug key is held
    if (inputManager.isActionHeld(Core::InputAction::DEBUG_TOGGLE)) {
        auto stats = resourceManager.getMemoryStats();
        // Stats can be displayed in debug overlay
    }
    
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
        player->update(deltaTime, *map, camera->getCamera(), scrapHounds, automatons, inputManager);
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
        uiController->update(deltaTime, currentState);
    }
}

void Game::render(float interpolation) {
    auto& inputManager = Core::GetInputManager();
    
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
                    if (inputManager.isActionHeld(Core::InputAction::DEBUG_TOGGLE)) { 
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
                    if (inputManager.isActionHeld(Core::InputAction::DEBUG_TOGGLE)) { 
                        DrawRectangleLines((int)automatonRect.x, (int)automatonRect.y, (int)automatonRect.width, (int)automatonRect.height, BLUE);
                    }
                }
            }
        }
        
        if (inputManager.isActionHeld(Core::InputAction::DEBUG_TOGGLE) && player->isAttacking()) {
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
        } else if (currentState == GameState::GAME_OVER) {
            UI::UIAction action = uiController->draw(currentState);
            if (action == UI::UIAction::RESTART) {
                resetGame();
            } else if (action == UI::UIAction::QUIT_TO_MENU) {
                currentState = GameState::TITLE;
            }
        } else {
            uiController->draw(currentState, player.get());
        }

        EndDrawing();
    }
}