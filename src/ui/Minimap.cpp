#include "ui/Minimap.hpp"
#include <algorithm>
#include <cmath>

namespace UI {
    Minimap::Minimap(int x, int y, int w, int h) 
        : posX(x), posY(y), width(w), height(h), lastPlayerPos({-1000, -1000}) {
        minimapTexture = LoadRenderTexture(width, height);
    }
    
    Minimap::~Minimap() {
        UnloadRenderTexture(minimapTexture);
    }
    
    void Minimap::setPosition(int x, int y) {
        posX = x;
        posY = y;
    }
    
    void Minimap::setSize(int w, int h) {
        width = w;
        height = h;
        UnloadRenderTexture(minimapTexture);
        minimapTexture = LoadRenderTexture(width, height);
        lastPlayerPos = {-1000, -1000}; 
    }
    
    void Minimap::update(const Map& map, const Player& player) {
        Vector2 currentPlayerPos = player.getPosition();
        
        
        
        updateMinimapTexture(map, player);
        lastPlayerPos = currentPlayerPos;
        
    }
    
    void Minimap::updateMinimapTexture(const Map& map, const Player& player) {
        BeginTextureMode(minimapTexture);
        ClearBackground(BLACK);
        
        Vector2 playerPos = player.getPosition();
        int playerTileX = static_cast<int>(playerPos.x / 32.0f);
        int playerTileY = static_cast<int>(playerPos.y / 32.0f);
        
        int textureWidth = minimapTexture.texture.width;
        int textureHeight = minimapTexture.texture.height;
        

        int tilesWidth = textureWidth / MINIMAP_SCALE;
        int tilesHeight = textureHeight / MINIMAP_SCALE;
        

        int viewRadius = std::min(VIEW_RADIUS, std::min(tilesWidth, tilesHeight) / 2);
        int startX = playerTileX - viewRadius;
        int startY = playerTileY - viewRadius;
        int endX = playerTileX + viewRadius;
        int endY = playerTileY + viewRadius;

        int offsetX = (textureWidth - (viewRadius * 2 * MINIMAP_SCALE)) / 2;
        int offsetY = (textureHeight - (viewRadius * 2 * MINIMAP_SCALE)) / 2;
        
        for (int y = startY; y <= endY; ++y) {
            for (int x = startX; x <= endX; ++x) {
                if (map.isInsideBounds(x, y)) {
                    int tileType = map.getTileValue(x, y);
                    Color tileColor = getTileColor(tileType);
                    
                    if (tileColor.a > 0) {
                        int pixelX = offsetX + ((x - startX) * MINIMAP_SCALE);
                        int pixelY = offsetY + ((y - startY) * MINIMAP_SCALE);
                        
                        DrawRectangle(pixelX, pixelY, MINIMAP_SCALE, MINIMAP_SCALE, tileColor);
                    }
                }
            }
        }
        
        EndTextureMode();
    }
    
    Color Minimap::getTileColor(int tileValue) const {
        switch (tileValue) {
            case 1:
                return ColorAlpha(WHITE, 0.8f);
            case 6:
                return ColorAlpha(GRAY, 0.7f);
            case 2:
                return ColorAlpha(BROWN, 0.6f);
            case 3:
                return ColorAlpha(YELLOW, 0.5f);
            case 4:
                return ColorAlpha(GOLD, 0.9f);
            case 5:
                return ColorAlpha(PURPLE, 0.8f);
            case 7:
                return ColorAlpha(ORANGE, 0.9f);
            case 0:
            default:
                return ColorAlpha(BLACK, 0.0f);
        }
    }
    
    void Minimap::draw(const Map& map, const Player& player) {

        Color shadowColor = ColorAlpha(BLACK, 0.5f);
        DrawRectangle(posX + SHADOW_OFFSET, posY + SHADOW_OFFSET, width, height, shadowColor);

        Rectangle sourceRect = {0, 0, static_cast<float>(minimapTexture.texture.width), -static_cast<float>(minimapTexture.texture.height)};
        Rectangle destRect = {static_cast<float>(posX), static_cast<float>(posY), 
                              static_cast<float>(width), static_cast<float>(height)};
        
        DrawTexturePro(minimapTexture.texture, sourceRect, destRect, {0, 0}, 0.0f, WHITE);

        DrawRectangleLines(posX, posY, width, height, WHITE);

        int centerX = posX + width / 2;
        int centerY = posY + height / 2;
        
        DrawCircle(centerX, centerY, PLAYER_DOT_SIZE, RED);
        DrawCircleLines(centerX, centerY, PLAYER_DOT_SIZE, WHITE);
    }
}
