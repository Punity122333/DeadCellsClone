#include "Player.hpp"
#include "enemies/ScrapHound.hpp"
#include "raymath.h"
#include <algorithm>
#include <raylib.h>
#include "weapons/WeaponTypes.hpp"  // Ensure this path is correct

Player::Player(const Map &map) {
    position = map.findEmptySpawn();
    velocity = {0, 0};
    
    // Initialize combat variables
    health = 100.0f;
    maxHealth = 100.0f;
    invincibilityTimer = 0.0f;
    facingRight = true;
    
    // Initialize weapon system with a default sword
    weapons.push_back(std::make_unique<Sword>());
    currentWeaponIndex = 0;
}

void Player::update(float dt, const Map& map) {
    // Update invincibility timer
    if (invincibilityTimer > 0.0f) {
        invincibilityTimer -= dt;
    }

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
    
    // Update current weapon
    if (weapons.size() > 0 && currentWeaponIndex < weapons.size()) {
        weapons[currentWeaponIndex]->update(dt);
    }
    
    // Handle input for attacks
    if (IsKeyPressed(KEY_J) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        attack();
    }
    
    // Handle weapon switching with number keys 1-9
    for (int i = 0; i < 9; i++) {
        if (IsKeyPressed(KEY_ONE + i) && i < weapons.size()) {
            switchWeapon(i);
        }
    }
    
    // Update facing direction based on movement
    if (velocity.x > 10.0f || IsKeyDown(KEY_D)) {
        facingRight = true;
    } else if (velocity.x < -10.0f || IsKeyDown(KEY_A)) {
        facingRight = false;
    }
    
    for (auto& p : dustParticles) {
        p.position = Vector2Add(p.position, Vector2Scale(p.velocity, dt));
        p.velocity.y += 600 * dt; 
        p.age += dt;
    }
    
    dustParticles.erase(
        std::remove_if(dustParticles.begin(), dustParticles.end(),
            [](const Particle& p) { return p.age > p.lifetime; }),
        dustParticles.end()
    );
}

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
            int handX = (int)((position.x + width / 2 + dir * width * 0.5f) / 32);
            int chestY = (int)((position.y + height * 0.5f) / 32);
            int headY = (int)((position.y + height * 0.3f) / 32);

            if (map.isSolidTile(handX, chestY) &&
                !map.isSolidTile(handX, headY)) {
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
                    p.position = { position.x + width/2, position.y + height - 4 };
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

                    bool isHorizontalCollision = intersection.width < intersection.height;
                    bool isMovingTowardsTile = (velocity.x > 0 && playerRect.x < tileRect.x) ||
                                               (velocity.x < 0 && playerRect.x > tileRect.x);
                    bool isNearGroundLevelOfTile = (playerRect.y + playerRect.height >= tileRect.y) &&
                                                   (playerRect.y + playerRect.height <= tileRect.y + 32 + 5); 

                    if (isHorizontalCollision && isMovingTowardsTile && isNearGroundLevelOfTile && onGround) {
                        if (!map.isSolidTile(x, y - 1)) {
                            int targetXTile = (int)((nextPos.x + width / 2) / 32); 
                            if (!map.isSolidTile(targetXTile, y - 1)) { 
                                nextPos.y = tileRect.y - height;
                                if (velocity.x > 0) {
                                    nextPos.x = tileRect.x - width;
                                } else {
                                    nextPos.x = tileRect.x + 32;
                                }
                                velocity.y = 0;
                                onGround = true;
                                playerRect.x = nextPos.x;
                                playerRect.y = nextPos.y;
                                continue;
                            }
                        }
                    }

                    if (isHorizontalCollision) {
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
            return;
        }
    }
}

