#include "weapons/WeaponTypes.hpp"
#include "enemies/ScrapHound.hpp"
#include "enemies/Automaton.hpp"

#include <cmath>
#include <cfloat>
#include <algorithm> 
#include <raylib.h>
#include <algorithm>
#include <cmath>

namespace {
    const char* BOW_NAME = "Bow";
    constexpr float BOW_DAMAGE = 30.0f;
    constexpr float BOW_ATTACK_SPEED = 0.8f; 
    constexpr float BOW_RANGE = 400.0f; 
    constexpr float BOW_MIN_CHARGE_TIME = 0.2f;
    constexpr float BOW_MAX_CHARGE_TIME = 0.8f;
    constexpr float BOW_DEFAULT_CHARGE_TIME = 0.0f; 
    const char* BOW_TEXTURE_PATH = "../resources/weapons/bow.png";
    const char* ARROW_TEXTURE_PATH = "../resources/weapons/arrow.png";

    constexpr float BOW_FIRE_DIRECTION_FORWARD = 1.0f;
    constexpr float BOW_FIRE_DIRECTION_BACKWARD = -1.0f;
    constexpr float BOW_FIRE_DIRECTION_NEUTRAL_Y = 0.0f;

    constexpr float BOW_KNOCKBACK_X = 20.0f;
    constexpr float BOW_KNOCKBACK_Y = -10.0f;
    
    constexpr float ARROW_LIFETIME = 3.0f;
    constexpr float ARROW_START_Y_OFFSET = 28.0f;
    constexpr float MIN_ARROW_SPEED = 300.0f;
    constexpr float MAX_THEORETICAL_ARROW_SPEED = 700.0f;
    constexpr float EFFECTIVE_SPEED_CAP = 696.0f;

    constexpr float ARROW_KNOCKBACK_BASE_X = 150.0f;
    constexpr float ARROW_KNOCKBACK_BASE_Y = 150.0f; 
    constexpr float ARROW_CHARGE_FACTOR_SPEED_THRESHOLD = 500.0f;
    constexpr float ARROW_HIGH_CHARGE_FACTOR = 1.5f;
    constexpr float ARROW_LOW_CHARGE_FACTOR = 1.0f;

    constexpr float ARROW_HITBOX_Y_OFFSET = -6.0f;
    constexpr float ARROW_HITBOX_WIDTH_ADJUSTMENT = 32.0f;
    constexpr float ARROW_HITBOX_HEIGHT = 12.0f;
    
    constexpr float BOW_DRAW_RECT_OFFSET_X_FACING_RIGHT = 16.0f;
    constexpr float BOW_DRAW_RECT_OFFSET_X_FACING_LEFT = -16.0f;
    constexpr float BOW_DRAW_RECT_OFFSET_Y = 8.0f;
    constexpr float BOW_DRAW_RECT_WIDTH = 24.0f;
    constexpr float BOW_DRAW_RECT_HEIGHT = 24.0f;
    constexpr float BOW_DRAW_LINE_THICKNESS = 1.0f;

    constexpr float CHARGE_METER_WIDTH = 40.0f;
    constexpr float CHARGE_METER_HEIGHT = 8.0f;
    constexpr float CHARGE_METER_X_OFFSET = 12.0f;
    constexpr float CHARGE_METER_Y_OFFSET = -18.0f;
    constexpr float CHARGE_METER_BG_ALPHA = 0.5f;
    constexpr float CHARGE_METER_FG_ALPHA = 0.8f;

    constexpr float ARROW_DRAW_Y_OFFSET = -4.0f;
    constexpr float ARROW_DRAW_WIDTH = 32.0f;
    constexpr float ARROW_DRAW_HEIGHT = 8.0f;
    constexpr float ARROW_DRAW_LINE_THICKNESS = 1.0f;

    constexpr float BOW_HITBOX_X = 0.0f;
    constexpr float BOW_HITBOX_Y = 0.0f;
    constexpr float BOW_HITBOX_WIDTH = 0.0f;
    constexpr float BOW_HITBOX_HEIGHT = 0.0f;

