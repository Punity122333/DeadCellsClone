#include "Player.hpp"
#include "enemies/EnemyManager.hpp"
#include "core/GlobalThreadPool.hpp"

#include <raylib.h>
#include <raymath.h>
#include "weapons/WeaponTypes.hpp"
#include <future>

#include <chrono>
#include <cmath> 

Image LoadImageAsync(const std::string& path) {
    Image image = LoadImage(path.c_str());
    if (image.data == nullptr) {
    } else {
    }
    return image;
}

Player::Player(const Map &map) {
    printf("[Player] Starting player initialization...\n");
    
    const auto& rooms = map.getGeneratedRooms();
    printf("[Player] Map has %zu generated rooms\n", rooms.size());
    
    printf("[Player] Calling findEmptySpawn...\n");
    auto start_time = std::chrono::high_resolution_clock::now();
    
    position = map.findEmptySpawn();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    printf("[Player] findEmptySpawn took %ld ms\n", duration.count());
    
    printf("[Player] Player spawn position set to: (%.1f, %.1f)\n", position.x, position.y);
    
    velocity = {0, 0};
    health = 100.0f;
    maxHealth = 100.0f;
    invincibilityTimer = 0.0f;
    facingRight = true;
    weapons.push_back(std::make_unique<Sword>());
    weapons.push_back(std::make_unique<Bow>());
    currentWeaponIndex = 1;

    imageFuture = GlobalThreadPool::getInstance().getMainPool().enqueue([]() {
        return LoadImageAsync("../resources/image.png");
    });

    width = 48;
    height = 60;
    hitboxOffsetX = width * 0.1f;
    hitboxOffsetY = height * 0.05f;
    hitboxWidth = width * 0.7f;
    hitboxHeight = height * 0.9f;
}

void Player::update(float dt, const Map& map, const Camera2D& gameCamera, EnemyManager& enemyManager, Core::InputManager& inputManager) {
    if (!imageFutureRetrieved.load(std::memory_order_acquire)) {
        if (imageFuture.valid()) {
            auto status = imageFuture.wait_for(std::chrono::seconds(0));
            if (status == std::future_status::ready) {
                Image loadedImage = imageFuture.get(); 
                imageFutureRetrieved.store(true, std::memory_order_release); 

                if (loadedImage.data != nullptr) {
                    texture = LoadTextureFromImage(loadedImage); 
                    UnloadImage(loadedImage); 

                    if (texture.id == 0) {
                        textureLoadedAtomic.store(false, std::memory_order_release); 
                    } else {
                        textureLoadedAtomic.store(true, std::memory_order_release);
                    }
                } else {
                    textureLoadedAtomic.store(false, std::memory_order_release); 
                }
            }
        } else {
        }
    }

    if (invincibilityTimer > 0.0f) {
        invincibilityTimer -= dt;
    }
    applyGravity(dt, inputManager);
    updateLadderState(map);
    updateWallState(map);
    updateLedgeGrab(map, inputManager);
    handleMovementInput(dt, inputManager);
    handleDropThrough(map, inputManager);
    updateDropTimer(dt);
    Vector2 nextPos = computeNextPosition(dt);
    handleCollisions(nextPos, map, dt);
    updateCoyoteTimer(dt);
    handleJumpInput(map, dt, inputManager);
    handleLedgeGrabInput(inputManager);
    position = nextPos;

    if (weapons.size() > 0 && currentWeaponIndex < weapons.size()) {
        weapons[currentWeaponIndex]->update(dt, gameCamera, facingRight);

        Weapon* currentWeapon = weapons[currentWeaponIndex].get();
        if (currentWeapon->getType() == WeaponType::BOW) {
            Bow* bow = dynamic_cast<Bow*>(currentWeapon);
            if (bow) {
                if ((inputManager.isActionPressed(Core::InputAction::ATTACK)) && !bow->isCharging()) {
                    attack();
                }
                bow->updatePosition(position);

                if (bow->hasActiveArrows()) {
                    int default_substeps = 1;
                    bow->updateArrowsWithSubsteps(dt, enemyManager, default_substeps, map);
                }
            }
        } else {
            if (inputManager.isActionPressed(Core::InputAction::ATTACK)) {
                attack();
            }
        }
    }

    for (int i = 0; i < 9; i++) {
        if (inputManager.isActionPressed(static_cast<Core::InputAction>(static_cast<int>(Core::InputAction::SWITCH_WEAPON_1) + i)) && i < (int)weapons.size()) {
            switchWeapon(i);
        }
    }
    if (velocity.x > 10.0f || inputManager.isActionHeld(Core::InputAction::MOVE_RIGHT)) {
        facingRight = true;
    } else if (velocity.x < -10.0f || inputManager.isActionHeld(Core::InputAction::MOVE_LEFT)) {
        facingRight = false;
    }
    checkWeaponHits(enemyManager);
}

Vector2 Player::getPosition() const {
    return position;
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
    if (textureLoadedAtomic.load(std::memory_order_acquire) && texture.id > 0) {
        printf("[Player] Warning: Player destroyed with texture still loaded (ID: %d)\n", texture.id);
    }
}

void Player::cleanup() {
    if (imageFuture.valid()) {
        imageFuture.wait(); 
        if (!imageFutureRetrieved.load(std::memory_order_acquire)) {
            Image loadedImage = imageFuture.get(); 
            if (loadedImage.data != nullptr) {
                UnloadImage(loadedImage); 
            }
        }
    }

    if (textureLoadedAtomic.load(std::memory_order_acquire)) {
        if (texture.id > 0) { 
            printf("[Player] Unloading texture with ID: %d\n", texture.id);
            UnloadTexture(texture);
            texture.id = 0;
        }
    }
    textureLoadedAtomic.store(false, std::memory_order_release);
}

void Player::applyKnockback(Vector2 force) {
    // Apply knockback force directly to velocity
    velocity = Vector2Add(velocity, force);

    const float maxKnockbackVelocity = 600.0f;
    if (Vector2Length(velocity) > maxKnockbackVelocity) {
        velocity = Vector2Scale(Vector2Normalize(velocity), maxKnockbackVelocity);
    }
}