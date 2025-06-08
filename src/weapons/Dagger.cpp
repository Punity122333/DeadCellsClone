#include "weapons/WeaponTypes.hpp"
#include <cmath>

namespace {    
    constexpr float DAGGER_KNOCKBACK_X = 75.0f;
    constexpr float DAGGER_KNOCKBACK_Y = -25.0f;

    constexpr const char* DAGGER_TEXTURE_PATH = "../resources/weapons/dagger.png";
    constexpr float DAGGER_ANIM_FRAME_DURATION = 0.03f;
    constexpr int DAGGER_ANIM_NUM_FRAMES = 4;

    constexpr float DAGGER_SPRITE_FRAME_WIDTH = 32.0f;
    constexpr float DAGGER_SPRITE_FRAME_HEIGHT = 32.0f;
    constexpr float DAGGER_DRAW_OFFSET_X_RIGHT = 16.0f;
    constexpr float DAGGER_DRAW_OFFSET_X_LEFT = -32.0f;
    constexpr float DAGGER_DRAW_OFFSET_Y = 0.0f;
    constexpr float DAGGER_ROTATION_FACING_RIGHT = 0.0f;
    constexpr float DAGGER_ROTATION_FACING_LEFT = 180.0f;
    constexpr float DAGGER_ORIGIN_X = 0.0f;
    constexpr float DAGGER_ORIGIN_Y = 16.0f;

    constexpr float DAGGER_ATTACK_PROGRESS_THRESHOLD_1 = 0.3f;
    constexpr float DAGGER_ATTACK_PROGRESS_THRESHOLD_2 = 0.7f;
    constexpr float DAGGER_DRAW_ADJUST_X_1 = 30.0f;
    constexpr float DAGGER_DRAW_ADJUST_X_2_BASE = 9.0f;
    constexpr float DAGGER_DRAW_ADJUST_X_2_FACTOR = 15.0f;
    constexpr float DAGGER_DRAW_ADJUST_X_3 = 15.0f;

    constexpr float DAGGER_HITBOX_HEIGHT = 20.0f;
    constexpr float DAGGER_HITBOX_OFFSET_X_RIGHT = 20.0f;
    constexpr float DAGGER_HITBOX_OFFSET_X_LEFT_FACTOR = -20.0f;
    constexpr float DAGGER_HITBOX_ADJUST_X = 30.0f;
}

Dagger::Dagger()
    : Weapon("Dagger", WeaponType::DAGGER, 15.0f, 3.0f, 25.0f) {
    texture = LoadTexture(DAGGER_TEXTURE_PATH);
}

void Dagger::update(float dt, const Camera2D& gameCamera, bool playerFacingRight) {
    Weapon::update(dt, gameCamera, playerFacingRight);
    if (attacking) {
        frameTime += dt;
        if (frameTime >= DAGGER_ANIM_FRAME_DURATION) {
            frameTime = 0;
            currentFrame = (currentFrame + 1) % DAGGER_ANIM_NUM_FRAMES;
        }
    } else {
        currentFrame = 0;
        frameTime = 0;
    }
}

void Dagger::draw(Vector2 playerPosition, bool facingRight) const {
    if (!attacking) return;
    float attackProgress = 1.0f - (attackTimer * attackSpeed);
    Rectangle source = { currentFrame * DAGGER_SPRITE_FRAME_WIDTH, 0.0f, DAGGER_SPRITE_FRAME_WIDTH, DAGGER_SPRITE_FRAME_HEIGHT };
    Rectangle dest = {
        playerPosition.x + (facingRight ? DAGGER_DRAW_OFFSET_X_RIGHT : DAGGER_DRAW_OFFSET_X_LEFT),
        playerPosition.y + DAGGER_DRAW_OFFSET_Y,
        DAGGER_SPRITE_FRAME_WIDTH,
        DAGGER_SPRITE_FRAME_HEIGHT
    };
    float rotation = facingRight ? DAGGER_ROTATION_FACING_RIGHT : DAGGER_ROTATION_FACING_LEFT;
    Vector2 origin = { DAGGER_ORIGIN_X, DAGGER_ORIGIN_Y };
    if (attackProgress < DAGGER_ATTACK_PROGRESS_THRESHOLD_1) {
        dest.x += facingRight ? attackProgress * DAGGER_DRAW_ADJUST_X_1 : -attackProgress * DAGGER_DRAW_ADJUST_X_1;
    } else if (attackProgress < DAGGER_ATTACK_PROGRESS_THRESHOLD_2) {
        dest.x += facingRight ? DAGGER_DRAW_ADJUST_X_2_BASE - (attackProgress - DAGGER_ATTACK_PROGRESS_THRESHOLD_1) * DAGGER_DRAW_ADJUST_X_2_FACTOR : 
                               -DAGGER_DRAW_ADJUST_X_2_BASE + (attackProgress - DAGGER_ATTACK_PROGRESS_THRESHOLD_1) * DAGGER_DRAW_ADJUST_X_2_FACTOR;
    } else {
        dest.x += facingRight ? (attackProgress - DAGGER_ATTACK_PROGRESS_THRESHOLD_2) * DAGGER_DRAW_ADJUST_X_3 : 
                               -(attackProgress - DAGGER_ATTACK_PROGRESS_THRESHOLD_2) * DAGGER_DRAW_ADJUST_X_3;
    }
    DrawTexturePro(texture, source, dest, origin, rotation, WHITE);
}

Rectangle Dagger::getHitbox(Vector2 playerPosition, bool facingRight) const {
    if (!attacking) {
        return Rectangle{0, 0, 0, 0};
    }
    float attackProgress = 1.0f - (attackTimer * attackSpeed);
    float hitboxWidth = range;
    float hitboxHeight = DAGGER_HITBOX_HEIGHT;
    float offsetX = facingRight ? DAGGER_HITBOX_OFFSET_X_RIGHT : DAGGER_HITBOX_OFFSET_X_LEFT_FACTOR - hitboxWidth;
    if (attackProgress < DAGGER_ATTACK_PROGRESS_THRESHOLD_1) {
        offsetX += facingRight ? attackProgress * DAGGER_HITBOX_ADJUST_X : -attackProgress * DAGGER_HITBOX_ADJUST_X;
    }
    return Rectangle{
        playerPosition.x + offsetX,
        playerPosition.y + DAGGER_DRAW_OFFSET_Y, // Assuming same Y offset for hitbox as draw
        hitboxWidth,
        hitboxHeight
    };
}

void Dagger::startAttack() {
    Weapon::startAttack();
    attackTimer = 1.0f / (attackSpeed * attackSpeedMultiplier);
}

Vector2 Dagger::getKnockback(bool facingRight) const {
    return {facingRight ? DAGGER_KNOCKBACK_X : -DAGGER_KNOCKBACK_X, DAGGER_KNOCKBACK_Y};
}