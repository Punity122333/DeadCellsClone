#pragma once

#include "Map.hpp"
#include "Player.hpp"
#include "Camera.hpp"
#include <memory>
#include <raylib.h>

class Game {
public:
    Game();
    ~Game();

    void run();
private:
    const int screenWidth = 1280;
    const int screenHeight = 720;
    Shader bloomShader;
    std::unique_ptr<Map> map;
    std::unique_ptr<Player> player;
    std::unique_ptr<GameCamera> camera;
    RenderTexture2D sceneTexture;
    Texture2D fisheyeBackground;
};