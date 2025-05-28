#include "Player.hpp"
#include "raymath.h"
#include <raylib.h>

Player::Player(const Map &map) {
    position = {50, 8500};
    velocity = {0, 0};
}

void Player::update(float dt, const Map& map) {
    applyGravity(dt);
    updateLadderState(map);
    updateWallState(map);
    updateLedgeGrab(map);
    handleMovementInput(dt);
    handleDropThrough(map);
    updateDropTimer(dt);
    Vector2 nextPos = computeNextPosition(dt);
    handleCollisions(nextPos, map, dt);
    updateCoyoteTimer(dt);
    handleJumpInput(map, dt);
    handleLedgeGrabInput();
    position = nextPos;
}

void Player::applyGravity(float dt) {
    if (onLadder) {
        velocity.y = 0; // No gravity on ladder
    } else if (onRope) {
        // Slide down rope slowly unless pressing up or jumping
        if (IsKeyDown(KEY_W) || IsKeyPressed(KEY_SPACE)) {
            velocity.y = 0;
        } else {
            velocity.y = 60; // Slow slide down (adjust as needed)
        }
    } else {
        velocity.y += 1000 * dt;
    }
}

void Player::updateLadderState(const Map& map) {
    int playerTileX = (int)((position.x + width / 2) / 32);
    int playerTileY = (int)((position.y + height / 2) / 32);

    onLadder = false;
    bool onRope = false;
    const int leeway = 8;
    int leftX = (int)((position.x + width / 2 - leeway) / 32);
    int rightX = (int)((position.x + width / 2 + leeway) / 32);

    for (int x = leftX; x <= rightX; ++x) {
        if (map.isLadderTile(x, playerTileY)) {
            onLadder = true;
            break;
        }
        if (map.isRopeTile(x, playerTileY)) {
            onRope = true;
        }
    }

    canLadderJump = false;
    int feetTileY = (int)((position.y + height - 1) / 32);
    int bodyTileY = (int)((position.y + height / 2) / 32);
    for (int x = leftX; x <= rightX; ++x) {
        if ((map.isLadderTile(x, feetTileY) || map.isRopeTile(x, feetTileY)) &&
            !(map.isLadderTile(x, bodyTileY) || map.isRopeTile(x, bodyTileY))) {
            canLadderJump = true;
            break;
        }
    }

    // Store rope state for use in gravity
    this->onRope = onRope && !onLadder;
}

void Player::updateWallState(const Map& map) {
    touchingWallLeft = false;
    touchingWallRight = false;
    const int wallCheckOffset = 2;
    int wallCheckTop = (int)((position.y + wallCheckOffset) / 32);
    int wallCheckBottom = (int)((position.y + height - wallCheckOffset) / 32);

    int leftWallX = (int)((position.x - 1) / 32);
    for (int y = wallCheckTop; y <= wallCheckBottom; ++y) {
        if (map.isSolidTile(leftWallX, y)) {
            touchingWallLeft = true;
            break;
        }
    }
    int rightWallX = (int)((position.x + width + 1) / 32);
    for (int y = wallCheckTop; y <= wallCheckBottom; ++y) {
        if (map.isSolidTile(rightWallX, y)) {
            touchingWallRight = true;
            break;
        }
    }
    canWallJump = !onGround && (touchingWallLeft || touchingWallRight);
}

