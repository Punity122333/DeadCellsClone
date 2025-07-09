#include "enemies/Detonode.hpp"
#include <raylib.h>
#include <cmath>

void Detonode::draw() const {
    if (!alive) return;

    Vector2 renderPos = position;
    renderPos.y += sin(bobOffset) * bobAmplitude;

    Color drawColor = currentColor;
    
    if (currentState == BLINKING) {
        float blinkAlpha = 0.5f + 0.5f * sin(blinkTimer * 10.0f);
        drawColor = ColorAlpha(RED, blinkAlpha);
    }

    DrawCircleV(renderPos, 16.0f, drawColor);
    DrawCircleLines(renderPos.x, renderPos.y, 16.0f, BLACK);
    
    if (currentState == APPROACHING || currentState == BLINKING) {
        DrawCircleLines(renderPos.x, renderPos.y, 20.0f, ColorAlpha(RED, 0.3f));
    }
}
