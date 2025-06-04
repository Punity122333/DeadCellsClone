#include "Player.hpp"
#include "enemies/ScrapHound.hpp"
#include "raymath.h"
#include <algorithm>
#include <raylib.h>
#include "weapons/WeaponTypes.hpp"

Player::Player(const Map &map) {
    position = map.findEmptySpawn();
    velocity = {0, 0};
    
    health = 100.0f;
    maxHealth = 100.0f;
    invincibilityTimer = 0.0f;
    facingRight = true;
    
    weapons.push_back(std::make_unique<Sword>());
    currentWeaponIndex = 0;

    textureLoaded = false;
    texture = LoadTexture("../resources/playersheet.png");
    if (texture.id == 0) {
        textureLoaded = false;
        TraceLog(LOG_ERROR, "Failed to load player texture");
    } else {
        textureLoaded = true;
        
        frameWidth = 46;
        frameHeight = 46;
        
        currentFrame = 0;
        framesCounter = 0;
        framesSpeed = 8;
        
        frameRec = { 0.0f, 0.0f, (float)frameWidth, (float)frameHeight };
    }
    width = 46;
    height = 46;
    hitboxOffsetX = width * 0.25f;
    hitboxOffsetY = height * 0.1f;
    hitboxWidth = width * 0.5f;
    hitboxHeight = height * 0.8f;
}

void Player::update(float dt, const Map& map) {
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
    
    if (weapons.size() > 0 && currentWeaponIndex < weapons.size()) {
        weapons[currentWeaponIndex]->update(dt);
    }
    
    if (IsKeyPressed(KEY_J) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        attack();
    }
    
    for (int i = 0; i < 9; i++) {
        if (IsKeyPressed(KEY_ONE + i) && i < weapons.size()) {
            switchWeapon(i);
        }
    }
    
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

    if (textureLoaded) {
        framesCounter++;
        
        if (framesCounter >= (60/framesSpeed)) {
            framesCounter = 0;
            
            int row = 0;
            
            if (!onGround) {
                row = 1;
            } else if (fabs(velocity.x) > 10.0f) {
                row = 2;
                currentFrame = (currentFrame + 1) % 6;
            } else {
                row = 0;
                currentFrame = (currentFrame + 1) % 4;
            }
            
            frameRec.x = (float)(currentFrame * frameWidth);
            frameRec.y = (float)(row * frameHeight);
        }
    }
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
                // Use full width for visual positioning
                climbEndPos.x = handX * 32.0f - width;
            } else {
                climbEndPos.x = (handX + 1) * 32.0f;
            }
            // Use full height for visual positioning
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

