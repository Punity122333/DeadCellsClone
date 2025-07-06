#include "Game.hpp"
#include "core/GlobalThreadPool.hpp"
#include "ui/LoadingScreenComponent.hpp"
#include "Spawner.hpp"
#include <thread>
#include <chrono>

void Game::resetGame() {
    if (resetInProgress) {
        printf("[Game] resetGame() already in progress, skipping\n");
        return;
    }
    
    printf("[Game] Starting resetGame...\n");
    resetInProgress = true;
    
    mapGenerationInProgress.store(false, std::memory_order_release);
    mapGenerationComplete.store(true, std::memory_order_release);
    
    if (mapGenerationFuture.valid()) {
        try {
            printf("[Game] Waiting for existing map generation to complete...\n");
            mapGenerationFuture.wait();
            printf("[Game] Existing map generation cleaned up\n");
        } catch (...) {
            printf("[Game] Exception while cleaning up existing map generation\n");
        }
    }
    
    currentState = GameState::LOADING;
    loadingStartTime = GetTime(); 
    uiController->getLoadingScreen()->setProgress(0.0f);
    
    scrapHounds.clear();
    automatons.clear();
    automataTimer = 0.0f;
    fadeAlpha = 0.0f;
    fadingToPlay = false;
    gameOverTriggered = false;
    
    if (player) {
        printf("[Game] Cleaning up existing player before reset\n");
        player->cleanup();
    }
    
    tempMap.reset();
    tempPlayer.reset();
    tempCamera.reset();
    tempScrapHounds.clear();
    tempAutomatons.clear();
    
    printf("[Game] Starting new map generation...\n");
    mapGenerationInProgress = true;
    mapGenerationComplete = false;
    
    auto mapGenerationTask = [this]() {
        try {
            printf("[Game] Map generation task started\n");
            
            auto progressCallback = [this](float progress) {
                if (uiController && uiController->getLoadingScreen()) {
                    uiController->getLoadingScreen()->setProgress(progress);
                }
            };
            
            progressCallback(0.01f);
            
            if (!mapGenerationInProgress.load(std::memory_order_acquire)) {
                printf("[Game] Map generation cancelled before starting\n");
                return;
            }
            
            printf("[Game] Starting map generation (500x300)...\n");
            auto start_time = std::chrono::high_resolution_clock::now();
            
            auto newMap = std::make_unique<Map>(500, 300, tileTextures, progressCallback);
            
            if (!mapGenerationInProgress.load(std::memory_order_acquire)) {
                printf("[Game] Map generation cancelled after map creation\n");
                return;
            }
            
            auto map_end_time = std::chrono::high_resolution_clock::now();
            auto map_duration = std::chrono::duration_cast<std::chrono::milliseconds>(map_end_time - start_time);
            printf("[Game] Map generation completed in %ld ms\n", map_duration.count());
            
            printf("[Game] Creating player...\n");
            auto player_start_time = std::chrono::high_resolution_clock::now();
            
            auto newPlayer = std::make_unique<Player>(*newMap);
            
            if (!mapGenerationInProgress.load(std::memory_order_acquire)) {
                printf("[Game] Map generation cancelled after player creation\n");
                return;
            }
            
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
            
            if (!mapGenerationInProgress.load(std::memory_order_acquire)) {
                printf("[Game] Map generation cancelled after enemy spawning\n");
                return;
            }
            
            auto spawn_end_time = std::chrono::high_resolution_clock::now();
            auto spawn_duration = std::chrono::duration_cast<std::chrono::milliseconds>(spawn_end_time - spawn_start_time);
            printf("[Game] Enemy spawning completed in %ld ms\n", spawn_duration.count());

            printf("[Game] Storing new objects for main thread assignment...\n");
            
            progressCallback(1.0f);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            
            printf("[Game] Map generation task completed successfully\n");
            
            tempMap = std::move(newMap);
            tempPlayer = std::move(newPlayer);
            tempCamera = std::move(newCamera);
            tempScrapHounds = std::move(newScrapHounds);
            tempAutomatons = std::move(newAutomatons);
            
            mapGenerationInProgress.store(false, std::memory_order_release);
            mapGenerationComplete.store(true, std::memory_order_release);
        } catch (const std::exception& e) {
            printf("[Game] Exception in map generation: %s\n", e.what());
            if (uiController && uiController->getLoadingScreen()) {
                uiController->getLoadingScreen()->setProgress(1.0f);
            }
            mapGenerationInProgress.store(false, std::memory_order_release);
            mapGenerationComplete.store(true, std::memory_order_release);
        } catch (...) {
            printf("[Game] Unknown exception in map generation\n");
            if (uiController && uiController->getLoadingScreen()) {
                uiController->getLoadingScreen()->setProgress(1.0f);
            }
            mapGenerationInProgress.store(false, std::memory_order_release);
            mapGenerationComplete.store(true, std::memory_order_release);
        }
    };  
    
    mapGenerationFuture = GlobalThreadPool::getInstance().getMainPool().enqueue(mapGenerationTask);
    printf("[Game] New map generation task enqueued\n");
    resetInProgress = false;
}

void Game::startNewGame() {
    resetGame();
    
    std::atomic_thread_fence(std::memory_order_release);
    fadingToPlay = true;
}
