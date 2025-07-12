#include "map/Map.hpp" 
#include <cmath>
#include <algorithm> 

using namespace MapConstants;

struct TileBatch {
    std::vector<Vector2> positions;
    std::vector<Color> colors;
    int textureIndex;
    
    void clear() {
        positions.clear();
        colors.clear();
    }
};

struct RectBatch {
    std::vector<Rectangle> rects;
    std::vector<Color> colors;
    
    void clear() {
        rects.clear();
        colors.clear();
    }
};
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
    static bool colorsInitialized = false;
    static std::vector<std::vector<Color>> treasureColors;
    static std::vector<std::vector<Color>> shopColors;
    static std::vector<std::vector<Color>> specialColors;
    if (!colorsInitialized) {
        treasureColors.resize(width, std::vector<Color>(height, WHITE));
        shopColors.resize(width, std::vector<Color>(height, WHITE));
        specialColors.resize(width, std::vector<Color>(height, WHITE));
        for (int x = 0; x < width; ++x) {
            for (int y = 0; y < height; ++y) {
                treasureColors[x][y] = {(unsigned char)GetRandomValue(180, 255), (unsigned char)GetRandomValue(0, 255), (unsigned char)GetRandomValue(180, 255), 255};
                shopColors[x][y] = {(unsigned char)GetRandomValue(0, 255), (unsigned char)GetRandomValue(180, 255), (unsigned char)GetRandomValue(0, 180), 255};
                specialColors[x][y] = {(unsigned char)GetRandomValue(100, 200), (unsigned char)GetRandomValue(100, 200), (unsigned char)GetRandomValue(200, 255), 255};
            }
        }
        colorsInitialized = true;
    }
    
    float viewX = camera.target.x - (camera.offset.x / camera.zoom);
    float viewY = camera.target.y - (camera.offset.y / camera.zoom);
    float viewWidth = GetScreenWidth() / camera.zoom;
    float viewHeight = GetScreenHeight() / camera.zoom;
    Rectangle visibleWorldRect = { viewX, viewY, viewWidth, viewHeight };
    
    static int frameCount = 0;
    frameCount++;
    
    static std::vector<TileBatch> tileBatches;
    static std::vector<RectBatch> rectBatches;
    static std::vector<Vector2> circlePositions;
    static std::vector<float> circleSizes;
    static std::vector<Color> circleColors;
    
    if (tileBatches.size() < tileTextures.size()) {
        tileBatches.resize(tileTextures.size());
    }
    
    for (auto& batch : tileBatches) {
        batch.clear();
    }
    
    rectBatches.clear();
    rectBatches.resize(7);
    
    circlePositions.clear();
    circleSizes.clear();
    circleColors.clear();

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
                        if (idx < tileBatches.size()) {
                            tileBatches[idx].positions.push_back({(float)(x * 32), (float)(y * 32)});
                            tileBatches[idx].colors.push_back(WHITE);
                            tileBatches[idx].textureIndex = idx;
                        }
                    } else if (tile == TREASURE_TILE_VALUE) {
                        float alpha = std::min(transitionTimers[x][y] / GLITCH_TIME, 1.0f);
                        Color glitchColor = treasureColors[x][y];
                        glitchColor.a = (unsigned char)(alpha * 255);
                        rectBatches[0].rects.push_back({(float)(x * 32), (float)(y * 32), 32, 32});
                        rectBatches[0].colors.push_back(glitchColor);
                    } else if (tile == SHOP_TILE_VALUE) {
                        float alpha = 1.0f - (transitionTimers[x][y] / GLITCH_TIME);
                        alpha = std::max(alpha, 0.0f);
                        Color glitchColor = shopColors[x][y];
                        glitchColor.a = (unsigned char)(alpha * 255);
                        rectBatches[1].rects.push_back({(float)(x * 32), (float)(y * 32), 32, 32});
                        rectBatches[1].colors.push_back(glitchColor);
                    } else if (tile == 8) {
                        float alpha = std::min(transitionTimers[x][y] / GLITCH_TIME, 1.0f);
                        Color glitchColor = specialColors[x][y];
                        glitchColor.a = (unsigned char)(alpha * 255);
                        rectBatches[2].rects.push_back({(float)(x * 32), (float)(y * 32), 32, 32});
                        rectBatches[2].colors.push_back(glitchColor);
                    } else if (tile == TILE_HIGHLIGHT_CREATE) {
                        float alpha = getBlinkAlpha(transitionTimers[x][y], BLINK_CYCLE_TIME, MIN_HIGHLIGHT_OPACITY);
                        Color highlightColor = { 0, 255, 0, (unsigned char)(alpha * 255) };
                        int idx = getTileIndex(tiles, x, y, width, height);
                        if (idx < tileBatches.size()) {
                            tileBatches[idx].positions.push_back({(float)(x * 32), (float)(y * 32)});
                            tileBatches[idx].colors.push_back(highlightColor);
                            tileBatches[idx].textureIndex = idx;
                        }
                    } else if (tile == TILE_HIGHLIGHT_DELETE) {
                        float alpha = getBlinkAlpha(transitionTimers[x][y], BLINK_CYCLE_TIME, MIN_HIGHLIGHT_OPACITY);
                        Color highlightColor = { 255, 0, 0, (unsigned char)(alpha * 255) };
                        int idx = getTileIndex(tiles, x, y, width, height);
                        if (idx < tileBatches.size()) {
                            tileBatches[idx].positions.push_back({(float)(x * 32), (float)(y * 32)});
                            tileBatches[idx].colors.push_back(highlightColor);
                            tileBatches[idx].textureIndex = idx;
                        }
                    } else if (tile == LADDER_TILE_VALUE) { 
                        rectBatches[3].rects.push_back({(float)(x * 32 + 10), (float)(y * 32), 12, 32});
                        rectBatches[3].colors.push_back(GOLD);
                    } else if (tile == ROPE_TILE_VALUE) {
                        rectBatches[4].rects.push_back({(float)(x * 32 + 14), (float)(y * 32), 4, 32});
                        rectBatches[4].colors.push_back(SKYBLUE);
                    } else if (tile == CHEST_TILE_VALUE) {
                        rectBatches[5].rects.push_back({(float)(x * 32), (float)(y * 32), 32, 32});
                        rectBatches[5].colors.push_back(BROWN);
                    }

                }
            }
        }
    }

    for (size_t i = 0; i < tileBatches.size(); ++i) {
        const auto& batch = tileBatches[i];
        if (!batch.positions.empty() && i < tileTextures.size()) {
            for (size_t j = 0; j < batch.positions.size(); ++j) {
                DrawTexture(tileTextures[i], (int)batch.positions[j].x, (int)batch.positions[j].y, batch.colors[j]);
            }
        }
    }

    for (const auto& batch : rectBatches) {
        for (size_t i = 0; i < batch.rects.size(); ++i) {
            DrawRectangleRec(batch.rects[i], batch.colors[i]);
        }
    }
    
    {
        std::lock_guard<std::mutex> lock(particlesMutex);
        
        int visibleCount = 0;
        for (const auto& particle : particles) {
            if (particle.position.x >= visibleWorldRect.x - 32 && 
                particle.position.x <= visibleWorldRect.x + visibleWorldRect.width + 32 &&
                particle.position.y >= visibleWorldRect.y - 32 && 
                particle.position.y <= visibleWorldRect.y + visibleWorldRect.height + 32) {
                visibleCount++;
                
                circlePositions.push_back(particle.position);
                circleSizes.push_back(particle.size);
                circleColors.push_back(particle.color);
            }
        }
        
        for (size_t i = 0; i < circlePositions.size(); ++i) {
            DrawCircleV(circlePositions[i], circleSizes[i], circleColors[i]);
        }
    }

    drawLavaFluid(camera);
    
    if (renderedChunks > 0) {
        
    }
}

