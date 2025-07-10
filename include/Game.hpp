#pragma once
#include "Spawner.hpp"
#include "map/Map.hpp"
#include "Player.hpp"
#include "Camera.hpp"
#include "enemies/EnemyManager.hpp"
#include "ui/UIController.hpp"
#include "core/GameLoop.hpp"
#include "core/ResourceManager.hpp"
#include <memory>
#include <raylib.h>
#include <vector>
#include <future>
#include <atomic>

enum class GameState {
    LOADING,
    TITLE,
    PLAYING,
    PAUSED,
    GAME_OVER
};

class Game {
public:
    Game();
    ~Game();

    void run();
    void resetGame();
    void startNewGame();
    void showResourceStats(); // Debug method to display resource usage
    
private:
    const int screenWidth = 1920;
    const int screenHeight = 1080;
    
    std::unique_ptr<Core::GameLoop> gameLoop;
    
    Shader bloomShader;
    std::unique_ptr<Map> map;
    std::unique_ptr<Player> player;
    std::unique_ptr<GameCamera> camera;
    RenderTexture2D sceneTexture;
    Texture2D fisheyeBackground;
    Shader chromaticAberrationShader;
    Shader screenshakeShader;
    Shader* activeShader;
    EnemyManager enemyManager;
    Spawner spawner;
    GameState currentState;
    std::vector<Texture2D> tileTextures;
    std::vector<Core::ResourceHandle<Texture2D>> tileTextureHandles;
    Core::ResourceHandle<Shader> bloomShaderHandle;
    Core::ResourceHandle<Shader> chromaticAberrationShaderHandle;
    Core::ResourceHandle<Shader> screenshakeShaderHandle;
    std::unique_ptr<UI::UIController> uiController;
    
    float automataTimer;
    const float automataInterval = 5.0f;
    float fadeAlpha;
    bool fadingToPlay;
    bool gameOverTriggered;
    bool resetInProgress;
    float pauseDebounceTimer;
    

    std::future<void> mapGenerationFuture;
    std::atomic<bool> mapGenerationInProgress{false};
    std::atomic<bool> mapGenerationComplete{false};
    float loadingStartTime;
    const float loadingTimeoutSeconds = 30.0f;

    std::unique_ptr<Map> tempMap;
    std::unique_ptr<Player> tempPlayer;
    std::unique_ptr<GameCamera> tempCamera;
    EnemyManager tempEnemyManager;
    
    void update(float deltaTime);
    void render(float interpolation);
    void handleInput();
    void initializeResources();
    

};