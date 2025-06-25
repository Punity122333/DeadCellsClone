#include "enemies/Automaton.hpp"
#include <raylib.h>

void Automaton::draw() const {
    if (!alive) return;
    float barWidth = 32.0f;
    float barHeight = 4.0f;
    float healthRatio = (float)health / (float)maxHealth;
    int barX = (int)position.x;
    int barY = (int)position.y - 8;
    DrawRectangle(barX, barY, (int)barWidth, (int)barHeight, DARKGRAY);
    DrawRectangle(barX, barY, (int)(barWidth * healthRatio), (int)barHeight, LIME);
    DrawRectangle((int)position.x, (int)position.y, 32, 32, currentColor);
    for (const auto& proj : projectiles) {
        if (proj.active) proj.draw();
    }
}

void AutomatonProjectile::draw() const {
    if (!active) return;
    DrawCircle((int)position.x + 8, (int)position.y + 8, 8, RED);
}

Rectangle AutomatonProjectile::getHitbox() const {
    if (!active) return Rectangle{0, 0, 0, 0};
    return Rectangle{position.x, position.y, 16, 16};
}
