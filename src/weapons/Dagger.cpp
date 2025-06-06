#include "weapons/WeaponTypes.hpp"
#include <cmath>

Dagger::Dagger()
    : Weapon("Dagger", WeaponType::DAGGER, 15.0f, 3.0f, 25.0f) {
    texture = LoadTexture("../resources/weapons/dagger.png");
}

void Dagger::update(float dt, const Camera2D& gameCamera, bool playerFacingRight) {
    Weapon::update(dt, gameCamera, playerFacingRight);
    if (attacking) {
        frameTime += dt;
        if (frameTime >= 0.03f) {
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
    attackTimer = 1.0f / (attackSpeed * attackSpeedMultiplier);
}