    constexpr float SUBSTEP_SPEED_THRESHOLD = 600.0f;
    constexpr int SUBSTEP_MAX_COUNT = 4;
}

Bow::Bow()
    : Weapon(BOW_NAME, WeaponType::BOW, BOW_DAMAGE, BOW_ATTACK_SPEED, BOW_RANGE),
      minChargeTime(BOW_MIN_CHARGE_TIME), 
      maxChargeTime(BOW_MAX_CHARGE_TIME)  
{
    texture = LoadTexture(BOW_TEXTURE_PATH);
    arrowTexture = LoadTexture(ARROW_TEXTURE_PATH);
    chargeTime = BOW_DEFAULT_CHARGE_TIME;
}

Bow::~Bow() {
    if (arrowTexture.id > 0) {
        UnloadTexture(arrowTexture);
    }
}

void Bow::startAttack() {
    if (!charging) {
        Weapon::startAttack(); 
        charging = true;
        chargeTime = BOW_DEFAULT_CHARGE_TIME;
    }
}

void Bow::update(float dt, const Camera2D& gameCamera, bool playerFacingRight) {
    Weapon::update(dt, gameCamera, playerFacingRight); 

    if (charging) {
        chargeTime += dt;
        

        if (chargeTime >= maxChargeTime) {
            chargeTime = maxChargeTime;
            
        }
        
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) || IsKeyReleased(KEY_J)) {
            
            
            if (chargeTime >= minChargeTime) {
                
                Vector2 fireDirection = { (playerFacingRight ? BOW_FIRE_DIRECTION_FORWARD : BOW_FIRE_DIRECTION_BACKWARD), BOW_FIRE_DIRECTION_NEUTRAL_Y };
                fireArrow(position, fireDirection);
            }
            
            charging = false;
            chargeTime = BOW_DEFAULT_CHARGE_TIME;
            
        }
    }
}

void Bow::draw(Vector2 playerPosition, bool facingRight) const {
    Rectangle boxRect = {
        playerPosition.x + (facingRight ? BOW_DRAW_RECT_OFFSET_X_FACING_RIGHT : BOW_DRAW_RECT_OFFSET_X_FACING_LEFT),
        playerPosition.y + BOW_DRAW_RECT_OFFSET_Y,
        BOW_DRAW_RECT_WIDTH,
        BOW_DRAW_RECT_HEIGHT
    };
    DrawRectangleRec(boxRect, SKYBLUE);
    DrawRectangleLinesEx(boxRect, BOW_DRAW_LINE_THICKNESS, DARKBLUE);
    drawArrows();
    if (charging) {
        float meterWidth = CHARGE_METER_WIDTH;
        float meterHeight = CHARGE_METER_HEIGHT;
        float progress = chargeTime / maxChargeTime;
        float filledWidth = meterWidth * progress;
        float meterX = playerPosition.x - meterWidth / 2.0f + CHARGE_METER_X_OFFSET;
        float meterY = playerPosition.y + CHARGE_METER_Y_OFFSET;
        DrawRectangle(meterX, meterY, meterWidth, meterHeight, Fade(GRAY, CHARGE_METER_BG_ALPHA));
        DrawRectangle(meterX, meterY, filledWidth, meterHeight, Fade(GREEN, CHARGE_METER_FG_ALPHA));
        DrawRectangleLines(meterX, meterY, meterWidth, meterHeight, DARKGREEN);
    }
}

Rectangle Bow::getHitbox(Vector2 playerPosition, bool facingRight) const {
    return Rectangle{BOW_HITBOX_X, BOW_HITBOX_Y, BOW_HITBOX_WIDTH, BOW_HITBOX_HEIGHT};
}

Vector2 Bow::getKnockback(bool facingRight) const {
    return {facingRight ? BOW_KNOCKBACK_X : -BOW_KNOCKBACK_X, BOW_KNOCKBACK_Y};
}

