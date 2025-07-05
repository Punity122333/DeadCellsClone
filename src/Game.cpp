#include "Game.hpp"
#include "raylib.h"
#include "Camera.hpp"
#include "FishEyeGradient.hpp"
#include "Spawner.hpp" 
#include "ui/UIController.hpp"
#include "ui/LoadingScreenComponent.hpp"
#include "effects/ParticleSystem.hpp"
#include "core/Core.hpp"
#include "core/GlobalThreadPool.hpp"
#include "effects/ParticleThreadPool.hpp"
#include <algorithm>
#include <filesystem>
#include <thread>
#include <chrono>

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

    resourceManager.setSearchPaths({"../resources/", "./resources/", "../shader/", "./shader/"});
    

    
    resourceManager.setHotReloadEnabled(true);
    

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
    mapGenerationInProgress.store(false, std::memory_order_release);
    mapGenerationComplete.store(true, std::memory_order_release);
    
    if (mapGenerationFuture.valid()) {
        try {
            mapGenerationFuture.wait();
        } catch (...) {
        }
    }
    
    
    camera.reset();
    player.reset();
    map.reset();
    
    
    GlobalThreadPool::getInstance().shutdown();
    ParticleThreadPool::getInstance().shutdown();
    
    
    if (Core::IsInitialized()) {
        try {
            auto& resourceManager = Core::GetResourceManager();
            
        } catch (...) {
        }
    }
    
    CloseWindow();
}

void Game::initializeResources() {
    auto& resourceManager = Core::GetResourceManager();
    
    Color innerCyan = { 0, 80, 80, 255 };
    Color outerPrussianBlue = { 0, 30, 50, 255 };
    fisheyeBackground = CreateFisheyeGradient(screenWidth, screenHeight, innerCyan, outerPrussianBlue);

    
    std::vector<std::string> tilePaths;
    int numTiles = 0;
    for (int i = 0; ; ++i) {
        char path_buffer[64];
        snprintf(path_buffer, sizeof(path_buffer), GamePaths::Tile, i);
        if (!std::filesystem::exists(path_buffer)) break;
        tilePaths.push_back(path_buffer);
        numTiles++;
    }
    
    
    resourceManager.preloadBatch(tilePaths);
    
    
    tileTextureHandles.clear();
    tileTextures.clear();
    for (const auto& path : tilePaths) {
        auto handle = resourceManager.getTexture(path);
        if (handle.isValid()) {
            tileTextureHandles.push_back(handle);
            tileTextures.push_back(*handle.get());
        }
    }

    
    sceneTexture = LoadRenderTexture(screenWidth, screenHeight);
    
    
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
    
    currentState = GameState::LOADING;
    loadingStartTime = GetTime(); 
    uiController->getLoadingScreen()->setProgress(0.0f);
    
    
    scrapHounds.clear();
    automatons.clear();
    automataTimer = 0.0f;
    fadeAlpha = 0.0f;
    fadingToPlay = false;
    
    
    mapGenerationInProgress = true;
    mapGenerationComplete = false;
    
    auto mapGenerationTask = [this]() {
        try {
            
            auto progressCallback = [this](float progress) {
                if (uiController && uiController->getLoadingScreen()) {
                    uiController->getLoadingScreen()->setProgress(progress);
                }
            };
            
            
            progressCallback(0.01f);
            
            
            printf("[Game] Starting map generation (500x300)...\n");
            auto start_time = std::chrono::high_resolution_clock::now();
            
            auto newMap = std::make_unique<Map>(500, 300, tileTextures, progressCallback);
            
            auto map_end_time = std::chrono::high_resolution_clock::now();
            auto map_duration = std::chrono::duration_cast<std::chrono::milliseconds>(map_end_time - start_time);
            printf("[Game] Map generation completed in %ld ms\n", map_duration.count());
            
            printf("[Game] Creating player...\n");
            auto player_start_time = std::chrono::high_resolution_clock::now();
            
            auto newPlayer = std::make_unique<Player>(*newMap);
            
            auto player_end_time = std::chrono::high_resolution_clock::now();
            auto player_duration = std::chrono::duration_cast<std::chrono::milliseconds>(player_end_time - player_start_time);
            printf("[Game] Player creation completed in %ld ms\n", player_duration.count());
            
            printf("[Game] Creating camera...\n");
            auto newCamera = std::make_unique<GameCamera>(screenWidth, screenHeight, *newPlayer);
            
            
            std::vector<ScrapHound> newScrapHounds;
            std::vector<Automaton> newAutomatons;
            
            
            printf("[Game] Spawning enemies...\n");
            auto spawn_start_time = std::chrono::high_resolution_clock::now();
            
            spawner.spawnEnemiesInRooms(*newMap, newScrapHounds, newAutomatons);
            
            auto spawn_end_time = std::chrono::high_resolution_clock::now();
            auto spawn_duration = std::chrono::duration_cast<std::chrono::milliseconds>(spawn_end_time - spawn_start_time);
            printf("[Game] Enemy spawning completed in %ld ms\n", spawn_duration.count());

            map = std::move(newMap);
            player = std::move(newPlayer);
            camera = std::move(newCamera);
            scrapHounds = std::move(newScrapHounds);
            automatons = std::move(newAutomatons);
            
            
            progressCallback(1.0f);
            
            
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            
            
            mapGenerationInProgress.store(false, std::memory_order_release);
            mapGenerationComplete.store(true, std::memory_order_release);
        } catch (const std::exception& e) {
            
            if (uiController && uiController->getLoadingScreen()) {
                uiController->getLoadingScreen()->setProgress(1.0f);
            }
            mapGenerationInProgress.store(false, std::memory_order_release);
            mapGenerationComplete.store(true, std::memory_order_release);
        } catch (...) {
            
            if (uiController && uiController->getLoadingScreen()) {
                uiController->getLoadingScreen()->setProgress(1.0f);
            }
            mapGenerationInProgress.store(false, std::memory_order_release);
            mapGenerationComplete.store(true, std::memory_order_release);
        }
    };  
    
    mapGenerationFuture = GlobalThreadPool::getInstance().getMainPool().enqueue(mapGenerationTask);
}

