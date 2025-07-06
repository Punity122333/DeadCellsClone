#include "Game.hpp"
#include "raylib.h"
#include "effects/ParticleSystem.hpp"
#include "core/Core.hpp"

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
    
    if (currentState == GameState::GAME_OVER) {
        printf("[GameRendering] Rendering GAME_OVER state\n");
        BeginDrawing();
        ClearBackground(BLACK);
        UI::UIAction action = uiController->draw(currentState);
        if (action == UI::UIAction::RESTART) {
            printf("[GameRendering] Restart action triggered\n");
            resetGame();
            printf("[GameRendering] resetGame() completed\n");
        } else if (action == UI::UIAction::QUIT_TO_MENU) {
            currentState = GameState::TITLE;
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
            printf("[GameRendering] Drawing pause menu\n");
            UI::UIAction action = uiController->draw(currentState);
            if (action == UI::UIAction::RESUME) {
                currentState = GameState::PLAYING;
            } else if (action == UI::UIAction::QUIT_TO_MENU) {
                currentState = GameState::TITLE;
            }
        } else {
            uiController->draw(currentState, player.get(), map.get());
        }

        EndDrawing();
    }
}