void Player::updateLedgeGrab(const Map& map) {
    static bool climbingLedge = false;
    static Vector2 climbStartPos = {0, 0};
    static Vector2 climbEndPos = {0, 0};
    static float climbTimer = 0.0f;
    const float climbDuration = 0.25f; // seconds

    if (climbingLedge) {
        climbTimer += GetFrameTime();
        float t = fminf(climbTimer / climbDuration, 1.0f);
        position.x = climbStartPos.x + (climbEndPos.x - climbStartPos.x) * t;
        position.y = climbStartPos.y + (climbEndPos.y - climbStartPos.y) * t;
        velocity = {0, 0};
        ledgeGrabbed = true;

        if (t >= 1.0f) {
            climbingLedge = false;
            ledgeGrabbed = false;
        }
        return;
    }

    if (!onGround && !onLadder && velocity.y > 0 && !ledgeGrabbed && !climbingLedge) {
        int dir = (IsKeyDown(KEY_A) ? -1 : (IsKeyDown(KEY_D) ? 1 : 0));
        if (dir != 0) {
            int handX = (int)((position.x + width / 2 + dir * width * 0.5f) / 32);
            int chestY = (int)((position.y + height * 0.5f) / 32);
            int headY = (int)((position.y + height * 0.3f) / 32);

            if (map.isSolidTile(handX, chestY) &&
                !map.isSolidTile(handX, headY)) {
                // Start smooth ledge climb
                climbStartPos = position;
                climbEndPos.x = handX * 32.0f - (dir == -1 ? width : 0);
                climbEndPos.y = (chestY * 32.0f) - height + 2;
                climbTimer = 0.0f;
                climbingLedge = true;
                velocity = {0, 0};
            }
        }
    }
}

void Player::handleMovementInput(float dt) {
    float targetVelX = 0.0f;
    if (onLadder || onRope) { // Allow climbing on both
        if (IsKeyDown(KEY_W)) {
            velocity.y = -speed;
        } else if (IsKeyDown(KEY_S)) {
            velocity.y = speed;
        } else if (onLadder) {
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
}

void Player::handleDropThrough(const Map& map) {
    if (onGround && IsKeyDown(KEY_S) && IsKeyPressed(KEY_SPACE)) {
        int belowTileY = (int)((position.y + height + 1) / 32);
        int belowTileX = (int)((position.x + width / 2) / 32);
        if (belowTileY < map.getHeight() - 1 && map.isSolidTile(belowTileX, belowTileY)) {
            dropTimer = DROP_TIME;
        }
    }
}

void Player::updateDropTimer(float dt) {
    if (dropTimer > 0.0f) {
        dropTimer -= dt;
    }
}

Vector2 Player::computeNextPosition(float dt) {
    return Vector2Add(position, Vector2Scale(velocity, dt));
}

void Player::handleCollisions(Vector2& nextPos, const Map& map, float dt) {
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

                bool climbingUp = onLadder && (velocity.y < 0);
                bool headInsideTile = (playerRect.y <= y * 32.0f + 31) && (playerRect.y > y * 32.0f - height + 1);
                if (climbingUp && headInsideTile) continue;

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
}

void Player::updateCoyoteTimer(float dt) {
    if (onGround)
        coyoteTimer = COYOTE_TIME;
    else
        coyoteTimer -= dt;
}

void Player::handleJumpInput(const Map& map, float dt) {
    static float wallJumpLockTimer = 0.0f;
    const float wallJumpLockTime = 0.15f;
    if (wallJumpLockTimer > 0.0f) wallJumpLockTimer -= dt;

    bool canJump = (coyoteTimer > 0.0f) || canLadderJump;

    if (IsKeyPressed(KEY_SPACE) && !IsKeyDown(KEY_S)) {
        if (canJump) {
            velocity.y = -700;
            coyoteTimer = 0.0f;
        } else if (canWallJump && wallJumpLockTimer <= 0.0f) {
            velocity.y = -700;
            if (touchingWallLeft) {
                velocity.x = 400;
            } else if (touchingWallRight) {
                velocity.x = -400;
            }
            wallJumpLockTimer = wallJumpLockTime;
        }
    }
}

void Player::handleLedgeGrabInput() {
    if (ledgeGrabbed) {
        position = ledgeGrabPos;

        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_W)) {
            velocity.y = -700;
            ledgeGrabbed = false;
        } else if (IsKeyPressed(KEY_S)) {
            velocity.y = 100;
            ledgeGrabbed = false;
        } else {
            // Stay grabbed
            return;
        }
    }
}


void Player::draw() const {
    DrawRectangle((int)position.x, (int)position.y, width, height, GREEN);
    DrawText(onGround ? "ON GROUND" : "IN AIR", 10, 180, 20, RED);
    DrawText(dropTimer > 0.0f ? "DROPPING" : "NOT DROPPING", 10, 210, 20, RED);
}

Vector2 Player::getPosition() const {
    return position;
}
