#include "weapons/WeaponTypes.hpp"
#include "enemies/ScrapHound.hpp"
#include <algorithm>
#include <cmath>

// Sword implementation
Sword::Sword()
    : Weapon("Sword", WeaponType::SWORD, 25.0f, 1.5f, 40.0f) {
    texture = LoadTexture("../resources/weapons/sword.png");
}

void Sword::update(float dt) {
    Weapon::update(dt);
    
    if (attacking) {
        frameTime += dt;
        if (frameTime >= 0.05f) {  // Animation speed
            frameTime = 0;
            currentFrame = (currentFrame + 1) % 6;
        }
    } else {
        currentFrame = 0;
        frameTime = 0;
    }
}

void Sword::draw(Vector2 playerPosition, bool facingRight) const {
    if (!attacking) return;
    
    float attackProgress = 1.0f - (attackTimer * attackSpeed);
    
    Rectangle source = { currentFrame * 64.0f, comboCount * 64.0f, 64.0f, 64.0f };
    Rectangle dest = {
        playerPosition.x + (facingRight ? 16.0f : -64.0f),
        playerPosition.y - 16.0f,
        64.0f,
        64.0f
    };
    
    float rotation = facingRight ? 0.0f : 180.0f;
    Vector2 origin = { 0.0f, 32.0f };
    
    // Apply swing animation
    if (attackProgress < 0.5f) {
        rotation += facingRight ? -45.0f * (attackProgress * 2) : 45.0f * (attackProgress * 2);
    } else {
        rotation += facingRight ? -45.0f + 90.0f * ((attackProgress - 0.5f) * 2) : 
                                  45.0f - 90.0f * ((attackProgress - 0.5f) * 2);
    }
    
    DrawTexturePro(texture, source, dest, origin, rotation, WHITE);
}

Rectangle Sword::getHitbox(Vector2 playerPosition, bool facingRight) const {
    if (!attacking) {
        return Rectangle{0, 0, 0, 0};
    }
    
    float hitboxWidth = range;
    float hitboxHeight = 32.0f;
    
    if (comboCount == 1) {
        hitboxHeight = 40.0f; // Upper swing
    } else if (comboCount == 2) {
        hitboxWidth = range * 1.2f; // Wider swing
    }
    
    return Rectangle{
        playerPosition.x + (facingRight ? 16.0f : -16.0f - hitboxWidth),
        playerPosition.y - 8.0f,
        hitboxWidth,
        hitboxHeight
    };
}

void Sword::startAttack() {
    Weapon::startAttack();
    // Add sword-specific attack code here if needed
}

// Dagger implementation
Dagger::Dagger()
    : Weapon("Dagger", WeaponType::DAGGER, 15.0f, 3.0f, 25.0f) {
    texture = LoadTexture("../resources/weapons/dagger.png");
}

void Dagger::update(float dt) {
    Weapon::update(dt);
    
    if (attacking) {
        frameTime += dt;
        if (frameTime >= 0.03f) {  // Faster animation for daggers
            frameTime = 0;
            currentFrame = (currentFrame + 1) % 4;
        }
    } else {
        currentFrame = 0;
        frameTime = 0;
    }
}

void Dagger::draw(Vector2 playerPosition, bool facingRight) const {
    if (!attacking) return;
    
    float attackProgress = 1.0f - (attackTimer * attackSpeed);
    
    Rectangle source = { currentFrame * 32.0f, 0.0f, 32.0f, 32.0f };
    Rectangle dest = {
        playerPosition.x + (facingRight ? 16.0f : -32.0f),
        playerPosition.y,
        32.0f,
        32.0f
    };
    
    float rotation = facingRight ? 0.0f : 180.0f;
    Vector2 origin = { 0.0f, 16.0f };
    
    // Thrust animation
    if (attackProgress < 0.3f) {
        dest.x += facingRight ? attackProgress * 30.0f : -attackProgress * 30.0f;
    } else if (attackProgress < 0.7f) {
        dest.x += facingRight ? 9.0f - (attackProgress - 0.3f) * 15.0f : 
                               -9.0f + (attackProgress - 0.3f) * 15.0f;
    } else {
        dest.x += facingRight ? (attackProgress - 0.7f) * 15.0f : 
                               -(attackProgress - 0.7f) * 15.0f;
    }
    
    DrawTexturePro(texture, source, dest, origin, rotation, WHITE);
}

