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
        bool ledgeGrabbed = false;
        Vector2 ledgeGrabPos = {0};

        Vector2 velocity;
        float dropTimer = 0.0f;
        const float DROP_TIME = 0.5f;
        const float LADDER_JUMP_LEEWAY = 10.0f;
        bool dropDown;
        float width = 32;
        float height = 60;
        float speed = 450;
        bool onGround = false;
        bool onRope = false;
        float coyoteTimer = 0.0f;
        static constexpr float COYOTE_TIME = 0.1f;

        void applyGravity(float dt);
        void updateLadderState(const Map& map);
        void updateWallState(const Map& map);
        void updateLedgeGrab(const Map& map);
        void handleMovementInput(float dt);
        void handleDropThrough(const Map& map);
        void updateDropTimer(float dt);
        Vector2 computeNextPosition(float dt);
        void handleCollisions(Vector2& nextPos, const Map& map, float dt);
        void updateCoyoteTimer(float dt);
        void handleJumpInput(const Map& map, float dt);
        void handleLedgeGrabInput();

        bool onLadder = false;
        bool canLadderJump = false;
        bool touchingWallLeft = false;
        bool touchingWallRight = false;
        bool canWallJump = false;
};