void Player::handleCollisions(Vector2& nextPos, const Map& map, float dt) {
    Rectangle playerHitbox = { nextPos.x + hitboxOffsetX, nextPos.y + hitboxOffsetY, hitboxWidth, hitboxHeight };
    onGround = false;

    int tileXStart = (int)(playerHitbox.x / 32) - 1;
    int tileYStart = (int)(playerHitbox.y / 32) - 1;
    int tileXEnd = tileXStart + 3;
    int tileYEnd = tileYStart + 4;

    if (dropTimer <= 0.0f) {
        for (int y = tileYStart; y <= tileYEnd; y++) {
            for (int x = tileXStart; x <= tileXEnd; x++) {
                if (!map.isSolidTile(x, y)) continue;

                bool climbingUp = onLadder && (velocity.y < 0);
                bool headInsideTile = (playerHitbox.y <= y * 32.0f + 31) && (playerHitbox.y > y * 32.0f - hitboxHeight + 1);
                if (climbingUp && headInsideTile) continue;

                bool isBelowTile = (y * 32.0f >= playerHitbox.y + playerHitbox.height) &&
                                   (x == (int)((playerHitbox.x + playerHitbox.width / 2) / 32));
                if (dropTimer > 0.0f && isBelowTile) continue;

                Rectangle tileRect = { x * 32.0f, y * 32.0f, 32.0f, 32.0f };
                if (CheckCollisionRecs(playerHitbox, tileRect)) {
                    Rectangle intersection = GetCollisionRec(playerHitbox, tileRect);

                    bool isHorizontalCollision = intersection.width < intersection.height;
                    bool isMovingTowardsTile = (velocity.x > 0 && playerHitbox.x < tileRect.x) ||
                                               (velocity.x < 0 && playerHitbox.x > tileRect.x);
                    bool isNearGroundLevelOfTile = (playerHitbox.y + playerHitbox.height >= tileRect.y) &&
                                                   (playerHitbox.y + playerHitbox.height <= tileRect.y + 32 + 5); 

                    if (isHorizontalCollision && isMovingTowardsTile && isNearGroundLevelOfTile && onGround) {
                        if (!map.isSolidTile(x, y - 1)) {
                            int targetXTile = (int)((playerHitbox.x + playerHitbox.width / 2) / 32); 
                            if (!map.isSolidTile(targetXTile, y - 1)) { 
                                nextPos.y = tileRect.y - hitboxHeight - hitboxOffsetY;
                                if (velocity.x > 0) {
                                    nextPos.x = tileRect.x - hitboxWidth - hitboxOffsetX;
                                } else {
                                    nextPos.x = tileRect.x + 32 - hitboxOffsetX;
                                }
                                velocity.y = 0;
                                onGround = true;
                                playerHitbox.x = nextPos.x + hitboxOffsetX;
                                playerHitbox.y = nextPos.y + hitboxOffsetY;
                                continue;
                            }
                        }
                    }

                    if (isHorizontalCollision) {
                        if (playerHitbox.x < tileRect.x)
                            nextPos.x -= intersection.width;
                        else
                            nextPos.x += intersection.width;
                        velocity.x = 0;
                    } else { 
                        if (playerHitbox.y < tileRect.y) {
                            nextPos.y -= intersection.height;
                            onGround = true;
                        } else {
                            nextPos.y += intersection.height;
                        }
                        velocity.y = 0;
                    }

                    playerHitbox.x = nextPos.x + hitboxOffsetX;
                    playerHitbox.y = nextPos.y + hitboxOffsetY;
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
    for (auto& p : dustParticles) {
        float alpha = 1.0f - (p.age / p.lifetime);
        Color c = { 200, 200, 180, (unsigned char)(alpha * 180) };
        DrawCircleV(p.position, 4, c);
    }

    if (textureLoaded) {
        Color tint = WHITE;
        if (invincibilityTimer > 0.0f && (int)(invincibilityTimer * 10) % 2 == 0) {
            tint = RED;
        }
        
        Rectangle destRec = { position.x, position.y, width, height };
        
        Rectangle sourceRec = frameRec;
        if (!facingRight) {
            sourceRec.width = -sourceRec.width;
        }
        
        DrawTexturePro(texture, sourceRec, destRec, (Vector2){0, 0}, 0.0f, tint);
    } else {
        Color playerColor = GREEN;
        if (invincibilityTimer > 0.0f && (int)(invincibilityTimer * 10) % 2 == 0) {
            playerColor = RED;
        }
        DrawRectangle((int)position.x, (int)position.y, width, height, playerColor);
    }
    
    Color dirColor = BLUE;
    if (facingRight) {
        DrawTriangle(
            (Vector2){position.x + width, position.y + height/2},
            (Vector2){position.x + width - 8, position.y + height/2 - 8},
            (Vector2){position.x + width - 8, position.y + height/2 + 8},
            dirColor
        );
    } else {
        DrawTriangle(
            (Vector2){position.x, position.y + height/2},
            (Vector2){position.x - 12, position.y + height/2 - 10},
            (Vector2){position.x - 12, position.y + height/2 + 10},
            RED
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
    
    if (weapons[currentWeaponIndex]->getType() == WeaponType::BOW) {
        dynamic_cast<Bow*>(weapons[currentWeaponIndex].get())->checkArrowCollisions(enemies);
        return;
    }
    
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
        
        velocity.y = -300.0f;
        velocity.x = (facingRight ? -200.0f : 200.0f);
        
        if (health <= 0) {
            health = 0;
        }
    }
}

Player::~Player() {
    if (textureLoaded) {
        UnloadTexture(texture);
    }
}