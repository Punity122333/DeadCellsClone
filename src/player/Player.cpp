#include "Player.hpp"
#include "enemies/ScrapHound.hpp"
#include "raymath.h"
#include <algorithm>
#include <raylib.h>
#include "weapons/WeaponTypes.hpp"
#include <future>
#include <thread>

#include <chrono> 

Image LoadImageAsync(const std::string& path) {
    Image image = LoadImage(path.c_str());
    if (image.data == nullptr) {
    } else {
    }
    return image;
}

Player::Player(const Map &map) {
    // Find a treasure room to spawn in first, then fall back to other rooms
    const auto& rooms = map.getGeneratedRooms();
    if (!rooms.empty()) {
        // Look for a treasure room first
        const Room* spawnRoom = nullptr;
        for (const auto& room : rooms) {
            if (room.type == Room::TREASURE) {
                spawnRoom = &room;
                break;
            }
        }
        // If no treasure room found, use first room
        if (spawnRoom == nullptr) {
            spawnRoom = &rooms[0];
        }
        
        int spawnX = (spawnRoom->startX + spawnRoom->endX) / 2;
        int spawnY = (spawnRoom->startY + spawnRoom->endY) / 2;
        position = { spawnX * 32.0f, spawnY * 32.0f };
        printf("DEBUG: Player spawning in room type %d at tile coords (%d,%d), pixel coords (%.1f,%.1f)\n", 
               spawnRoom->type, spawnX, spawnY, position.x, position.y);
    } else {
        position = map.findEmptySpawn();
    }
    velocity = {0, 0};
    health = 100.0f;
    maxHealth = 100.0f;
    invincibilityTimer = 0.0f;
    facingRight = true;
    weapons.push_back(std::make_unique<Sword>());
    weapons.push_back(std::make_unique<Bow>());
    currentWeaponIndex = 1;

    imageFuture = std::async(std::launch::async, LoadImageAsync, "../resources/image.png");

    width = 48;
    height = 60;
    hitboxOffsetX = width * 0.1f;
    hitboxOffsetY = height * 0.05f;
    hitboxWidth = width * 0.7f;
    hitboxHeight = height * 0.9f;
}

void Player::update(float dt, const Map& map, const Camera2D& gameCamera, std::vector<ScrapHound>& enemies) {
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
    checkWeaponHits(enemies);
}

void Player::updateParticles(float dt) {
    size_t numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 2; 
    
    if (dustParticles.empty()) {
        return;
    }

    size_t chunkSize = (dustParticles.size() + numThreads - 1) / numThreads;
    std::vector<std::future<void>> futures;
    futures.reserve(numThreads); 

    for (size_t t = 0; t < numThreads; ++t) {
        size_t start = t * chunkSize;
        size_t end = std::min(start + chunkSize, dustParticles.size());

        if (start >= end) {
            continue; 
        }

        futures.push_back(std::async(std::launch::async, [&, start, end, dt]() { 
            for (size_t i = start; i < end; ++i) {
                if (i < dustParticles.size()) { 
                    auto& p = dustParticles[i]; 
                    p.position = Vector2Add(p.position, Vector2Scale(p.velocity, dt));
                    p.velocity.y += 600 * dt; 
                    p.age += dt;
                }
            }
        }));
    }

    for (auto& f : futures) {
        if (f.valid()) { 
            f.get();
        }
    }

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
    if (imageFuture.valid()) {
        imageFuture.wait(); 
        if (!imageFutureRetrieved.load(std::memory_order_acquire)) {
            Image loadedImage = imageFuture.get(); 
            if (loadedImage.data != nullptr) {
                UnloadImage(loadedImage); 
            }
        }
    } else {
    }

    if (textureLoadedAtomic.load(std::memory_order_acquire)) {
        if (texture.id > 0) { 
            UnloadTexture(texture);
        } else {
        }
    } else {
    }
}