#pragma once 
#include "Map.hpp"
#include "raylib.h"

class Player {
    public :
        Player(const Map &map);
        void update(float dt, const Map& map);
        void draw() const;
        
        Vector2 getPosition() const;

    private :   
        Vector2 position;
        Vector2 velocity;
        float width = 32;
        float height = 48;
        float speed = 200;
        bool onGround = false;
        float coyoteTimer = 0.0f;
        static constexpr float COYOTE_TIME = 0.1f;
};

