#include "map/Map.hpp"
#include <vector>
#include <algorithm>
#include <cmath>

using namespace MapConstants;

void Map::drawLavaFluid(const Camera2D& camera) const {
    float viewX = camera.target.x - (camera.offset.x / camera.zoom);
    float viewY = camera.target.y - (camera.offset.y / camera.zoom);
    float viewWidth = GetScreenWidth() / camera.zoom;
    float viewHeight = GetScreenHeight() / camera.zoom;
    
    static int frameCount = 0;
    frameCount++;
    float time = frameCount * 0.012f;
    
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
            if (!isLavaTile(x, y) || lavaGrid[x][y].mass <= Map::LAVA_MIN_MASS) continue;
            
            float worldX = x * 32.0f;
            float worldY = y * 32.0f;

            if (worldX + 32 < viewX || worldX > viewX + viewWidth ||
                worldY + 32 < viewY || worldY > viewY + viewHeight) {
                continue;
            }

            const LavaCell& cell = lavaGrid[x][y];
            
            float targetHorizontalFlow = 0.0f;
            float targetVerticalFlow = 0.0f;
            
            float leftMass = (x > 0 && isLavaTile(x - 1, y)) ? lavaGrid[x - 1][y].mass : 0.0f;
            float rightMass = (x < width - 1 && isLavaTile(x + 1, y)) ? lavaGrid[x + 1][y].mass : 0.0f;
            float topMass = (y > 0 && isLavaTile(x, y - 1)) ? lavaGrid[x][y - 1].mass : 0.0f;
            float bottomMass = (y < height - 1 && isLavaTile(x, y + 1)) ? lavaGrid[x][y + 1].mass : 0.0f;
            
            if (x > 0 && isLavaTile(x - 1, y)) {
                targetHorizontalFlow += (leftMass - cell.mass) * 2.0f;
            }
            if (x < width - 1 && isLavaTile(x + 1, y)) {
                targetHorizontalFlow += (cell.mass - rightMass) * 2.0f;
            }
            
            if (y > 0 && isLavaTile(x, y - 1)) {
                targetVerticalFlow += (topMass - cell.mass) * 3.0f;
            }
            if (y < height - 1 && isLavaTile(x, y + 1)) {
                targetVerticalFlow += (cell.mass - bottomMass) * 1.5f;
            }
            
            targetHorizontalFlow = std::clamp(targetHorizontalFlow, -2.0f, 2.0f);
            targetVerticalFlow = std::clamp(targetVerticalFlow, -2.0f, 2.0f);
            
            float dt = 0.012f;
            float lerpFactor = 1.0f - exp(-interpolationSpeed * dt);
            
            prevHorizontalFlow[x][y] += (targetHorizontalFlow - prevHorizontalFlow[x][y]) * lerpFactor;
            prevVerticalFlow[x][y] += (targetVerticalFlow - prevVerticalFlow[x][y]) * lerpFactor;
            
            float horizontalFlow = prevHorizontalFlow[x][y];
            float verticalFlow = prevVerticalFlow[x][y];

            float massRatio = std::clamp(cell.mass / Map::LAVA_MAX_MASS, 0.1f, 1.0f);
            float avgNeighborMass = (leftMass + rightMass + topMass + bottomMass) / 4.0f;
            float blendFactor = std::clamp(avgNeighborMass / Map::LAVA_MAX_MASS, 0.0f, 1.0f);
            
            float massScale = std::clamp(massRatio * 1.2f, 0.3f, 1.0f);
            float baseHeight = (8.0f + (massRatio * 24.0f)) * massScale;
            float baseWidth = (16.0f + (massRatio * 16.0f)) * massScale;

            if (blendFactor > 0.2f) {
                baseHeight += blendFactor * 6.0f; 
                baseWidth += blendFactor * 4.0f; 
            }
            
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

            if (hasLavaLeft && leftMass > Map::LAVA_MIN_MASS) {
                float leftBlend = std::min(leftMass / cell.mass, 1.5f);
                targetWidthMultiplier += leftBlend * 0.2f;
            }
            if (hasLavaRight && rightMass > Map::LAVA_MIN_MASS) {
                float rightBlend = std::min(rightMass / cell.mass, 1.5f);
                targetWidthMultiplier += rightBlend * 0.2f;
            }
            if (hasLavaAbove && topMass > Map::LAVA_MIN_MASS) {
                float topBlend = std::min(topMass / cell.mass, 1.5f);
                targetHeightMultiplier += topBlend * 0.3f;
            }
            if (hasLavaBelow && bottomMass > Map::LAVA_MIN_MASS) {
                float bottomBlend = std::min(bottomMass / cell.mass, 1.5f);
                targetHeightMultiplier += bottomBlend * 0.4f;
            }
            
            prevWidthMultiplier[x][y] += (targetWidthMultiplier - prevWidthMultiplier[x][y]) * lerpFactor;
            prevHeightMultiplier[x][y] += (targetHeightMultiplier - prevHeightMultiplier[x][y]) * lerpFactor;
            
            float finalWidth = std::clamp(baseWidth * prevWidthMultiplier[x][y], 24.0f, 56.0f);
            float finalHeight = std::clamp(baseHeight * prevHeightMultiplier[x][y], 16.0f, 56.0f);
            
            float widthOffset = (32.0f - finalWidth) * 0.5f;
            float topOffset = 32.0f - finalHeight;
            
            bool isFalling = verticalFlow > 0.1f && hasLavaAbove && !hasLavaBelow;
            bool isReceivingFall = hasLavaAbove && topMass > Map::LAVA_MIN_MASS && lavaGrid[x][y-1].flow > 0.1f;
            
            if (hasLavaLeft && leftMass > Map::LAVA_MIN_MASS) {
                widthOffset = 0.0f;
                finalWidth = 48.0f;
            }
            if (hasLavaRight && rightMass > Map::LAVA_MIN_MASS) {
                if (widthOffset == 0.0f) {
                    finalWidth = 64.0f;
                } else {
                    finalWidth = 48.0f;
                    widthOffset = -16.0f;
                }
            }
            
            if (hasLavaAbove && topMass > Map::LAVA_MIN_MASS) {
                topOffset = 0.0f;
                finalHeight = 48.0f;
            }
            if (hasLavaBelow && bottomMass > Map::LAVA_MIN_MASS) {
                if (topOffset == 0.0f) {
                    finalHeight = 64.0f;
                } else {
                    finalHeight = 48.0f;
                    topOffset = -16.0f;
                }
            }
            
            if (isFalling) {
                finalHeight += 16.0f;
            }
            
            bool hasLavaTopLeft = (x > 0 && y > 0) && isLavaTile(x - 1, y - 1);
            bool hasLavaTopRight = (x < width - 1 && y > 0) && isLavaTile(x + 1, y - 1);
            bool hasLavaBottomLeft = (x > 0 && y < height - 1) && isLavaTile(x - 1, y + 1);
            bool hasLavaBottomRight = (x < width - 1 && y < height - 1) && isLavaTile(x + 1, y + 1);
            
            if (hasLavaLeft && hasLavaAbove && hasLavaTopLeft) {
                widthOffset = std::min(widthOffset, 0.0f);
                topOffset = std::min(topOffset, 0.0f);
                finalWidth = std::max(finalWidth, 48.0f);
                finalHeight = std::max(finalHeight, 48.0f);
            }
            if (hasLavaRight && hasLavaAbove && hasLavaTopRight) {
                topOffset = std::min(topOffset, 0.0f);
                finalWidth = std::max(finalWidth, 48.0f);
                finalHeight = std::max(finalHeight, 48.0f);
            }
            if (hasLavaLeft && hasLavaBelow && hasLavaBottomLeft) {
                widthOffset = std::min(widthOffset, 0.0f);
                finalWidth = std::max(finalWidth, 48.0f);
                finalHeight = std::max(finalHeight, 48.0f);
            }
            if (hasLavaRight && hasLavaBelow && hasLavaBottomRight) {
                finalWidth = std::max(finalWidth, 48.0f);
                finalHeight = std::max(finalHeight, 48.0f);
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

            float roundness = std::clamp(massRatio * 8.0f, 2.0f, 8.0f);
            
            if (hasLavaLeft || hasLavaRight) roundness *= 0.6f;
            if (hasLavaAbove || hasLavaBelow) roundness *= 0.6f;
            
            if (isFalling) {
                roundness *= 0.3f;
            }
            
            float actualRoundness = std::clamp(roundness / std::max(std::min(finalWidth, finalHeight), 4.0f), 0.05f, 0.4f);
            
            DrawRectangle((int)finalX, (int)finalTopY, (int)finalWidth, (int)finalHeight, lavaColor);

            Color glowColor = {lavaColor.r, lavaColor.g, lavaColor.b, 50};
            DrawRectangle((int)(finalX - 2), (int)(finalTopY - 2), (int)(finalWidth + 4), (int)(finalHeight + 4), glowColor);
            
            Color bridgeColor = {lavaColor.r, lavaColor.g, lavaColor.b, lavaColor.a};
            
            if (hasLavaLeft && leftMass > Map::LAVA_MIN_MASS * 0.3f) {
                DrawRectangle((int)(worldX - 16), (int)(finalTopY + finalHeight * 0.2f), 24, (int)(finalHeight * 0.6f), bridgeColor);
            }
            if (hasLavaRight && rightMass > Map::LAVA_MIN_MASS * 0.3f) {
                DrawRectangle((int)(worldX + 24), (int)(finalTopY + finalHeight * 0.2f), 24, (int)(finalHeight * 0.6f), bridgeColor);
            }
            
            if (hasLavaAbove && topMass > Map::LAVA_MIN_MASS * 0.3f) {
                DrawRectangle((int)(finalX + finalWidth * 0.2f), (int)(worldY - 16), (int)(finalWidth * 0.6f), 24, bridgeColor);
            }
            if (hasLavaBelow && bottomMass > Map::LAVA_MIN_MASS * 0.3f) {
                DrawRectangle((int)(finalX + finalWidth * 0.2f), (int)(worldY + 24), (int)(finalWidth * 0.6f), 24, bridgeColor);
            }
            
            if (hasLavaLeft && hasLavaAbove && hasLavaTopLeft) {
                DrawRectangle((int)(worldX - 8), (int)(worldY - 8), 16, 16, bridgeColor);
            }
            if (hasLavaRight && hasLavaAbove && hasLavaTopRight) {
                DrawRectangle((int)(worldX + 24), (int)(worldY - 8), 16, 16, bridgeColor);
            }
            if (hasLavaLeft && hasLavaBelow && hasLavaBottomLeft) {
                DrawRectangle((int)(worldX - 8), (int)(worldY + 24), 16, 16, bridgeColor);
            }
            if (hasLavaRight && hasLavaBelow && hasLavaBottomRight) {
                DrawRectangle((int)(worldX + 24), (int)(worldY + 24), 16, 16, bridgeColor);
            }
            
            if (isFalling || isReceivingFall) {
                Color connectColor = {lavaColor.r, lavaColor.g, lavaColor.b, (unsigned char)(lavaColor.a * 0.9f)};
                
                if (hasLavaAbove) {
                    DrawRectangle((int)worldX, (int)(finalTopY - 16), 32, 20, connectColor);
                }
                if (hasLavaBelow || isFalling) {
                    DrawRectangle((int)worldX, (int)(finalTopY + finalHeight - 4), 32, 20, connectColor);
                }
            }
            
            if (hasLavaAbove && topMass > Map::LAVA_MIN_MASS * 0.3f) {
                Color blendColor = {lavaColor.r, lavaColor.g, lavaColor.b, (unsigned char)(lavaColor.a * 0.8f)};
                DrawRectangle((int)(worldX + 4), (int)(finalTopY - 8), 24, 12, blendColor);
            }
            if (hasLavaBelow && bottomMass > Map::LAVA_MIN_MASS * 0.3f) {
                Color blendColor = {lavaColor.r, lavaColor.g, lavaColor.b, (unsigned char)(lavaColor.a * 0.8f)};
                DrawRectangle((int)(worldX + 4), (int)(finalTopY + finalHeight - 4), 24, 12, blendColor);
            }
            if (hasLavaLeft && leftMass > Map::LAVA_MIN_MASS * 0.3f) {
                Color blendColor = {lavaColor.r, lavaColor.g, lavaColor.b, (unsigned char)(lavaColor.a * 0.8f)};
                DrawRectangle((int)(worldX - 8), (int)(finalTopY + 4), 12, 24, blendColor);
            }
            if (hasLavaRight && rightMass > Map::LAVA_MIN_MASS * 0.3f) {
                Color blendColor = {lavaColor.r, lavaColor.g, lavaColor.b, (unsigned char)(lavaColor.a * 0.8f)};
                DrawRectangle((int)(finalX + finalWidth - 4), (int)(finalTopY + 4), 12, 24, blendColor);
            }

            if (cell.flow > 0.02f) {
                Color flowColor = {
                    (unsigned char)std::min(255, lavaColor.r + 40),
                    (unsigned char)std::min(255, lavaColor.g + 30),
                    (unsigned char)std::min(120, lavaColor.b + 30),
                    150
                };
                
                if (verticalFlow > 0.3f && !hasLavaBelow) {
                    DrawRectangle((int)(finalX + finalWidth * 0.3f), (int)(finalTopY + finalHeight - 2), (int)(finalWidth * 0.4f), 8, flowColor);
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