Rectangle Dagger::getHitbox(Vector2 playerPosition, bool facingRight) const {
    if (!attacking) {
        return Rectangle{0, 0, 0, 0};
    }
    
    float attackProgress = 1.0f - (attackTimer * attackSpeed);
    float hitboxWidth = range;
    float hitboxHeight = 20.0f;
    float offsetX = facingRight ? 20.0f : -20.0f - hitboxWidth;
    
    // Adjust hitbox based on thrust animation
    if (attackProgress < 0.3f) {
        offsetX += facingRight ? attackProgress * 30.0f : -attackProgress * 30.0f;
    }
    
    return Rectangle{
        playerPosition.x + offsetX,
        playerPosition.y,
        hitboxWidth,
        hitboxHeight
    };
}

void Dagger::startAttack() {
    Weapon::startAttack();
    // Apply dagger's speed multiplier
    attackTimer = 1.0f / (attackSpeed * attackSpeedMultiplier);
}

// Spear implementation
Spear::Spear()
    : Weapon("Spear", WeaponType::SPEAR, 20.0f, 1.2f, 60.0f) {
    texture = LoadTexture("../resources/weapons/spear.png");
}

void Spear::update(float dt) {
    Weapon::update(dt);
    
    if (attacking) {
        frameTime += dt;
        if (frameTime >= 0.06f) {
            frameTime = 0;
            currentFrame = (currentFrame + 1) % 5;
        }
    } else {
        currentFrame = 0;
        frameTime = 0;
    }
}

void Spear::draw(Vector2 playerPosition, bool facingRight) const {
    if (!attacking) return;
    
    float attackProgress = 1.0f - (attackTimer * attackSpeed);
    
    Rectangle source = { currentFrame * 80.0f, 0.0f, 80.0f, 20.0f };
    Rectangle dest = {
        playerPosition.x + (facingRight ? 16.0f : -96.0f),
        playerPosition.y + 10.0f,
        80.0f,
        20.0f
    };
    
    float rotation = facingRight ? 0.0f : 180.0f;
    Vector2 origin = { 0.0f, 10.0f };
    
    // Thrust animation
    if (attackProgress < 0.4f) {
        dest.x += facingRight ? attackProgress * 40.0f : -attackProgress * 40.0f;
    } else {
        dest.x += facingRight ? 16.0f - (attackProgress - 0.4f) * 26.7f : 
                               -16.0f + (attackProgress - 0.4f) * 26.7f;
    }
    
    DrawTexturePro(texture, source, dest, origin, rotation, WHITE);
}

Rectangle Spear::getHitbox(Vector2 playerPosition, bool facingRight) const {
    if (!attacking) {
        return Rectangle{0, 0, 0, 0};
    }
    
    float attackProgress = 1.0f - (attackTimer * attackSpeed);
    float hitboxWidth = range * extraRange; // Apply extra range multiplier
    float hitboxHeight = 12.0f;
    float offsetX = facingRight ? 20.0f : -20.0f - hitboxWidth;
    
    // Adjust hitbox based on thrust animation
    if (attackProgress < 0.4f) {
        offsetX += facingRight ? attackProgress * 40.0f : -attackProgress * 40.0f;
    }
    
    return Rectangle{
        playerPosition.x + offsetX,
        playerPosition.y + 10.0f,
        hitboxWidth,
        hitboxHeight
    };
}

void Spear::startAttack() {
    Weapon::startAttack();
    // Spear-specific attack code here if needed
}

// Bow implementation
Bow::Bow()
    : Weapon("Bow", WeaponType::BOW, 30.0f, 0.8f, 400.0f) {
    texture = LoadTexture("../resources/weapons/bow.png");
    arrowTexture = LoadTexture("../resources/weapons/arrow.png");
    
    // Initialize default camera
    defaultCamera.zoom = 1.0f;
}

void Bow::update(float dt) {
    Weapon::update(dt);
    
    if (charging) {
        chargeTime += dt;
        if (chargeTime >= maxChargeTime) {
            chargeTime = maxChargeTime;
        }
        
        // Fire arrow when mouse button released
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            Vector2 mousePos = GetMousePosition();
            
            // If we can access the current camera, use it, otherwise use default
            Camera2D camera = { 0 };
            camera.zoom = 1.0f; // Default zoom
            
            // Simple screen-to-world conversion if GetWorldToScreen is unavailable
            Vector2 direction = {
                mousePos.x - position.x,
                mousePos.y - position.y
            };
            
            // Normalize direction vector
            float magnitude = sqrtf(direction.x * direction.x + direction.y * direction.y);
            if (magnitude > 0) {
                direction.x /= magnitude;
                direction.y /= magnitude;
            }
            
            fireArrow(position, direction);
            charging = false;
            chargeTime = 0.0f;
        }
    }
    
    // Update existing arrows
    updateArrows(dt);
}

