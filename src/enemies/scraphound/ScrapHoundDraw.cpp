#include "enemies/ScrapHound.hpp"
#include <raylib.h>


void ScrapHound::draw() const {
    float barWidth = 32.0f;
    float barHeight = 4.0f;
    float healthRatio = health / maxHealth;
    int barX = (int)position.x;
    int barY = (int)position.y - 8;
    DrawRectangle(barX, barY, (int)barWidth, (int)barHeight, DARKGRAY);
    DrawRectangle(barX, barY, (int)(barWidth * healthRatio), (int)barHeight, LIME);
    Color displayColor = currentColor;
    if (isMeleeAttacking) {
        displayColor = hitEffectTimer > 0.0f ? currentColor : GREEN;
    } else if (isPouncing) {
        displayColor = hitEffectTimer > 0.0f ? currentColor : ORANGE;
    } else if (isMeleeCharging) {
        displayColor = hitEffectTimer > 0.0f ? currentColor : YELLOW;
    } else if (isPounceCharging) {
        displayColor = hitEffectTimer > 0.0f ? currentColor : PURPLE;
    } else {
        displayColor = hitEffectTimer > 0.0f ? currentColor : RED;
    }
    DrawRectangle((int)position.x, (int)position.y, 32, 32, displayColor);
    if (pounceAnimFade > 0.0f) {
        DrawCircleLines((int)position.x + 16, (int)position.y + 16, pounceAnimRadius, Fade(BLUE, pounceAnimFade));
    }
}
