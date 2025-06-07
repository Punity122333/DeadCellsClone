#include "weapons/WeaponTypes.hpp"
#include "enemies/ScrapHound.hpp"

#include <cmath>
#include <cfloat>
#include <algorithm> 
#include <raylib.h> 

Bow::Bow()
    : Weapon("Bow", WeaponType::BOW, 30.0f, 0.8f, 400.0f),
      minChargeTime(0.2f), 
      maxChargeTime(0.8f)  
{
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
        TraceLog(LOG_INFO, "Bow::update - Accumulated chargeTime: %f", chargeTime);

        if (chargeTime >= maxChargeTime) {
            chargeTime = maxChargeTime;
            TraceLog(LOG_INFO, "Bow::update - chargeTime capped at max: %f", chargeTime);
        }

        
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) || IsKeyReleased(KEY_J)) {
            TraceLog(LOG_INFO, "Bow::update - Attack button released. chargeTime before firing check: %f", chargeTime);
            
            if (chargeTime >= minChargeTime) {
                TraceLog(LOG_INFO, "Bow::update - Firing arrow. Effective chargeTime: %f", chargeTime);
                Vector2 fireDirection = { (playerFacingRight ? 1.0f : -1.0f), 0.0f };
                fireArrow(position, fireDirection);
            }
            
            charging = false;
            chargeTime = 0.0f;
            TraceLog(LOG_INFO, "Bow::update - Stopped charging. chargeTime reset to: %f", chargeTime);
        }
    }
    
    
    
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

    
    if (charging) {
        float meterWidth = 40.0f;
        float meterHeight = 8.0f;
        float progress = chargeTime / maxChargeTime;
        float filledWidth = meterWidth * progress;
        float meterX = playerPosition.x - meterWidth / 2.0f + 12.0f;
        float meterY = playerPosition.y - 18.0f;
        
        DrawRectangle(meterX, meterY, meterWidth, meterHeight, Fade(GRAY, 0.5f));
        
        DrawRectangle(meterX, meterY, filledWidth, meterHeight, Fade(GREEN, 0.8f));
        
        DrawRectangleLines(meterX, meterY, meterWidth, meterHeight, DARKGREEN);
    }
}

Rectangle Bow::getHitbox(Vector2 playerPosition, bool facingRight) const {
    return Rectangle{0, 0, 0, 0};
}

void Bow::fireArrow(Vector2 startPosition, Vector2 direction) {
    TraceLog(LOG_INFO, "Bow::fireArrow - Entry. Member chargeTime: %f", this->chargeTime); 
    
    float chargeRatio = 0.0f;
    
    if (maxChargeTime > 0.0f) {
        
        chargeRatio = this->chargeTime / maxChargeTime; 
    }
    
    
    
    chargeRatio = fmaxf(0.0f, fminf(chargeRatio, 1.0f)); 
    TraceLog(LOG_INFO, "Bow::fireArrow - Calculated chargeRatio: %f", chargeRatio);
    
    const float MIN_ARROW_SPEED = 300.0f; 
    const float MAX_THEORETICAL_ARROW_SPEED = 700.0f; 
    const float EFFECTIVE_SPEED_CAP = 696.0f; 

    
    float calculatedSpeed = MIN_ARROW_SPEED + (MAX_THEORETICAL_ARROW_SPEED - MIN_ARROW_SPEED) * chargeRatio;
    
    
    float arrowSpeed = fminf(calculatedSpeed, EFFECTIVE_SPEED_CAP);
    TraceLog(LOG_INFO, "Bow::fireArrow - Final arrowSpeed: %f", arrowSpeed);
    
    Arrow arrow;
    arrow.position = { startPosition.x, startPosition.y + 28.0f };
    arrow.prevPosition = arrow.position;  
    arrow.direction = direction;
    arrow.speed = arrowSpeed; 
    arrow.lifetime = 3.0f;
    arrow.active = true;
    activeArrows.push_back(arrow);
}


void Bow::updateArrows(float dt) { 
    for (auto& arrow : activeArrows) {
        if (!arrow.active) continue; 

        TraceLog(LOG_INFO, "Bow::updateArrows - Arrow Addr: %p, Speed: %f, PosX: %f, dt_param: %f",
                   (void*)&arrow, arrow.speed, arrow.position.x, dt);

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

void Bow::checkArrowCollisions(std::vector<ScrapHound>& enemies) {
    for (auto& arrow : activeArrows) {
        if (!arrow.active) continue;
        
        
        Rectangle arrowHitbox;
        float minX = fminf(arrow.prevPosition.x, arrow.position.x);
        float maxX = fmaxf(arrow.prevPosition.x, arrow.position.x);
        
        
        if (arrow.direction.x >= 0) {
            arrowHitbox = {
                minX,                    
                arrow.position.y - 6.0f, 
                maxX - minX + 32.0f,     
                12.0f                    
            };
        } 
        
        else {
            arrowHitbox = {
                minX - 32.0f,           
                arrow.position.y - 6.0f, 
                maxX - minX + 32.0f,       
                12.0f                    
            };
        }
        
        for (auto& enemy : enemies) {
            if (!enemy.isAlive()) continue;
            Rectangle enemyRect = enemy.getArrowHitbox();
            
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


void Bow::updateArrowsWithSubsteps(float dt, std::vector<ScrapHound>& enemies, int substeps_param) {
    int local_substeps = substeps_param;

    for (auto& arrow : activeArrows) {
        if (arrow.active && arrow.speed > 600.0f) {
            local_substeps = std::max(local_substeps, 4); 
            break;
        }
    }
    
    TraceLog(LOG_INFO, "Bow::updateArrowsWithSubsteps - Received dt: %f, substeps_param: %d, calculated local_substeps: %d", 
               dt, substeps_param, local_substeps);

    float subDt = dt / local_substeps;
    for (int i = 0; i < local_substeps; ++i) {
        
        
        updateArrows(subDt);
        
        checkArrowCollisions(enemies);
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
            arrow.position.y - 4.0f,
            32.0f,
            8.0f
        };
        DrawRectangleRec(arrowRect, GREEN);
        DrawRectangleLinesEx(arrowRect, 1.0f, DARKGREEN);
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