void Bow::draw(Vector2 playerPosition, bool facingRight) const {
    // Store position for arrow firing
    position = playerPosition;
    
    // Draw bow
    if (charging) {
        // Draw bow being pulled back
        float chargeRatio = chargeTime / maxChargeTime;
        
        Rectangle source = { 0.0f, 0.0f, 48.0f, 48.0f };
        Rectangle dest = {
            playerPosition.x + (facingRight ? 16.0f : -16.0f),
            playerPosition.y,
            48.0f,
            48.0f
        };
        
        float rotation = 0.0f;
        
        // Aim bow at mouse
        Vector2 mousePos = GetMousePosition();
        
        // Simple direction calculation (screen space)
        rotation = atan2f(mousePos.y - playerPosition.y, mousePos.x - playerPosition.x) * RAD2DEG;
        if (!facingRight) {
            rotation += 180.0f;
        }
        
        Vector2 origin = { 16.0f, 24.0f };
        
        // Draw bow with pull-back animation
        DrawTexturePro(texture, source, dest, origin, rotation, WHITE);
        
        // Draw charge indicator
        DrawCircleV(playerPosition, 20.0f * chargeRatio, ColorAlpha(RED, 0.3f));
    }
    
    // Draw active arrows
    drawArrows();
}

void Bow::startAttack() {
    if (!charging && !attacking) {
        charging = true;
        chargeTime = 0.0f;
    }
}

Rectangle Bow::getHitbox(Vector2 playerPosition, bool facingRight) const {
    // Bow itself doesn't have a hitbox, arrows do
    return Rectangle{0, 0, 0, 0};
}

void Bow::fireArrow(Vector2 position, Vector2 direction) {
    float chargeRatio = chargeTime / maxChargeTime;
    float arrowSpeed = 300.0f + 400.0f * chargeRatio;
    
    Arrow arrow;
    arrow.position = position;
    arrow.direction = direction;
    arrow.speed = arrowSpeed;
    arrow.lifetime = 3.0f;
    arrow.active = true;
    
    activeArrows.push_back(arrow);
}

void Bow::updateArrows(float dt) {
    for (auto& arrow : activeArrows) {
        if (!arrow.active) continue;
        
        // Move arrow
        arrow.position.x += arrow.direction.x * arrow.speed * dt;
        arrow.position.y += arrow.direction.y * arrow.speed * dt;
        
        // Update lifetime
        arrow.lifetime -= dt;
        if (arrow.lifetime <= 0.0f) {
            arrow.active = false;
        }
    }
    
    // Remove inactive arrows
    activeArrows.erase(
        std::remove_if(activeArrows.begin(), activeArrows.end(),
            [](const Arrow& a) { return !a.active; }),
        activeArrows.end());
}

void Bow::drawArrows() const {
    for (const auto& arrow : activeArrows) {
        if (!arrow.active) continue;
        
        float rotation = atan2f(arrow.direction.y, arrow.direction.x) * RAD2DEG;
        
        Rectangle source = { 0.0f, 0.0f, 32.0f, 8.0f };
        Rectangle dest = {
            arrow.position.x,
            arrow.position.y,
            32.0f,
            8.0f
        };
        
        Vector2 origin = { 0.0f, 4.0f };
        
        DrawTexturePro(arrowTexture, source, dest, origin, rotation, WHITE);
    }
}

void Bow::checkArrowCollisions(std::vector<ScrapHound>& enemies) {
    for (auto& arrow : activeArrows) {
        if (!arrow.active) continue;
        
        Rectangle arrowHitbox = {
            arrow.position.x,
            arrow.position.y - 4.0f,
            32.0f, 
            8.0f
        };
        
        for (auto& enemy : enemies) {
            if (!enemy.isAlive()) continue;
            
            Vector2 enemyPos = enemy.getPosition();
            Rectangle enemyRect = {
                enemyPos.x - 16.0f,
                enemyPos.y - 16.0f,
                32.0f,
                32.0f
            };
            
            if (CheckCollisionRecs(arrowHitbox, enemyRect)) {
                // Apply damage with potential bonus for fully charged shots
                float chargeFactor = arrow.speed > 500.0f ? 1.5f : 1.0f;
                enemy.takeDamage(getDamage() * chargeFactor);
                
                // Apply knockback
                Vector2 knockback = {
                    arrow.direction.x * 150.0f,
                    arrow.direction.y * 150.0f
                };
                enemy.applyKnockback(knockback);
                
                // Deactivate arrow
                arrow.active = false;
                break;
            }
        }
    }
}