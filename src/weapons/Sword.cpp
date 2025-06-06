#include "weapons/WeaponTypes.hpp"
#include <cmath>


Sword::Sword()
    : Weapon("Sword", WeaponType::SWORD, 25.0f, 1.5f, 40.0f) {
    texture = LoadTexture("../resources/weapons/sword.png");
}

void Sword::update(float dt, const Camera2D& gameCamera, bool playerFacingRight) {
    Weapon::update(dt, gameCamera, playerFacingRight);
    
    if (attacking) {
        frameTime += dt;
        if (frameTime >= 0.05f) {  
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
        hitboxHeight = 40.0f; 
    } else if (comboCount == 2) {
        hitboxWidth = range * 1.2f; 
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
    
}