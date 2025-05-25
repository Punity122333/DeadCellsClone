#include "Player.hpp"
#include "raymath.h"
#include <raylib.h>

Player::Player(const Map &map) {
    position = {100, 100};
    velocity = {0, 0};
}

void Player::update(float dt, const Map& map) {
    velocity.y += 1000 * dt;

    if (IsKeyDown(KEY_A))
        velocity.x = -speed;
    else if (IsKeyDown(KEY_D))
        velocity.x = speed;
    else
        velocity.x = 0;

    Vector2 nextPos = Vector2Add(position, Vector2Scale(velocity, dt));
    Rectangle playerRect = { nextPos.x, nextPos.y, width, height };

    onGround = false;

    int tileXStart = (int)(playerRect.x / 32) - 1;
    int tileYStart = (int)(playerRect.y / 32) - 1;
    int tileXEnd = tileXStart + 3;
    int tileYEnd = tileYStart + 4;

    for (int y = tileYStart; y <= tileYEnd; y++) {
        for (int x = tileXStart; x <= tileXEnd; x++) {
            if (map.isSolidTile(x, y)) {
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

    if (IsKeyPressed(KEY_SPACE) && coyoteTimer > 0.0f) {
        velocity.y = -450;
        coyoteTimer = 0.0f;
    }

    position = nextPos;

    
}

void Player::draw() const {
    DrawRectangle((int)position.x, (int)position.y, width, height, GREEN);
    DrawText(onGround ? "ON GROUND" : "IN AIR", 10, 10, 20, RED);
}

Vector2 Player::getPosition() const {
    return position;
}
