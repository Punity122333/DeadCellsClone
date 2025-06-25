#pragma once
#include "Spawner.hpp"
#include "map/Map.hpp"
#include "Player.hpp"
#include "Camera.hpp"
#include "enemies/ScrapHound.hpp"
#include "enemies/Automaton.hpp"
#include "GameUI.hpp"
#include "TitleScreenUI.hpp" // Include TitleScreenUI
#include <memory>
#include <raylib.h>
#include <vector>

enum class GameState {
    TITLE, // Added title screen state
    PLAYING,
    GAME_OVER
};

class Game {
public:
    Game();
    ~Game();

    void run();
    void resetGame(); // Added resetGame declaration
    
private:
    const int screenWidth = 1280;
    const int screenHeight = 720;
    Shader bloomShader;
    std::unique_ptr<Map> map;
    std::unique_ptr<Player> player;
    std::unique_ptr<GameCamera> camera;
    RenderTexture2D sceneTexture;
    Texture2D fisheyeBackground;
    Shader chromaticAberrationShader;
    Shader* activeShader;
    std::vector<ScrapHound> scrapHounds;
    std::vector<Automaton> automatons; // Add automatons vector
    Spawner spawner;
    GameState currentState;
    std::vector<Texture2D> tileTextures; // Add to Game class
    std::unique_ptr<GameUI> gameUI;
    std::unique_ptr<TitleScreenUI> titleScreenUI; // Add title screen UI
};