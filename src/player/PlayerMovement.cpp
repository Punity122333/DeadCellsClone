#include "Player.hpp"
#include <raylib.h>
#include "raymath.h"

void Player::applyGravity(float dt) {
    if (onLadder) {
        velocity.y = 0; 
    } else if (onRope) {
        if (IsKeyDown(KEY_W) || IsKeyPressed(KEY_SPACE)) {
            velocity.y = 0;
        } else {
            velocity.y = 60; 
        }
    } else {
        velocity.y += 1000 * dt;
    }
}

void Player::updateLadderState(const Map& map) {
    int playerTileX = (int)((position.x + hitboxOffsetX + hitboxWidth / 2) / 32);
    int playerTileY = (int)((position.y + hitboxOffsetY + hitboxHeight / 2) / 32);

    onLadder = false;
    bool onRope = false;
    const int leeway = 8;
    int leftX = (int)((position.x + hitboxOffsetX + hitboxWidth / 2 - leeway) / 32);
    int rightX = (int)((position.x + hitboxOffsetX + hitboxWidth / 2 + leeway) / 32);

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
    int feetTileY = (int)((position.y + hitboxOffsetY + hitboxHeight - 1) / 32);
    int bodyTileY = (int)((position.y + hitboxOffsetY + hitboxHeight / 2) / 32);
    for (int x = leftX; x <= rightX; ++x) {
        if ((map.isLadderTile(x, feetTileY) || map.isRopeTile(x, feetTileY)) &&
            !(map.isLadderTile(x, bodyTileY) || map.isRopeTile(x, bodyTileY))) {
            canLadderJump = true;
            break;
        }
    }

    this->onRope = onRope && !onLadder;
}

void Player::updateWallState(const Map& map) {
    touchingWallLeft = false;
    touchingWallRight = false;
    const int wallCheckOffset = 2;
    int wallCheckTop = (int)((position.y + hitboxOffsetY + wallCheckOffset) / 32);
    int wallCheckBottom = (int)((position.y + hitboxOffsetY + hitboxHeight - wallCheckOffset) / 32);

    int leftWallX = (int)((position.x + hitboxOffsetX - 1) / 32);
    for (int y = wallCheckTop; y <= wallCheckBottom; ++y) {
        if (map.isSolidTile(leftWallX, y)) {
            touchingWallLeft = true;
            break;
        }
    }
    int rightWallX = (int)((position.x + hitboxOffsetX + hitboxWidth + 1) / 32);
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
    const float climbDuration = 0.25f; 

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
            int handX = (int)((position.x + hitboxOffsetX + hitboxWidth / 2 + dir * hitboxWidth * 0.5f) / 32);
            int chestY = (int)((position.y + hitboxOffsetY + hitboxHeight * 0.5f) / 32);
            int headY = (int)((position.y + hitboxOffsetY + hitboxHeight * 0.3f) / 32);

            if (map.isSolidTile(handX, chestY) &&
                !map.isSolidTile(handX, headY)) {
                climbStartPos = position;
                if (dir == -1) {
                    climbEndPos.x = handX * 32.0f - width;
                } else {
                    climbEndPos.x = (handX + 1) * 32.0f;
                }
                climbEndPos.y = chestY * 32.0f - height + 2;
                climbEndPos.y = (chestY * 32.0f - hitboxHeight) - hitboxOffsetY + 2;
                climbTimer = 0.0f;
                climbingLedge = true;
                velocity = {0, 0};
            }
        }
    }
}

void Player::handleMovementInput(float dt) {
    float targetVelX = 0.0f;
    static float dustTimer = 0.0f;

    if (onLadder || onRope) { 
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

            if (fabs(velocity.x) > 0.1f) {
                dustTimer -= dt;
                if (dustTimer <= 0.0f) {
                    Particle p;
                    p.position = { position.x + hitboxOffsetX + hitboxWidth/2, position.y + hitboxOffsetY + hitboxHeight - 4 };
                    p.velocity = { (float)GetRandomValue(-20, 20), (float)GetRandomValue(-60, -20) };
                    p.lifetime = 0.25f;
                    p.age = 0.0f;
                    dustParticles.push_back(p);
                    dustTimer = 0.04f; 
                }
            } else {
                dustTimer = 0.0f;
            }
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
        int belowTileY = (int)((position.y + hitboxOffsetY + hitboxHeight + 1) / 32);
        int belowTileX = (int)((position.x + hitboxOffsetX + hitboxWidth / 2) / 32);
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
            return;
        }
    }
}