void Bow::fireArrow(Vector2 startPosition, Vector2 direction) {
    float chargeRatio = 0.0f;
    if (maxChargeTime > 0.0f) {
        chargeRatio = this->chargeTime / maxChargeTime; 
    }
    chargeRatio = fmaxf(0.0f, fminf(chargeRatio, 1.0f)); 
    float calculatedSpeed = MIN_ARROW_SPEED + (MAX_THEORETICAL_ARROW_SPEED - MIN_ARROW_SPEED) * chargeRatio;
    float arrowSpeed = fminf(calculatedSpeed, EFFECTIVE_SPEED_CAP);
    Arrow arrow;
    arrow.position = { startPosition.x, startPosition.y + ARROW_START_Y_OFFSET };
    arrow.prevPosition = arrow.position;  
    arrow.direction = direction;
    arrow.speed = arrowSpeed; 
    arrow.lifetime = ARROW_LIFETIME;
    arrow.active = true;
    activeArrows.push_back(arrow);
}

void Bow::updateArrows(float dt) { 
    for (auto& arrow : activeArrows) {
        if (!arrow.active) continue; 

        

        arrow.prevPosition = arrow.position;  
        arrow.position.x += arrow.direction.x * arrow.speed * dt;
        arrow.position.y += arrow.direction.y * arrow.speed * dt;
        arrow.lifetime -= dt;
        if (arrow.lifetime <= 0.0f) {
            arrow.active = false; 
        }
    }
    
    activeArrows.erase(
        std::remove_if(activeArrows.begin(), activeArrows.end(),
                       [](const Arrow& a) { return !a.active; }),
        activeArrows.end());
}

void Bow::checkArrowCollisions(std::vector<ScrapHound>& enemies, std::vector<Automaton>& automatons) {
    for (auto& arrow : activeArrows) {
        if (!arrow.active) continue;
        for (auto& enemy : enemies) {
            if (enemy.isAlive() && CheckCollisionRecs(arrow.getHitbox(), enemy.getArrowHitbox())) {
                enemy.takeDamage(BOW_DAMAGE);
                arrow.active = false;
            }
        }
        for (auto& automaton : automatons) {
            if (automaton.isAlive() && CheckCollisionRecs(arrow.getHitbox(), automaton.getHitbox())) {
                automaton.takeDamage(BOW_DAMAGE);
                arrow.active = false;
            }
        }
    }
}

void Bow::updateArrowsWithSubsteps(float dt, std::vector<ScrapHound>& enemies, std::vector<Automaton>& automatons, int substeps) {
    for (int s = 0; s < substeps; ++s) {
        for (auto& arrow : activeArrows) {
            if (!arrow.active) continue; 

        

            arrow.prevPosition = arrow.position;  
            arrow.position.x += arrow.direction.x * arrow.speed * dt;
            arrow.position.y += arrow.direction.y * arrow.speed * dt;
            arrow.lifetime -= dt;
            if (arrow.lifetime <= 0.0f) {
                arrow.active = false; 
            }
        }
        checkArrowCollisions(enemies, automatons);
    }
    
    activeArrows.erase(
        std::remove_if(activeArrows.begin(), activeArrows.end(),
                       [](const Arrow& a) { return !a.active; }),
        activeArrows.end());
}

void Bow::drawArrows() const {
    for (const auto& arrow : activeArrows) {
        if (!arrow.active) continue;
        Rectangle arrowRect = {
            arrow.position.x,
            arrow.position.y + ARROW_DRAW_Y_OFFSET,
            ARROW_DRAW_WIDTH,
            ARROW_DRAW_HEIGHT
        };
        DrawRectangleRec(arrowRect, GREEN);
        DrawRectangleLinesEx(arrowRect, ARROW_DRAW_LINE_THICKNESS, DARKGREEN);
    }
}

bool Bow::isCharging() const {
    return charging;
}

void Bow::updatePosition(Vector2 newPosition) {
    position = newPosition;
}

bool Bow::hasActiveArrows() const {
    return !activeArrows.empty();
}

Rectangle Bow::Arrow::getHitbox() const {
    return { position.x, position.y, 16, 8 };
}

