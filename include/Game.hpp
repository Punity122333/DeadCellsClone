#pragma once
#include "Spawner.hpp"
#include "map/Map.hpp"
#include "Player.hpp"
#include "Camera.hpp"
#include "enemies/ScrapHound.hpp"
#include "enemies/Automaton.hpp"
#include "ui/UIController.hpp"
#include "core/GameLoop.hpp"
#include <memory>
#include <raylib.h>
#include <vector>

enum class GameState {
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
    
private:
    const int screenWidth = 1280;
    const int screenHeight = 720;
    
    std::unique_ptr<Core::GameLoop> gameLoop;
    
    Shader bloomShader;
    std::unique_ptr<Map> map;
    std::unique_ptr<Player> player;
    std::unique_ptr<GameCamera> camera;
    RenderTexture2D sceneTexture;
    Texture2D fisheyeBackground;
    Shader chromaticAberrationShader;
    Shader* activeShader;
    std::vector<ScrapHound> scrapHounds;
    std::vector<Automaton> automatons;
    Spawner spawner;
    GameState currentState;
    std::vector<Texture2D> tileTextures;
    std::unique_ptr<UI::UIController> uiController;
    
    float automataTimer;
    const float automataInterval = 5.0f;
    float fadeAlpha;
    bool fadingToPlay;
    
    void update(float deltaTime);
    void render(float interpolation);
    void handleInput();
    void initializeResources();
};