#include "weapons/WeaponTypes.hpp"
#include <cmath>

namespace {
    
    constexpr float SPEAR_KNOCKBACK_X = 200.0f;
    constexpr float SPEAR_KNOCKBACK_Y = -75.0f;
    constexpr const char* SPEAR_TEXTURE_PATH = "../resources/weapons/spear.png";
    constexpr float SPEAR_ANIM_FRAME_DURATION = 0.06f;
    constexpr int SPEAR_ANIM_NUM_FRAMES = 5;
    constexpr float SPEAR_SPRITE_FRAME_WIDTH = 80.0f;
    constexpr float SPEAR_SPRITE_FRAME_HEIGHT = 20.0f;
    constexpr float SPEAR_DRAW_OFFSET_X_RIGHT = 16.0f;
    constexpr float SPEAR_DRAW_OFFSET_X_LEFT = -96.0f;
    constexpr float SPEAR_DRAW_OFFSET_Y = 10.0f;
    constexpr float SPEAR_ROTATION_FACING_RIGHT = 0.0f;
    constexpr float SPEAR_ROTATION_FACING_LEFT = 180.0f;
    constexpr float SPEAR_ORIGIN_X = 0.0f;
    constexpr float SPEAR_ORIGIN_Y = 10.0f;
    constexpr float SPEAR_ATTACK_PROGRESS_THRESHOLD = 0.4f;
    constexpr float SPEAR_DRAW_ADJUST_X_1 = 40.0f;
    constexpr float SPEAR_DRAW_ADJUST_X_2_BASE = 16.0f;
    constexpr float SPEAR_DRAW_ADJUST_X_2_FACTOR = 26.7f; // This was likely 80/3, kept as is
    constexpr float SPEAR_HITBOX_HEIGHT = 12.0f;
    constexpr float SPEAR_HITBOX_OFFSET_X_RIGHT = 20.0f;
    constexpr float SPEAR_HITBOX_OFFSET_X_LEFT_FACTOR = -20.0f;
    constexpr float SPEAR_HITBOX_ADJUST_X = 40.0f;
    constexpr float SPEAR_HITBOX_OFFSET_Y = 10.0f;
}


Spear::Spear()
    : Weapon("Spear", WeaponType::SPEAR, 20.0f, 1.2f, 60.0f) {
    texture = LoadTexture(SPEAR_TEXTURE_PATH);
}
void Spear::update(float dt, const Camera2D& gameCamera, bool playerFacingRight) {
    Weapon::update(dt, gameCamera, playerFacingRight);
    
    if (attacking) {
        frameTime += dt;
        if (frameTime >= SPEAR_ANIM_FRAME_DURATION) {
            frameTime = 0;
            currentFrame = (currentFrame + 1) % SPEAR_ANIM_NUM_FRAMES;
        }
    } else {
        currentFrame = 0;
        frameTime = 0;
    }
}

void Spear::draw(Vector2 playerPosition, bool facingRight) const {
    if (!attacking) return;
    
    float attackProgress = 1.0f - (attackTimer * attackSpeed);
    
    Rectangle source = { currentFrame * SPEAR_SPRITE_FRAME_WIDTH, 0.0f, SPEAR_SPRITE_FRAME_WIDTH, SPEAR_SPRITE_FRAME_HEIGHT };
    Rectangle dest = {
        playerPosition.x + (facingRight ? SPEAR_DRAW_OFFSET_X_RIGHT : SPEAR_DRAW_OFFSET_X_LEFT),
        playerPosition.y + SPEAR_DRAW_OFFSET_Y,
        SPEAR_SPRITE_FRAME_WIDTH,
        SPEAR_SPRITE_FRAME_HEIGHT
    };
    
    float rotation = facingRight ? SPEAR_ROTATION_FACING_RIGHT : SPEAR_ROTATION_FACING_LEFT;
    Vector2 origin = { SPEAR_ORIGIN_X, SPEAR_ORIGIN_Y };
    
    if (attackProgress < SPEAR_ATTACK_PROGRESS_THRESHOLD) {
        dest.x += facingRight ? attackProgress * SPEAR_DRAW_ADJUST_X_1 : -attackProgress * SPEAR_DRAW_ADJUST_X_1;
    } else {
        dest.x += facingRight ? SPEAR_DRAW_ADJUST_X_2_BASE - (attackProgress - SPEAR_ATTACK_PROGRESS_THRESHOLD) * SPEAR_DRAW_ADJUST_X_2_FACTOR : 
                               -SPEAR_DRAW_ADJUST_X_2_BASE + (attackProgress - SPEAR_ATTACK_PROGRESS_THRESHOLD) * SPEAR_DRAW_ADJUST_X_2_FACTOR;
    }
    
    DrawTexturePro(texture, source, dest, origin, rotation, WHITE);
}

Rectangle Spear::getHitbox(Vector2 playerPosition, bool facingRight) const {
    if (!attacking) {
        return Rectangle{0, 0, 0, 0};
    }
    
    float attackProgress = 1.0f - (attackTimer * attackSpeed);
    float hitboxWidth = range * extraRange;
    float hitboxHeight = SPEAR_HITBOX_HEIGHT;
    float offsetX = facingRight ? SPEAR_HITBOX_OFFSET_X_RIGHT : SPEAR_HITBOX_OFFSET_X_LEFT_FACTOR - hitboxWidth;
    
    if (attackProgress < SPEAR_ATTACK_PROGRESS_THRESHOLD) {
        offsetX += facingRight ? attackProgress * SPEAR_HITBOX_ADJUST_X : -attackProgress * SPEAR_HITBOX_ADJUST_X;
    }
    
    return Rectangle{
        playerPosition.x + offsetX,
        playerPosition.y + SPEAR_HITBOX_OFFSET_Y,
        hitboxWidth,
        hitboxHeight
    };
}

void Spear::startAttack() {
    Weapon::startAttack();
    
}

Vector2 Spear::getKnockback(bool facingRight) const {
    return {facingRight ? SPEAR_KNOCKBACK_X : -SPEAR_KNOCKBACK_X, SPEAR_KNOCKBACK_Y};
}