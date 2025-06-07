#include "map/Map.hpp"
#include <cmath>
#include <algorithm> 


namespace {
    int getTileIndex(const std::vector<std::vector<int>>& tiles, int x, int y, int width, int height) {
        bool top    = (y > 0)            && (tiles[x][y - 1] == 1 || tiles[x][y - 1] == 6);
        bool bottom = (y < height - 1)   && (tiles[x][y + 1] == 1 || tiles[x][y + 1] == 6);
        bool left   = (x > 0)            && (tiles[x - 1][y] == 1 || tiles[x - 1][y] == 6);
        bool right  = (x < width - 1)    && (tiles[x + 1][y] == 1 || tiles[x + 1][y] == 6);

        if (!top && !left && !right && !bottom) return 15;
        if (!top && !left && !right)            return 4;
        if (!top && !left && !bottom)           return 5;
        if (!top && !right && !bottom)          return 7;
        if (!left && !right && !bottom)         return 24;
        if (!top && !left)                      return 0;
        if (!top && !right)                     return 3;
        if (!top)                               return 1;
        if (!right && !left)                    return 14;
        if (!right && !bottom)                  return 103;
        if (!left && !bottom)                   return 104;
        if (!right)                             return 13;
        if (!left)                              return 10;
        if (!bottom)                            return 105;
        return 11; 
    }

    float getBlinkAlpha(float timer, float blinkCycle, float minOpacity) {
        float blinkProgress = std::fmod(timer, blinkCycle);
        float alphaNorm = blinkProgress < blinkCycle / 2.0f
            ? blinkProgress / (blinkCycle / 2.0f)
            : 1.0f - ((blinkProgress - blinkCycle / 2.0f) / (blinkCycle / 2.0f));
        alphaNorm = std::clamp(alphaNorm, 0.0f, 1.0f);

        float alpha = alphaNorm < 0.5f
            ? 1.0f - (alphaNorm / 0.5f) * (1.0f - minOpacity)
            : minOpacity + ((alphaNorm - 0.5f) / 0.5f) * (1.0f - minOpacity);
        return std::clamp(alpha, minOpacity, 1.0f);
    }
}

void Map::draw(const Camera2D& camera) const { 
    
    float viewX = camera.target.x - (camera.offset.x / camera.zoom);
    float viewY = camera.target.y - (camera.offset.y / camera.zoom);
    float viewWidth = GetScreenWidth() / camera.zoom;
    float viewHeight = GetScreenHeight() / camera.zoom;
    Rectangle visibleWorldRect = { viewX, viewY, viewWidth, viewHeight };

    for (const auto& chunk : chunks) {
        
        Rectangle chunkRect = {
            (float)chunk.startX * 32.0f,
            (float)chunk.startY * 32.0f,
            (float)(chunk.endX - chunk.startX + 1) * 32.0f,
            (float)(chunk.endY - chunk.startY + 1) * 32.0f
        };

        
        if (CheckCollisionRecs(visibleWorldRect, chunkRect)) {
            for (int x = chunk.startX; x <= chunk.endX; ++x) {
                for (int y = chunk.startY; y <= chunk.endY; ++y) {
                    
                    if (x < 0 || x >= width || y < 0 || y >= height) continue;

                    int tile = tiles[x][y];
                    if (tile == 1 || tile == 6) {
                        int idx = getTileIndex(tiles, x, y, width, height);
                        DrawTexture(tileTextures[idx], x * 32, y * 32, WHITE);
                    } else if (tile == 4 || tile == 7) {
                        float alpha = std::min(transitionTimers[x][y] / GLITCH_TIME, 1.0f);
                        int r = GetRandomValue(180, 255);
                        int g = GetRandomValue(0, 255);
                        int b = GetRandomValue(180, 255);
                        Color glitchColor = { (unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)(alpha * 255) };
                        DrawRectangle(x * 32, y * 32, 32, 32, glitchColor);
                    } else if (tile == 5) {
                        float alpha = 1.0f - (transitionTimers[x][y] / GLITCH_TIME);
                        alpha = std::max(alpha, 0.0f); // Ensure alpha doesn't go below 0
                        int r = GetRandomValue(0, 255);
                        int g = GetRandomValue(180, 255);
                        int b = GetRandomValue(0, 180);
                        Color glitchColor = { (unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)(alpha * 255) };
                        DrawRectangle(x * 32, y * 32, 32, 32, glitchColor);
                    } else if (tile == TILE_HIGHLIGHT_CREATE) {
                        float alpha = getBlinkAlpha(transitionTimers[x][y], BLINK_CYCLE_TIME, MIN_HIGHLIGHT_OPACITY);
                        Color highlightColor = { 0, 255, 0, (unsigned char)(alpha * 255) };
                        int idx = getTileIndex(tiles, x, y, width, height);
                        DrawTexture(tileTextures[idx], x * 32, y * 32, highlightColor);
                    } else if (tile == TILE_HIGHLIGHT_DELETE) {
                        float alpha = getBlinkAlpha(transitionTimers[x][y], BLINK_CYCLE_TIME, MIN_HIGHLIGHT_OPACITY);
                        Color highlightColor = { 255, 0, 0, (unsigned char)(alpha * 255) };
                        int idx = getTileIndex(tiles, x, y, width, height);
                        DrawTexture(tileTextures[idx], x * 32, y * 32, highlightColor);
                    } else if (tile == 2) {
                        DrawRectangle(x * 32 + 10, y * 32, 12, 32, GOLD);
                    } else if (tile == 3) {
                        DrawRectangle(x * 32 + 14, y * 32, 4, 32, SKYBLUE);
                    }
                }
            }
        }
    }
}