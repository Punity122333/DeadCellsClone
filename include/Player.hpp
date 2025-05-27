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
        float dropTimer = 0.0f;
        const float DROP_TIME = 0.5f;
        const float LADDER_JUMP_LEEWAY = 10.0f;
        bool dropDown;
        float width = 32;
        float height = 48;
        float speed = 450;
        bool onGround = false;
        float coyoteTimer = 0.0f;
        static constexpr float COYOTE_TIME = 0.1f;
};

