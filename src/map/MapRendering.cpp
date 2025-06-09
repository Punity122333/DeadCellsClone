#include "map/Map.hpp" 
#include <cmath> 
#include <algorithm> 

using namespace MapConstants;
namespace {
    
    


    
    int getTileIndex(const std::vector<std::vector<int>>& tiles, int x, int y, int width, int height) {
        bool top    = (y > 0)            && (tiles[x][y - 1] == WALL_TILE_VALUE || tiles[x][y - 1] == PLATFORM_TILE_VALUE);
        bool bottom = (y < height - 1)   && (tiles[x][y + 1] == WALL_TILE_VALUE || tiles[x][y + 1] == PLATFORM_TILE_VALUE);
        bool left   = (x > 0)            && (tiles[x - 1][y] == WALL_TILE_VALUE || tiles[x - 1][y] == PLATFORM_TILE_VALUE);
        bool right  = (x < width - 1)    && (tiles[x + 1][y] == WALL_TILE_VALUE || tiles[x + 1][y] == PLATFORM_TILE_VALUE);

        
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

    
    static int frameCount = 0;
    frameCount++;
    if (frameCount % 60 == 0) {
                FILE* debugFile = fopen("camera_debug.txt", "a");
        if (debugFile) {
            fprintf(debugFile, "Frame %d: Camera target=(%.1f,%.1f), visible rect=(%.1f,%.1f,%.1f,%.1f)\n",
                   frameCount, camera.target.x, camera.target.y, viewX, viewY, viewWidth, viewHeight);
            fclose(debugFile);
        }
    }

    int renderedChunks = 0;
    for (const auto& chunk : chunks) {
        Rectangle chunkRect = {
            (float)chunk.startX * 32.0f,
            (float)chunk.startY * 32.0f,
            (float)(chunk.endX - chunk.startX + 1) * 32.0f,
            (float)(chunk.endY - chunk.startY + 1) * 32.0f
        };

        if (CheckCollisionRecs(visibleWorldRect, chunkRect)) {
            renderedChunks++;
            for (int x = chunk.startX; x <= chunk.endX; ++x) {
                for (int y = chunk.startY; y <= chunk.endY; ++y) {
                    if (x < 0 || x >= width || y < 0 || y >= height) continue;
                    int tile = tiles[x][y];

                    
                    if (tile < 0 || tile > 1000) { 
                        printf("ERROR: Invalid tile value %d at (%d,%d)\n", tile, x, y);
                        continue;
                    }

                    if (tile == WALL_TILE_VALUE || tile == PLATFORM_TILE_VALUE) {
                        int idx = getTileIndex(tiles, x, y, width, height);
                        DrawTexture(tileTextures[idx], x * 32, y * 32, WHITE);
                    } else if (tile == TREASURE_TILE_VALUE) {
                        float alpha = std::min(transitionTimers[x][y] / GLITCH_TIME, 1.0f);
                        int r = GetRandomValue(180, 255);
                        int g = GetRandomValue(0, 255);
                        int b = GetRandomValue(180, 255);
                        Color glitchColor = { (unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)(alpha * 255) };
                        DrawRectangle(x * 32, y * 32, 32, 32, glitchColor);
                    } else if (tile == SHOP_TILE_VALUE) {
                        float alpha = 1.0f - (transitionTimers[x][y] / GLITCH_TIME);
                        alpha = std::max(alpha, 0.0f);
                        int r = GetRandomValue(0, 255);
                        int g = GetRandomValue(180, 255);
                        int b = GetRandomValue(0, 180);
                        Color glitchColor = { (unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)(alpha * 255) };
                        DrawRectangle(x * 32, y * 32, 32, 32, glitchColor);
                    } else if (tile == 8) {
                        float alpha = std::min(transitionTimers[x][y] / GLITCH_TIME, 1.0f);
                        int r = GetRandomValue(100, 200);
                        int g = GetRandomValue(100, 200);
                        int b = GetRandomValue(200, 255);
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
                    } else if (tile == LADDER_TILE_VALUE) { 
                        DrawRectangle(x * 32 + 10, y * 32, 12, 32, GOLD);
                    } else if (tile == ROPE_TILE_VALUE) {
                        DrawRectangle(x * 32 + 14, y * 32, 4, 32, SKYBLUE);
                    } else if (tile == CHEST_TILE_VALUE) {
                        FILE* debugFile = fopen("camera_debug.txt", "a");
                        if (debugFile) {
                            fprintf(debugFile, "Frame %d: Rendering chest (texture) at (%d,%d)\n", frameCount, x, y);
                            fclose(debugFile);
                        }
                        DrawRectangle(x * 32, y * 32, 32, 32, BROWN); 
                    }
                    
                }
            }
        }
    }

    if (frameCount % 60 == 0) {
        FILE* chestDebugFile = fopen("chest_tiles_debug.txt", "a");
        if (chestDebugFile) {
            fprintf(chestDebugFile, "Frame %d: CHEST_TILE_VALUE positions:\n", frameCount);
            int chestCount = 0;
            for (int x = 0; x < width; ++x) {
                for (int y = 0; y < height; ++y) {
                    if (tiles[x][y] == CHEST_TILE_VALUE) {
                        fprintf(chestDebugFile, "  Chest at (%d, %d)\n", x, y);
                        chestCount++;
                    }
                }
            }
            fprintf(chestDebugFile, "Total chests: %d\n\n", chestCount);
            fclose(chestDebugFile);
        }
    }

    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            if (tiles[x][y] == CHEST_TILE_VALUE) {
                Rectangle chestRect = { x*32.0f, y*32.0f, 32.0f, 32.0f };
                if (CheckCollisionRecs(visibleWorldRect, chestRect)) {
                    DrawRectangle(x * 32, y * 32, 32, 32, BROWN);
                }
            }
        }
    }

    
    if (frameCount % 60 == 0) {
        FILE* debugFile = fopen("camera_debug.txt", "a");
        if (debugFile) {
            fprintf(debugFile, "Frame %d: Rendered %d chunks out of %d total\n",
                   frameCount, renderedChunks, (int)chunks.size());
            fclose(debugFile);
        }
    }
}