#include "Map.hpp"

Map::Map(int w, int h) : width(w), height(h), tiles(w, std::vector<int>(h, 0)) {
    // Add platforms, walls, and ladders procedurally
    for (int x = 0; x < width; x++) {
        tiles[x][height - 1] = 1; // floor

        if (x % 10 == 0)
            for (int y = height - 2; y > height - 6; y--)
                tiles[x][y] = 1; // platform

        if (x % 15 == 0)
            for (int y = height - 1; y > height - 8; y--)
                tiles[x][y] = 2; // ladder
    }
}

void Map::draw() const {
    for (int x = 0; x < width; ++x)
        for (int y = 0; y < height; ++y) {
            if (tiles[x][y] == 1)
                DrawRectangle(x * 32, y * 32, 32, 32, DARKGRAY);
            else if (tiles[x][y] == 2)
                DrawRectangle(x * 32 + 10, y * 32, 12, 32, GOLD);
        }
}

bool Map::collidesWithGround(Vector2 pos) const {
    int tx = pos.x / 32;
    int ty = pos.y / 32;
    if (tx >= 0 && tx < width && ty >= 0 && ty < height)
        return tiles[tx][ty] == 1;
    return false;
}

bool Map::isSolidTile(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) return false;
    return tiles[x][y] == 1;
}
