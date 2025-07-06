#pragma once
#include <raylib.h>
#include "map/Map.hpp"
#include "Player.hpp"

namespace UI {
    class Minimap {
    public:
        Minimap(int x, int y, int w, int h);
        ~Minimap();
        
        void update(const Map& map, const Player& player);
        void draw(const Map& map, const Player& player);
        
        void setPosition(int x, int y);
        void setSize(int w, int h);
        
    private:
        int posX, posY;
        int width, height;
        
        RenderTexture2D minimapTexture;
        Vector2 lastPlayerPos;
        
        static constexpr int MINIMAP_SCALE = 2;
        static constexpr float BORDER_WIDTH = 2.0f;
        static constexpr int PLAYER_DOT_SIZE = 4;
        static constexpr int VIEW_RADIUS = 160; 
        static constexpr float UPDATE_THRESHOLD = 16.0f; 
        static constexpr int SHADOW_OFFSET = 4; 
        
        void updateMinimapTexture(const Map& map, const Player& player);
        Color getTileColor(int tileValue) const;
    };
}
