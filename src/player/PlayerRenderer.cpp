#include "Player.hpp"
#include <raylib.h>

void Player::draw() const {
    // Particles are now handled by the dedicated ParticleSystem
    
    if (textureLoadedAtomic.load(std::memory_order_acquire)) { 
        Color tint = WHITE;
        if (invincibilityTimer > 0.0f && (int)(invincibilityTimer * 10) % 2 == 0) {
            tint = RED;
        }
        
        float visualX = position.x - hitboxOffsetX;
        float visualY = position.y - hitboxOffsetY;
        
        Rectangle destRec = { 
            visualX, 
            visualY, 
            width,
            height
        };
        
        Rectangle sourceRec = { 
            0, 0, 
            static_cast<float>(facingRight ? texture.width : -texture.width), 
            static_cast<float>(texture.height) 
        };
        
        DrawTexturePro(texture, sourceRec, destRec, (Vector2){0, 0}, 0.0f, tint);
    } else {
        float visualX = position.x - hitboxOffsetX;
        float visualY = position.y - hitboxOffsetY;
        DrawRectangle((int)visualX, (int)visualY, width, height, GREEN);
    }
    
    
    Color dirColor = BLUE;
    float visualX = position.x - hitboxOffsetX;
    float visualY = position.y - hitboxOffsetY;
    
    if (facingRight) {
        DrawTriangle(
            (Vector2){visualX + width, visualY + height/2},
            (Vector2){visualX + width - 8, visualY + height/2 - 8},
            (Vector2){visualX + width - 8, visualY + height/2 + 8},
            dirColor
        );
    } else {
        DrawTriangle(
            (Vector2){visualX, visualY + height/2},
            (Vector2){visualX - 12, visualY + height/2 - 10},
            (Vector2){visualX - 12, visualY + height/2 + 10},
            RED
        );
    }
    
   
    if (weapons.size() > 0 && currentWeaponIndex < weapons.size()) {
        weapons[currentWeaponIndex]->draw(position, facingRight);
    }
}