void Player::draw() const {
    // Draw dust particles
    for (auto& p : dustParticles) {
        float alpha = 1.0f - (p.age / p.lifetime);
        Color c = { 200, 200, 180, (unsigned char)(alpha * 180) };
        DrawCircleV(p.position, 4, c);
    }

    // Draw player rectangle with flash effect when hit
    Color playerColor = GREEN;
    if (invincibilityTimer > 0.0f && (int)(invincibilityTimer * 10) % 2 == 0) {
        playerColor = RED;
    }
    DrawRectangle((int)position.x, (int)position.y, width, height, playerColor);
    
    // Draw current weapon
    if (weapons.size() > 0 && currentWeaponIndex < weapons.size()) {
        weapons[currentWeaponIndex]->draw(position, facingRight);
    }
    
    // Draw facing direction indicator
    Color dirColor = BLUE;
    if (facingRight) {
        DrawTriangle(
            (Vector2){position.x + width, position.y + height/2},
            (Vector2){position.x + width - 8, position.y + height/2 - 8},
            (Vector2){position.x + width - 8, position.y + height/2 + 8},
            dirColor
        );
    } else {
        // Make left triangle more visible with different color and slightly larger
        // Adjusted coordinates to draw the triangle outside the player's left edge
        DrawTriangle(
            (Vector2){position.x, position.y + height/2}, // Tip of the triangle at player's left edge
            (Vector2){position.x - 12, position.y + height/2 - 10}, // Top point of the base, 12 pixels to the left
            (Vector2){position.x - 12, position.y + height/2 + 10}, // Bottom point of the base, 12 pixels to the left
            RED  // Use a different color to make it stand out
        );
    }
}

Vector2 Player::getPosition() const {
    return position;
}

void Player::attack() {
    if (weapons.size() > 0 && currentWeaponIndex < weapons.size()) {
        weapons[currentWeaponIndex]->startAttack();
    }
}

bool Player::isAttacking() const {
    if (weapons.size() > 0 && currentWeaponIndex < weapons.size()) {
        return weapons[currentWeaponIndex]->isAttacking();
    }
    return false;
}

Rectangle Player::getWeaponHitbox() const {
    if (weapons.size() > 0 && currentWeaponIndex < weapons.size()) {
        return weapons[currentWeaponIndex]->getHitbox(position, facingRight);
    }
    return Rectangle{0, 0, 0, 0};
}

void Player::switchWeapon(int index) {
    if (index >= 0 && index < weapons.size()) {
        currentWeaponIndex = index;
    }
}

void Player::addWeapon(std::unique_ptr<Weapon> weapon) {
    weapons.push_back(std::move(weapon));
}

void Player::checkWeaponHits(std::vector<ScrapHound>& enemies) {
    if (!isAttacking()) return;
    
    // Check for special case of bow
    if (weapons[currentWeaponIndex]->getType() == WeaponType::BOW) {
        dynamic_cast<Bow*>(weapons[currentWeaponIndex].get())->checkArrowCollisions(enemies);
        return;
    }
    
    // Regular melee weapon collision check
    Rectangle hitbox = getWeaponHitbox();
    
    for (auto& enemy : enemies) {
        if (!enemy.isAlive()) continue;
        
        Vector2 enemyPos = enemy.getPosition();
        Rectangle enemyRect = {
            enemyPos.x - 16.0f,
            enemyPos.y - 16.0f,
            32.0f,
            32.0f
        };
        
        if (CheckCollisionRecs(hitbox, enemyRect)) {
            float damage = weapons[currentWeaponIndex]->getDamage();
            enemy.takeDamage(damage);
            
            // Apply knockback based on weapon type
            float knockbackForce = 100.0f;
            if (weapons[currentWeaponIndex]->getType() == WeaponType::SWORD) {
                knockbackForce = 150.0f;
            } else if (weapons[currentWeaponIndex]->getType() == WeaponType::SPEAR) {
                knockbackForce = 200.0f;
            }
            
            Vector2 knockbackDir = {
                facingRight ? 1.0f : -1.0f,
                -0.5f
            };
            Vector2 knockback = {
                knockbackDir.x * knockbackForce,
                knockbackDir.y * knockbackForce
            };
            enemy.applyKnockback(knockback);
        }
    }
}

bool Player::isSwordAttacking() const {
    return isAttacking();
}

Rectangle Player::getSwordHitbox() const {
    return getWeaponHitbox();
}

bool Player::canTakeDamage() const {
    return invincibilityTimer <= 0.0f;
}

void Player::takeDamage(int amount) {
    if (canTakeDamage()) {
        health -= amount;
        invincibilityTimer = 1.0f;
        
        // Apply knockback when damaged
        velocity.y = -300.0f;
        velocity.x = (facingRight ? -200.0f : 200.0f);
        
        // Check if player died
        if (health <= 0) {
            health = 0;
            // Handle player death (respawn, game over, etc.)
        }
    }
}