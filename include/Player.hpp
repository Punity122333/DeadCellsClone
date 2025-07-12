#pragma once
#include "map/Map.hpp"
#include "weapons/Weapon.hpp"  
#include "core/InputManager.hpp"
#include <raylib.h>
#include <vector>
#include <memory>
#include <future>   
#include <atomic>  

class ScrapHound;
class Automaton;
class Detonode;
class EnemyManager;



class Player {
public:
    Player(const Map &map);
    ~Player();
    void cleanup();
    void update(float dt, const Map& map, const Camera2D& gameCamera, EnemyManager& enemyManager, Core::InputManager& inputManager); 
    void draw() const;
    
    Vector2 getPosition() const;
    float getHealth() const { return health; }
    float getMaxHealth() const { return maxHealth; }
    
    
    void attack();
    bool isAttacking() const;
    Rectangle getWeaponHitbox() const;
    
    
    void switchWeapon(int index);
    void addWeapon(std::unique_ptr<Weapon> weapon);
    void checkWeaponHits(EnemyManager& enemyManager);
    
    
    bool isSwordAttacking() const;
    Rectangle getSwordHitbox() const;
    
    bool canTakeDamage() const;
    void takeDamage(int amount);
    void applyKnockback(Vector2 force);

    

private:   
    
    float hitboxOffsetX = 0;    
    float hitboxOffsetY = 0;   
    float hitboxWidth = 0;      
    float hitboxHeight = 0;    
    
    
    float health = 100.0f;
    float maxHealth = 100.0f;
    float invincibilityTimer = 0.0f;
    float lavaDamageTimer = 0.0f;
    Vector2 position;
    bool ledgeGrabbed = false;
    Vector2 ledgeGrabPos = {0};
    Vector2 velocity;
    float dropTimer = 0.0f;
    const float DROP_TIME = 0.5f;
    const float LADDER_JUMP_LEEWAY = 10.0f;
    bool dropDown;
    
    
    std::vector<std::unique_ptr<Weapon>> weapons;
    int currentWeaponIndex = 0;
    
    
    bool touchingWallLeft;
    bool touchingWallRight;
    bool canWallJump;
    bool onLadder;
    bool canLadderJump;
    float width = 80;
    float height = 100;
    float speed = 450;
    bool onGround = false;
    bool onRope = false;
    float coyoteTimer = 0.0f;
    static constexpr float COYOTE_TIME = 0.1f;
    bool facingRight;
    Texture2D texture;
    std::atomic<bool> textureLoadedAtomic{false};
    
    std::future<Image> imageFuture;
    std::atomic<bool> imageFutureRetrieved{false};

    Rectangle frameRec;
    
    int frameWidth;      
    int frameHeight;     
    int currentFrame;    
    int framesCounter;   
    int framesSpeed;
    void applyGravity(float dt, Core::InputManager& inputManager);
    void updateLadderState(const Map& map);
    void updateWallState(const Map& map);
    void updateLedgeGrab(const Map& map, Core::InputManager& inputManager);
    void handleMovementInput(float dt, Core::InputManager& inputManager);
    void handleDropThrough(const Map& map, Core::InputManager& inputManager);
    void updateDropTimer(float dt);
    Vector2 computeNextPosition(float dt);
    void handleCollisions(Vector2& nextPos, const Map& map, float dt);
    void updateCoyoteTimer(float dt);
    void handleJumpInput(const Map& map, float dt, Core::InputManager& inputManager);
    void handleLedgeGrabInput(Core::InputManager& inputManager);
};