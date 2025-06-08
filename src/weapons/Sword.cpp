#include "weapons/WeaponTypes.hpp"
#include <cmath>

namespace {
    constexpr float SWORD_KNOCKBACK_X = 150.0f;
    constexpr float SWORD_KNOCKBACK_Y = -100.0f;
    constexpr const char* SWORD_TEXTURE_PATH = "../resources/weapons/sword.png";
    constexpr float SWORD_ANIM_FRAME_DURATION = 0.05f;
    constexpr int SWORD_ANIM_NUM_FRAMES = 6;
    constexpr float SWORD_SPRITE_FRAME_WIDTH = 64.0f;
    constexpr float SWORD_SPRITE_FRAME_HEIGHT = 64.0f;
    constexpr float SWORD_DRAW_OFFSET_X_RIGHT = 16.0f;
    constexpr float SWORD_DRAW_OFFSET_X_LEFT = -64.0f;
    constexpr float SWORD_DRAW_OFFSET_Y = -16.0f;
    constexpr float SWORD_ROTATION_FACING_RIGHT = 0.0f;
    constexpr float SWORD_ROTATION_FACING_LEFT = 180.0f;
    constexpr float SWORD_ORIGIN_X = 0.0f;
    constexpr float SWORD_ORIGIN_Y = 32.0f;
    constexpr float SWORD_ATTACK_PROGRESS_THRESHOLD = 0.5f;
    constexpr float SWORD_ROTATION_ADJUST_INITIAL = -45.0f;
    constexpr float SWORD_ROTATION_ADJUST_LATER = 90.0f;
    constexpr float SWORD_HITBOX_DEFAULT_HEIGHT = 32.0f;
    constexpr float SWORD_HITBOX_COMBO1_HEIGHT = 40.0f;
    constexpr float SWORD_HITBOX_COMBO2_WIDTH_MULTIPLIER = 1.2f;
    constexpr float SWORD_HITBOX_OFFSET_X_RIGHT = 16.0f;
    constexpr float SWORD_HITBOX_OFFSET_X_LEFT_FACTOR = -16.0f;
    constexpr float SWORD_HITBOX_OFFSET_Y = -8.0f;
    constexpr int SWORD_MAX_COMBO = 3;
}

Sword::Sword()
    : Weapon("Sword", WeaponType::SWORD, 25.0f, 1.5f, 40.0f) {
    texture = LoadTexture(SWORD_TEXTURE_PATH);
}

void Sword::update(float dt, const Camera2D& gameCamera, bool playerFacingRight) {
    Weapon::update(dt, gameCamera, playerFacingRight);
    
    if (attacking) {
        frameTime += dt;
        if (frameTime >= SWORD_ANIM_FRAME_DURATION) {  
            frameTime = 0;
            currentFrame = (currentFrame + 1) % SWORD_ANIM_NUM_FRAMES;
        }
    } else {
        currentFrame = 0;
        frameTime = 0;
    }
}

void Sword::draw(Vector2 playerPosition, bool facingRight) const {
    if (!attacking) return;
    
    float attackProgress = 1.0f - (attackTimer * attackSpeed);
    
    Rectangle source = { currentFrame * SWORD_SPRITE_FRAME_WIDTH, comboCount * SWORD_SPRITE_FRAME_HEIGHT, SWORD_SPRITE_FRAME_WIDTH, SWORD_SPRITE_FRAME_HEIGHT };
    Rectangle dest = {
        playerPosition.x + (facingRight ? SWORD_DRAW_OFFSET_X_RIGHT : SWORD_DRAW_OFFSET_X_LEFT),
        playerPosition.y + SWORD_DRAW_OFFSET_Y,
        SWORD_SPRITE_FRAME_WIDTH,
        SWORD_SPRITE_FRAME_HEIGHT
    };
    
    float rotation = facingRight ? SWORD_ROTATION_FACING_RIGHT : SWORD_ROTATION_FACING_LEFT;
    Vector2 origin = { SWORD_ORIGIN_X, SWORD_ORIGIN_Y };
    
    
    if (attackProgress < SWORD_ATTACK_PROGRESS_THRESHOLD) {
        rotation += facingRight ? SWORD_ROTATION_ADJUST_INITIAL * (attackProgress * 2.0f) : -SWORD_ROTATION_ADJUST_INITIAL * (attackProgress * 2.0f);
    } else {
        rotation += facingRight ? SWORD_ROTATION_ADJUST_INITIAL + SWORD_ROTATION_ADJUST_LATER * ((attackProgress - SWORD_ATTACK_PROGRESS_THRESHOLD) * 2.0f) : 
                                 -SWORD_ROTATION_ADJUST_INITIAL - SWORD_ROTATION_ADJUST_LATER * ((attackProgress - SWORD_ATTACK_PROGRESS_THRESHOLD) * 2.0f);
    }
    
    DrawTexturePro(texture, source, dest, origin, rotation, WHITE);
}

Rectangle Sword::getHitbox(Vector2 playerPosition, bool facingRight) const {
    if (!attacking) {
        return Rectangle{0, 0, 0, 0};
    }
    
    float hitboxWidth = range;
    float hitboxHeight = SWORD_HITBOX_DEFAULT_HEIGHT;
    
    if (comboCount == 1) {
        hitboxHeight = SWORD_HITBOX_COMBO1_HEIGHT; 
    } else if (comboCount == 2) {
        hitboxWidth = range * SWORD_HITBOX_COMBO2_WIDTH_MULTIPLIER; 
    }
    
    return Rectangle{
        playerPosition.x + (facingRight ? SWORD_HITBOX_OFFSET_X_RIGHT : SWORD_HITBOX_OFFSET_X_LEFT_FACTOR - hitboxWidth),
        playerPosition.y + SWORD_HITBOX_OFFSET_Y,
        hitboxWidth,
        hitboxHeight
    };
}

void Sword::startAttack() {
    Weapon::startAttack(); 

    this->attacking = true;
    this->attackTimer = 0.0f; 
    this->currentFrame = 0;   
    this->frameTime = 0.0f;   
    this->comboCount = (this->comboCount + 1) % SWORD_MAX_COMBO; 
}

Vector2 Sword::getKnockback(bool facingRight) const {
    return {facingRight ? SWORD_KNOCKBACK_X : -SWORD_KNOCKBACK_X, SWORD_KNOCKBACK_Y};
}