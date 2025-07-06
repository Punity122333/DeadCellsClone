#include "Game.hpp"
#include "raylib.h"
#include "ui/UIController.hpp"
#include "core/Core.hpp"
#include "core/GlobalThreadPool.hpp"
#include "effects/ParticleThreadPool.hpp"

const int screenWidth = 1920;
const int screenHeight = 1080;

Game::Game() 
    : automataTimer(0.0f)
    , fadeAlpha(0.0f)
    , fadingToPlay(false)
    , gameOverTriggered(false)
    , resetInProgress(false)
    , pauseDebounceTimer(0.0f)
{
    InitWindow(screenWidth, screenHeight, "Cellular Automata");
    SetExitKey(KEY_F4);

    gameLoop = std::make_unique<Core::GameLoop>(60, 60);
    gameLoop->setUpdateCallback([this](float deltaTime) { update(deltaTime); });
    gameLoop->setRenderCallback([this](float interpolation) { render(interpolation); });

    auto& resourceManager = Core::GetResourceManager();
    resourceManager.setSearchPaths({"../resources/", "./resources/", "../shader/", "./shader/"});
    resourceManager.setHotReloadEnabled(true);

    auto iconHandle = resourceManager.loadTexture("../resources/icon/CellularAutomata.png");
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
        if (event.action == Core::InputAction::PAUSE && event.pressed && pauseDebounceTimer <= 0.0f) {
            printf("[Game] Pause key pressed, current state: %d\n", static_cast<int>(currentState));
            if (currentState == GameState::PLAYING) {
                printf("[Game] Transitioning from PLAYING to PAUSED\n");
                currentState = GameState::PAUSED;
                pauseDebounceTimer = 0.2f; // 200ms debounce
            } else if (currentState == GameState::PAUSED) {
                printf("[Game] Transitioning from PAUSED to PLAYING\n");
                currentState = GameState::PLAYING;
                pauseDebounceTimer = 0.2f; // 200ms debounce
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

void Game::run() {
    gameLoop->run();
}
