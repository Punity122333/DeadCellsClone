#include "Player.hpp"
#include "raymath.h"
#include <raylib.h>

Player::Player(const Map &map) {
    position = {50, 9500};
    velocity = {0, 0};
}

void Player::update(float dt, const Map& map) {
    velocity.y += 1000 * dt;

    bool onLadder = false;

    int playerTileX = (int)((position.x + width / 2) / 32);
    int playerTileY = (int)((position.y + height / 2) / 32);

    if (map.isLadderTile(playerTileX, playerTileY)) {
        onLadder = true;
    }

    bool canLadderJump = false;

    int centerTileX = (int)((position.x + width / 2) / 32);
    int feetTileY = (int)((position.y + height - 1) / 32);
    int bodyTileY = (int)((position.y + height / 2) / 32);

    const int leeway = 8;
    int leftX = (int)((position.x + width / 2 - leeway) / 32);
    int rightX = (int)((position.x + width / 2 + leeway) / 32);

    onLadder = false;
    for (int x = leftX; x <= rightX; ++x) {
        if (map.isLadderTile(x, playerTileY)) {
            onLadder = true;
            break;
        }
    }

    canLadderJump = false;
    for (int x = leftX; x <= rightX; ++x) {
        if (map.isLadderTile(x, feetTileY) && !map.isLadderTile(x, bodyTileY)) {
            canLadderJump = true;
            break;
        }
    }

    
    float targetVelX = 0.0f;
    if (onLadder) {
        if (IsKeyDown(KEY_W)) {
            velocity.y = -speed;
        } else if (IsKeyDown(KEY_S)) {
            velocity.y = speed;
        } else {
            velocity.y = 0;
        }
        if (IsKeyDown(KEY_A)) {
            targetVelX = -speed;
        } else if (IsKeyDown(KEY_D)) {
            targetVelX = speed;
        }
        velocity.x = targetVelX; 
    } else {
        if (IsKeyDown(KEY_A)) {
            targetVelX = -speed;
        } else if (IsKeyDown(KEY_D)) {
            targetVelX = speed;
        }
        if (onGround) {
            velocity.x = targetVelX;
        } else {
        
            const float airAccel = 2000.0f;
            if (velocity.x < targetVelX)
                velocity.x = fmin(velocity.x + airAccel * dt, targetVelX);
            else if (velocity.x > targetVelX)
                velocity.x = fmax(velocity.x - airAccel * dt, targetVelX);
        }
    }

    if (onGround && IsKeyPressed(KEY_S)) {
        int belowTileY = (int)((position.y + height + 1) / 32);
        int belowTileX = (int)((position.x + width / 2) / 32);
        if (belowTileY < map.getHeight() - 1 && map.isSolidTile(belowTileX, belowTileY)) {
            dropTimer = DROP_TIME;
        }
    }

    if (dropTimer > 0.0f) {
        dropTimer -= dt;
    }

    Vector2 nextPos = Vector2Add(position, Vector2Scale(velocity, dt));
    Rectangle playerRect = { nextPos.x, nextPos.y, width, height };

    onGround = false;

    int tileXStart = (int)(playerRect.x / 32) - 1;
    int tileYStart = (int)(playerRect.y / 32) - 1;
    int tileXEnd = tileXStart + 3;
    int tileYEnd = tileYStart + 4;

    if (dropTimer <= 0.0f) {
        for (int y = tileYStart; y <= tileYEnd; y++) {
            for (int x = tileXStart; x <= tileXEnd; x++) {

                if (!map.isSolidTile(x, y)) continue;

                bool isBelowTile = (y * 32.0f >= position.y + height) &&
                                (x == (int)((position.x + width / 2) / 32));
                if (dropTimer > 0.0f && isBelowTile) continue;

                Rectangle tileRect = { x * 32.0f, y * 32.0f, 32.0f, 32.0f };
                if (CheckCollisionRecs(playerRect, tileRect)) {
                    Rectangle intersection = GetCollisionRec(playerRect, tileRect);

                    if (intersection.width < intersection.height) {
                        if (playerRect.x < tileRect.x)
                            nextPos.x -= intersection.width;
                        else
                            nextPos.x += intersection.width;
                        velocity.x = 0;
                    } else {
                        if (playerRect.y < tileRect.y) {
                            nextPos.y -= intersection.height;
                            onGround = true;
                        } else {
                            nextPos.y += intersection.height;
                        }
                        velocity.y = 0;
                    }

                    playerRect.x = nextPos.x;
                    playerRect.y = nextPos.y;
                }
            }
        }

    }

    if (onGround)
        coyoteTimer = COYOTE_TIME;
    else
        coyoteTimer -= dt;

    bool canJump = (coyoteTimer > 0.0f) || canLadderJump;

    if (IsKeyPressed(KEY_SPACE) && canJump) {
        velocity.y = -700;
        coyoteTimer = 0.0f;
    }

    position = nextPos;
}


void Player::draw() const {
    DrawRectangle((int)position.x, (int)position.y, width, height, GREEN);
    DrawText(onGround ? "ON GROUND" : "IN AIR", 10, 180, 20, RED);
    DrawText(dropTimer > 0.0f ? "DROPPING" : "NOT DROPPING", 10, 210, 20, RED);
}

Vector2 Player::getPosition() const {
    return position;
}
