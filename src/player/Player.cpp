#include "Player.hpp"
#include "enemies/ScrapHound.hpp"
#include "raymath.h"
#include <algorithm>
#include <raylib.h>
#include "weapons/WeaponTypes.hpp"
#include <future>
#include <thread>

Player::Player(const Map &map) {
    position = map.findEmptySpawn();
    velocity = {0, 0};
    health = 100.0f;
    maxHealth = 100.0f;
    invincibilityTimer = 0.0f;
    facingRight = true;
    weapons.push_back(std::make_unique<Sword>());
    weapons.push_back(std::make_unique<Bow>());
    currentWeaponIndex = 1;
    textureLoaded = false;
    texture = LoadTexture("../resources/image.png");
    if (texture.id == 0) {
        textureLoaded = false;
        TraceLog(LOG_ERROR, "Failed to load player texture");
    } else {
        textureLoaded = true;
    }
    width = 48;
    height = 60;
    hitboxOffsetX = width * 0.1f;
    hitboxOffsetY = height * 0.05f;
    hitboxWidth = width * 0.7f;
    hitboxHeight = height * 0.9f;
}

void Player::update(float dt, const Map& map, const Camera2D& gameCamera, std::vector<ScrapHound>& enemies) {
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
        weapons[currentWeaponIndex]->update(dt, gameCamera, facingRight);
    }
    if (weapons.size() > 0 && currentWeaponIndex < weapons.size()) {
        Weapon* currentWeapon = weapons[currentWeaponIndex].get(); 
        if (currentWeapon->getType() == WeaponType::BOW) {
            Bow* bow = dynamic_cast<Bow*>(currentWeapon);
            if (bow) { 
                if ((IsKeyPressed(KEY_J) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) && !bow->isCharging()) {
                    attack();
                }
                bow->updatePosition(position);

                if (bow->hasActiveArrows()) { 
                    int default_substeps = 1; 
                    bow->updateArrowsWithSubsteps(dt, enemies, default_substeps); 
                }
            }
        } else {
            if (IsKeyPressed(KEY_J) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                attack();
            }
        }
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
    updateParticles(dt);
}

void Player::updateParticles(float dt) {
    size_t numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 2;
    size_t chunkSize = (dustParticles.size() + numThreads - 1) / numThreads;
    std::vector<std::future<void>> futures;
    for (size_t t = 0; t < numThreads; ++t) {
        size_t start = t * chunkSize;
        size_t end = std::min(start + chunkSize, dustParticles.size());
        futures.push_back(std::async(std::launch::async, [&, start, end]() {
            for (size_t i = start; i < end; ++i) {
                auto& p = dustParticles[i];
                p.position = Vector2Add(p.position, Vector2Scale(p.velocity, dt));
                p.velocity.y += 600 * dt;
                p.age += dt;
            }
        }));
    }
    for (auto& f : futures) f.get();
    dustParticles.erase(
        std::remove_if(dustParticles.begin(), dustParticles.end(),
            [](const Particle& p) { return p.age > p.lifetime; }),
        dustParticles.end()
    );
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
    if (textureLoaded) {
        UnloadTexture(texture);
    }
}