void Map::drawLavaFluid(const Camera2D& camera) const {
    float viewX = camera.target.x - (camera.offset.x / camera.zoom);
    float viewY = camera.target.y - (camera.offset.y / camera.zoom);
    float viewWidth = GetScreenWidth() / camera.zoom;
    float viewHeight = GetScreenHeight() / camera.zoom;
    
    static int frameCount = 0;
    frameCount++;
    float time = frameCount * 0.016f;
    
    static std::vector<std::vector<float>> prevHorizontalFlow;
    static std::vector<std::vector<float>> prevVerticalFlow;
    static std::vector<std::vector<float>> prevWidthMultiplier;
    static std::vector<std::vector<float>> prevHeightMultiplier;
    static bool interpolationInitialized = false;
    
    if (!interpolationInitialized || prevHorizontalFlow.size() != width || prevHorizontalFlow[0].size() != height) {
        prevHorizontalFlow.resize(width, std::vector<float>(height, 0.0f));
        prevVerticalFlow.resize(width, std::vector<float>(height, 0.0f));
        prevWidthMultiplier.resize(width, std::vector<float>(height, 1.0f));
        prevHeightMultiplier.resize(width, std::vector<float>(height, 1.0f));
        interpolationInitialized = true;
    }
    
    const float interpolationSpeed = 8.0f; 
    
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            if (!isLavaTile(x, y) || lavaGrid[x][y].mass <= LAVA_MIN_MASS) continue;
            
            float worldX = x * 32.0f;
            float worldY = y * 32.0f;

            if (worldX + 32 < viewX || worldX > viewX + viewWidth ||
                worldY + 32 < viewY || worldY > viewY + viewHeight) {
                continue;
            }

            const LavaCell& cell = lavaGrid[x][y];
            
            float targetHorizontalFlow = 0.0f;
            float targetVerticalFlow = 0.0f;
            
            if (x > 0 && isLavaTile(x - 1, y)) {
                targetHorizontalFlow += (lavaGrid[x - 1][y].mass - cell.mass) * 2.0f;
            }
            if (x < width - 1 && isLavaTile(x + 1, y)) {
                targetHorizontalFlow += (cell.mass - lavaGrid[x + 1][y].mass) * 2.0f;
            }
            
            if (y > 0 && isLavaTile(x, y - 1)) {
                targetVerticalFlow += (lavaGrid[x][y - 1].mass - cell.mass) * 3.0f;
            }
            if (y < height - 1 && isLavaTile(x, y + 1)) {
                targetVerticalFlow += (cell.mass - lavaGrid[x][y + 1].mass) * 1.5f;
            }
            
            targetHorizontalFlow = std::clamp(targetHorizontalFlow, -2.0f, 2.0f);
            targetVerticalFlow = std::clamp(targetVerticalFlow, -2.0f, 2.0f);
            
            float dt = 0.016f;
            float lerpFactor = 1.0f - exp(-interpolationSpeed * dt);
            
            prevHorizontalFlow[x][y] += (targetHorizontalFlow - prevHorizontalFlow[x][y]) * lerpFactor;
            prevVerticalFlow[x][y] += (targetVerticalFlow - prevVerticalFlow[x][y]) * lerpFactor;
            
            float horizontalFlow = prevHorizontalFlow[x][y];
            float verticalFlow = prevVerticalFlow[x][y];
            
            float massRatio = std::clamp(cell.mass / LAVA_MAX_MASS, 0.2f, 1.0f);
            float baseHeight = 16.0f + (massRatio * 16.0f);
            float baseWidth = 32.0f;
            
            float flowStrength = std::min(cell.flow * 20.0f, 1.0f);
            
            bool hasLavaAbove = (y > 0) && isLavaTile(x, y - 1);
            bool hasLavaBelow = (y < height - 1) && isLavaTile(x, y + 1);
            bool hasLavaLeft = (x > 0) && isLavaTile(x - 1, y);
            bool hasLavaRight = (x < width - 1) && isLavaTile(x + 1, y);
            
            float targetWidthMultiplier = 1.0f;
            float targetHeightMultiplier = 1.0f;
            float horizontalStretch = 0.0f;
            float verticalStretch = 0.0f;
            
            if (std::abs(horizontalFlow) > 0.1f) {
                horizontalStretch = std::abs(horizontalFlow) * flowStrength;
                targetWidthMultiplier = 1.0f + horizontalStretch * 0.8f;
                targetHeightMultiplier = 1.0f - horizontalStretch * 0.4f;
            }
            
            if (std::abs(verticalFlow) > std::abs(horizontalFlow) && verticalFlow > 0.1f) {
                verticalStretch = verticalFlow * flowStrength;
                targetHeightMultiplier = 1.0f + verticalStretch * 1.2f;
                targetWidthMultiplier = 1.0f - verticalStretch * 0.6f;
            }
            
            if (hasLavaLeft) targetWidthMultiplier += 0.15f;
            if (hasLavaRight) targetWidthMultiplier += 0.15f;
            if (hasLavaAbove) targetHeightMultiplier += 0.15f;
            if (hasLavaBelow) targetHeightMultiplier += 0.25f;
            
            prevWidthMultiplier[x][y] += (targetWidthMultiplier - prevWidthMultiplier[x][y]) * lerpFactor;
            prevHeightMultiplier[x][y] += (targetHeightMultiplier - prevHeightMultiplier[x][y]) * lerpFactor;
            
            float finalWidth = std::clamp(baseWidth * prevWidthMultiplier[x][y], 20.0f, 48.0f);
            float finalHeight = std::clamp(baseHeight * prevHeightMultiplier[x][y], 12.0f, 48.0f);
            
            float widthOffset = (32.0f - finalWidth) * 0.5f;
            if (hasLavaLeft) widthOffset -= 3.0f;
            if (hasLavaRight) finalWidth += 3.0f;
            
            float topOffset = 32.0f - finalHeight;
            if (hasLavaAbove) {
                topOffset -= 4.0f;
                finalHeight += 4.0f;
            }
            if (hasLavaBelow) {
                finalHeight += 5.0f;
            }
            
            float waveOffset = cell.settled ? 0.0f : 0.5f * sinf(time * 2.0f + x * 0.5f);
            
            float glow = 0.5f + 0.5f * sinf(time * 3.0f + x * 0.3f + y * 0.2f);
            float flowGlow = cell.flow > 0.01f ? 0.3f + flowStrength * 0.2f : 0.0f;
            
            float smoothFlowIntensity = (std::abs(horizontalFlow) + std::abs(verticalFlow)) * 0.3f;
            smoothFlowIntensity = std::clamp(smoothFlowIntensity, 0.0f, 1.0f);

            Color lavaColor = {
                (unsigned char)std::clamp(200 + (int)(55 * glow) + (int)(40 * (flowGlow + smoothFlowIntensity * 0.3f)), 0, 255),
                (unsigned char)std::clamp(80 + (int)(40 * glow) + (int)(30 * (flowGlow + smoothFlowIntensity * 0.2f)), 0, 255),
                (unsigned char)std::clamp(10 + (int)(15 * (flowGlow + smoothFlowIntensity * 0.1f)), 0, 255),
                255
            };

            float finalTopY = worldY + topOffset + waveOffset;
            float finalX = worldX + widthOffset;

            Rectangle lavaRect = {
                finalX,
                finalTopY,
                finalWidth,
                finalHeight
            };
            DrawRectangleRec(lavaRect, lavaColor);

            Color glowColor = {lavaColor.r, lavaColor.g, lavaColor.b, 50};
            DrawRectangle(finalX - 2, finalTopY - 2, finalWidth + 4, finalHeight + 4, glowColor);

            if (cell.flow > 0.02f) {
                Color flowColor = {
                    (unsigned char)std::min(255, lavaColor.r + 40),
                    (unsigned char)std::min(255, lavaColor.g + 30),
                    (unsigned char)std::min(120, lavaColor.b + 30),
                    150
                };

                if (horizontalFlow < -0.2f) {
                    Vector2 p1 = {finalX, finalTopY + finalHeight * 0.25f};
                    Vector2 p2 = {finalX, finalTopY + finalHeight * 0.75f};
                    Vector2 p3 = {finalX - 8.0f, finalTopY + finalHeight * 0.5f};
                    DrawTriangle(p1, p2, p3, flowColor);
                }
                
                if (horizontalFlow > 0.2f) {
                    Vector2 p1 = {finalX + finalWidth, finalTopY + finalHeight * 0.25f};
                    Vector2 p2 = {finalX + finalWidth, finalTopY + finalHeight * 0.75f};
                    Vector2 p3 = {finalX + finalWidth + 8.0f, finalTopY + finalHeight * 0.5f};
                    DrawTriangle(p1, p2, p3, flowColor);
                }
                
                if (verticalFlow > 0.3f && !hasLavaBelow) {
                    Vector2 p1 = {finalX + finalWidth * 0.25f, worldY + 32};
                    Vector2 p2 = {finalX + finalWidth * 0.75f, worldY + 32};
                    Vector2 p3 = {finalX + finalWidth * 0.5f, worldY + 32 + 6};
                    DrawTriangle(p1, p2, p3, flowColor);
                }
            }

            if (cell.mass > 0.6f) {
                float bubbleTime = time * 4.0f + x * 1.1f + y * 0.9f;
                if (sinf(bubbleTime) > 0.7f) {
                    Color bubbleColor = {255, 200, 100, 180};
                    Vector2 bubblePos = {
                        finalX + finalWidth * 0.5f + 6 * cosf(bubbleTime),
                        finalTopY + 2
                    };
                    DrawCircleV(bubblePos, 2.5f, bubbleColor);
                }
            }
        }
    }
}