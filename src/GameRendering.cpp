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

        enemyManager.drawEnemies();
        
        if (inputManager.isActionHeld(Core::InputAction::DEBUG_TOGGLE)) {
            auto allEnemies = enemyManager.getAllEnemies();
            for (auto* enemy : allEnemies) {
                Rectangle enemyRect = enemy->getHitbox();
                Color debugColor = RED;
                if (enemy->getType() == EnemyType::AUTOMATON) {
                    debugColor = BLUE;
                } else if (enemy->getType() == EnemyType::DETONODE) {
                    debugColor = YELLOW;
                }
                DrawRectangleLines((int)enemyRect.x, (int)enemyRect.y, (int)enemyRect.width, (int)enemyRect.height, debugColor);
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

        Shader* currentShader = activeShader;
        if (camera->getShakeIntensity() > 0.0f && screenshakeShaderHandle.isValid()) {
            currentShader = &screenshakeShader;

            Vector2 shakeOffset = camera->getShakeOffset();
            float shakeIntensity = camera->getShakeIntensity();
            
            int shakeOffsetLoc = GetShaderLocation(screenshakeShader, "shakeOffset");
            int shakeIntensityLoc = GetShaderLocation(screenshakeShader, "shakeIntensity");
            
            SetShaderValue(screenshakeShader, shakeOffsetLoc, &shakeOffset, SHADER_UNIFORM_VEC2);
            SetShaderValue(screenshakeShader, shakeIntensityLoc, &shakeIntensity, SHADER_UNIFORM_FLOAT);
        }
        
        BeginShaderMode(*currentShader);

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