void Game::startNewGame() {
    resetGame();
    
    std::atomic_thread_fence(std::memory_order_release);
    fadingToPlay = true;
}

void Game::run() {
    gameLoop->run();
}

void Game::showResourceStats() {
    auto& resourceManager = Core::GetResourceManager();
    auto stats = resourceManager.getMemoryStats();

}

void Game::update(float deltaTime) {
    Core::GetInputManager().update(deltaTime);
    
    std::atomic_thread_fence(std::memory_order_acquire);
    if (fadingToPlay) {
        fadeAlpha += deltaTime * 2.0f;
        if (fadeAlpha >= 1.0f) {
            fadeAlpha = 1.0f;
            startNewGame();
            fadingToPlay = false;
            fadeAlpha = 0.0f;
        }
    }

    auto& inputManager = Core::GetInputManager();
    auto& eventManager = Core::GetEventManager();
    auto& resourceManager = Core::GetResourceManager();
    
    inputManager.update(deltaTime);
    eventManager.processEvents();
    
    
    
    resourceManager.checkForHotReload();
    
    
    if (inputManager.isActionHeld(Core::InputAction::DEBUG_TOGGLE)) {
        auto stats = resourceManager.getMemoryStats();
        
    }
    
    if (currentState == GameState::TITLE) {
        uiController->update(deltaTime, currentState);
        return;
    }

    if (currentState == GameState::LOADING) {

        if (mapGenerationComplete.load(std::memory_order_acquire)) {
            
            if (mapGenerationFuture.valid()) {
                try {
                    mapGenerationFuture.wait();
                    currentState = GameState::PLAYING;
                } catch (const std::exception&) {
                    currentState = GameState::PLAYING;
                }
            } else {
                currentState = GameState::PLAYING;
            }
        } else {
            
            float loadingTime = GetTime() - loadingStartTime;
            if (loadingTime > loadingTimeoutSeconds) {
                
                mapGenerationInProgress.store(false, std::memory_order_release);
                mapGenerationComplete.store(true, std::memory_order_release);
                if (uiController && uiController->getLoadingScreen()) {
                    uiController->getLoadingScreen()->setProgress(1.0f);
                }
            }
        }
        uiController->update(deltaTime, currentState);
        return;
    }

    if (currentState == GameState::PLAYING) {

        if (mapGenerationInProgress.load(std::memory_order_acquire) || !map || !player || !camera) {
            uiController->update(deltaTime, currentState);
            return;
        }
        
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
    
    if (currentState == GameState::LOADING) {
        BeginDrawing();
        ClearBackground(BLACK);
        UI::UIAction action = uiController->draw(currentState);

        
        EndDrawing();
        return;
    }
    
    if (currentState == GameState::TITLE) {
        BeginDrawing();
        ClearBackground(BLACK);
        UI::UIAction action = uiController->draw(currentState);
        
        if (fadingToPlay) {
            fadeAlpha += GetFrameTime() * 1.5f;
            if (fadeAlpha >= 1.0f) {
                fadeAlpha = 1.0f;
                startNewGame();
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
        
        if (mapGenerationInProgress || !map || !player || !camera) {
            BeginDrawing();
            ClearBackground(BLACK);
            if (uiController) {
                uiController->draw(GameState::LOADING);
            }
            EndDrawing();
            return;
        }
        
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
