#include "Game.hpp"
#include "core/Core.hpp"
#include "effects/ParticleSystem.hpp"
#include "ui/LoadingScreenComponent.hpp"
#include "enemies/Automaton.hpp"


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

    if (pauseDebounceTimer > 0.0f) {
        pauseDebounceTimer -= deltaTime;
    }
    
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
                    
                    if (tempMap && tempPlayer && tempCamera) {
                        printf("[Game] Moving objects from temp storage to main game state (main thread)\n");
                        
                        if (player) {
                            printf("[Game] Cleaning up old player resources\n");
                            player->cleanup();
                        }
                        
                        std::unique_ptr<Map> oldMap = std::move(map);
                        std::unique_ptr<Player> oldPlayer = std::move(player);
                        std::unique_ptr<GameCamera> oldCamera = std::move(camera);
                        EnemyManager oldEnemyManager = std::move(enemyManager);
                        
                        map = std::move(tempMap);
                        player = std::move(tempPlayer);
                        camera = std::move(tempCamera);
                        enemyManager = std::move(tempEnemyManager);
                        
                        oldMap.reset();
                        oldPlayer.reset();
                        oldCamera.reset();
                        oldEnemyManager.clearEnemies();
                        
                        printf("[Game] Objects successfully moved to main game state\n");
                    } else {
                        printf("[Game] Warning: Some temp objects are null during transfer\n");
                    }
                    
                    currentState = GameState::PLAYING;
                } catch (const std::exception& e) {
                    printf("[Game] Exception during async completion: %s\n", e.what());
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
        map->updateLavaFlow(deltaTime);
        map->updateParticles(deltaTime, player->getPosition());
        ParticleSystem::getInstance().update(deltaTime);
        player->update(deltaTime, *map, camera->getCamera(), enemyManager, inputManager);
        camera->update(*player, *map, deltaTime);

        enemyManager.updateEnemies(*map, player->getPosition(), deltaTime, *camera);
        enemyManager.removeDeadEnemies();

        auto scrapHounds = enemyManager.getEnemiesOfType(EnemyType::SCRAP_HOUND);
        for (auto* enemy : scrapHounds) {
            Vector2 enemyPos = enemy->getPosition();
            Rectangle enemyRect = { enemyPos.x, enemyPos.y, 32, 32 };
            
            Vector2 playerPos = player->getPosition();
            Rectangle playerRect = { playerPos.x, playerPos.y, 32, 32 };
            
            if (CheckCollisionRecs(enemyRect, playerRect) && player->canTakeDamage()) {
                player->takeDamage(5);
                camera->addScreenshake(0.3f, 0.2f);
            }
        }

        auto automatons = enemyManager.getEnemiesOfType(EnemyType::AUTOMATON);
        for (auto* enemy : automatons) {
            Automaton* automaton = static_cast<Automaton*>(enemy);
            automaton->checkProjectileCollisions(*player, *camera);
            
            Rectangle automatonRect = automaton->getHitbox();
            
            Vector2 playerPos = player->getPosition();
            Rectangle playerRect = { playerPos.x, playerPos.y, 32, 32 };
            
            if (CheckCollisionRecs(automatonRect, playerRect) && player->canTakeDamage()) {
                player->takeDamage(10);
                camera->addScreenshake(0.5f, 0.3f);
            }
        }

        if (player->getHealth() <= 0 && !gameOverTriggered) {
            printf("[Game] Player health reached zero, transitioning to GAME_OVER state\n");
            gameOverTriggered = true;
            currentState = GameState::GAME_OVER;
        }

    } else if (currentState == GameState::PAUSED) {
        uiController->update(deltaTime, currentState);
    } else if (currentState == GameState::GAME_OVER) {
        uiController->update(deltaTime, currentState);
    }
}
