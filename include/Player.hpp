#pragma once
#include "map/Map.hpp"
#include "weapons/Weapon.hpp"  // Include complete Weapon definition instead of forward declaration
#include <raylib.h>
#include <vector>
#include <memory>

class ScrapHound;

struct Particle {
    Vector2 position;
    Vector2 velocity;
    float age;
    float lifetime;
};

class Player {
public:
    Player(const Map &map);
    void update(float dt, const Map& map);
    void draw() const;
    
    Vector2 getPosition() const;
    float getHealth() const { return health; }
    float getMaxHealth() const { return maxHealth; }
    
    // Combat system methods
    void attack();
    bool isAttacking() const;
    Rectangle getWeaponHitbox() const;
    
    // Weapon system methods
    void switchWeapon(int index);
    void addWeapon(std::unique_ptr<Weapon> weapon);
    void checkWeaponHits(std::vector<ScrapHound>& enemies);
    
    // Legacy methods for compatibility
    bool isSwordAttacking() const;
    Rectangle getSwordHitbox() const;
    
    bool canTakeDamage() const;
    void takeDamage(int amount);

private:   
    float health = 100.0f;
    float maxHealth = 100.0f;
    float invincibilityTimer = 0.0f;
    Vector2 position;
    bool ledgeGrabbed = false;
    Vector2 ledgeGrabPos = {0};
    std::vector<Particle> dustParticles;
    Vector2 velocity;
    float dropTimer = 0.0f;
    const float DROP_TIME = 0.5f;
    const float LADDER_JUMP_LEEWAY = 10.0f;
    bool dropDown;
    
    // Weapon system
    std::vector<std::unique_ptr<Weapon>> weapons;
    int currentWeaponIndex = 0;
    
    // Movement and physics
    bool touchingWallLeft;
    bool touchingWallRight;
    bool canWallJump;
    bool onLadder;
    bool canLadderJump;
    float width = 32;
    float height = 60;
    float speed = 450;
    bool onGround = false;
    bool onRope = false;
    float coyoteTimer = 0.0f;
    static constexpr float COYOTE_TIME = 0.1f;
    bool facingRight;
    
    // Remove these as they're replaced by the weapon system
    // bool isAttacking;
    // float attackTimer;
    // float attackDuration;
    // float attackCooldown;
    // float attackCooldownTimer;
    // int attackComboCount;
    // Rectangle swordHitbox;
    // Vector2 swordDirection;
    
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
};