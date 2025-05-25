#pragma once 

#include "Map.hpp"
#include "Player.hpp"
#include "Camera.hpp"

class Game {
    public : 
        Game();
        ~Game();
        
        void run();
    private:
        const int screenWidth = 1280;
        const int screenHeight = 720;

        Player player;
        Map map;
        GameCamera camera;

};