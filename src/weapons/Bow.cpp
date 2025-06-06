#include "weapons/WeaponTypes.hpp"
#include "enemies/ScrapHound.hpp"
#include <algorithm>
#include <cmath>
#include <cfloat>

Bow::Bow()
    : Weapon("Bow", WeaponType::BOW, 30.0f, 0.8f, 400.0f) {
    texture = LoadTexture("../resources/weapons/bow.png");
    arrowTexture = LoadTexture("../resources/weapons/arrow.png");
}

void Bow::startAttack() {
    if (!charging) {
        Weapon::startAttack(); 
        charging = true;
        chargeTime = 0.0f;
    }
}

void Bow::update(float dt, const Camera2D& gameCamera, bool playerFacingRight) {
    Weapon::update(dt, gameCamera, playerFacingRight); 
    if (charging) {
        chargeTime += dt;
        if (chargeTime >= maxChargeTime) {
            chargeTime = maxChargeTime;
        }
        if ((IsMouseButtonReleased(MOUSE_LEFT_BUTTON) || IsKeyReleased(KEY_J)) && chargeTime >= minChargeTime) {
            Vector2 fireDirection = { (playerFacingRight ? 1.0f : -1.0f), 0.0f };
            fireArrow(position, fireDirection); 
            charging = false;
            chargeTime = 0.0f;
        }
        else if ((IsMouseButtonReleased(MOUSE_LEFT_BUTTON) || IsKeyReleased(KEY_J)) && chargeTime < minChargeTime) {
            charging = false;
            chargeTime = 0.0f;
        }
    }
    updateArrows(dt);
}

void Bow::draw(Vector2 playerPosition, bool facingRight) const {
    Rectangle boxRect = {
        playerPosition.x + (facingRight ? 16.0f : -32.0f),
        playerPosition.y + 8.0f,
        24.0f,
        24.0f
    };
    DrawRectangleRec(boxRect, SKYBLUE);
    DrawRectangleLinesEx(boxRect, 1.0f, DARKBLUE);
    drawArrows();
}

Rectangle Bow::getHitbox(Vector2 playerPosition, bool facingRight) const {
    return Rectangle{0, 0, 0, 0};
}

void Bow::fireArrow(Vector2 startPosition, Vector2 direction) {
    float chargeRatio = chargeTime / maxChargeTime;
    float arrowSpeed = 300.0f + 400.0f * chargeRatio;
    Arrow arrow;
    arrow.position = { startPosition.x, startPosition.y + 28.0f };
    arrow.direction = direction;
    arrow.speed = arrowSpeed;
    arrow.lifetime = 3.0f;
    arrow.active = true;
    activeArrows.push_back(arrow);
}

void Bow::updateArrows(float dt) {
    for (auto& arrow : activeArrows) {
        if (!arrow.active) continue;
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

void Bow::updatePosition(Vector2 newPosition) {
    position = newPosition;
}

void Bow::drawArrows() const {
    for (const auto& arrow : activeArrows) {
        if (!arrow.active) continue;
        Rectangle arrowRect = {
            arrow.position.x,
            arrow.position.y - 4.0f,
            32.0f,
            8.0f
        };
        DrawRectangleRec(arrowRect, GREEN);
        DrawRectangleLinesEx(arrowRect, 1.0f, DARKGREEN);
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
                enemyPos.x,
                enemyPos.y,
                32.0f,
                32.0f
            };
            if (CheckCollisionRecs(arrowHitbox, enemyRect)) {
                float chargeFactor = arrow.speed > 500.0f ? 1.5f : 1.0f;
                enemy.takeDamage(getDamage() * chargeFactor);
                Vector2 knockback = {
                    arrow.direction.x * 150.0f,
                    arrow.direction.y * 150.0f
                };
                enemy.applyKnockback(knockback);
                arrow.active = false;
                break;
            }
        }
    }
}

bool Bow::isCharging() const {
    return charging;
}

