#include "weapons/WeaponTypes.hpp"
#include <cmath>


Spear::Spear()
    : Weapon("Spear", WeaponType::SPEAR, 20.0f, 1.2f, 60.0f) {
    texture = LoadTexture("../resources/weapons/spear.png");
}
void Spear::update(float dt, const Camera2D& gameCamera, bool playerFacingRight) {
    Weapon::update(dt, gameCamera, playerFacingRight);
    
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
    float hitboxWidth = range * extraRange;
    float hitboxHeight = 12.0f;
    float offsetX = facingRight ? 20.0f : -20.0f - hitboxWidth;
    
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
    
}