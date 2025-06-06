#include "Player.hpp"
#include <raylib.h>

void Player::draw() const {
    for (auto& p : dustParticles) {
        float alpha = 1.0f - (p.age / p.lifetime);
        Color c = { 200, 200, 180, (unsigned char)(alpha * 180) };
        DrawCircleV(p.position, 4, c);
    }

    if (textureLoaded) {
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
    
    // Direction indicator should be based on visual position too
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
    
    // Draw the current weapon
    if (weapons.size() > 0 && currentWeaponIndex < weapons.size()) {
        weapons[currentWeaponIndex]->draw(position, facingRight);
    }
}