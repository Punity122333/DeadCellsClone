#include "Map.hpp"
#include <stack>
#include <vector>

bool Map::collidesWithGround(Vector2 pos) const {
    int tx = static_cast<int>(pos.x / 32);
    int ty = static_cast<int>(pos.y / 32);
    if (tx >= 0 && tx < width && ty >= 0 && ty < height) {
        return tiles[tx][ty] == 1 || tiles[tx][ty] == 6 || tiles[tx][ty] == TILE_HIGHLIGHT_DELETE;
    }
    return false;
}

bool Map::isSolidTile(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) return false;
    return tiles[x][y] == 1 || tiles[x][y] == 6 || tiles[x][y] == TILE_HIGHLIGHT_DELETE;
}

bool Map::isLadderTile(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) return false;
    return tiles[x][y] == 2;
}

bool Map::isRopeTile(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) return false;
    return tiles[x][y] == 3;
}

bool Map::isTileEmpty(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) return false;
    return tiles[x][y] == 0 || tiles[x][y] == TILE_HIGHLIGHT_CREATE;
}

Vector2 Map::findEmptySpawn() const {
    int totalEmpty = countEmptyTiles();
    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            if (tiles[x][y] == 0 && y + 1 < height && (tiles[x][y + 1] == 1 || tiles[x][y + 1] == 6)) {
                int reachable = countReachableEmptyTiles(x, y);
                if (reachable >= totalEmpty * 0.8f) {
                    return { x * 32.0f, y * 32.0f };
                }
            }
        }
    }
    return { 32.0f, 32.0f };
}

int Map::countEmptyTiles() const {
    int count = 0;
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            if (tiles[x][y] == 0) count++;
    return count;
}

int Map::countReachableEmptyTiles(int startX, int startY) const {
    std::vector<std::vector<bool>> visited(width, std::vector<bool>(height, false));
    std::stack<std::pair<int, int>> s;
    s.push({startX, startY});
    int reachable = 0;

    while (!s.empty()) {
        auto [x, y] = s.top(); s.pop();
        if (x < 0 || x >= width || y < 0 || y >= height) continue;
        if (visited[x][y]) continue;
        if (tiles[x][y] != 0) continue;

        visited[x][y] = true;
        reachable++;

        s.push({x+1, y});
        s.push({x-1, y});
        s.push({x, y+1});
        s.push({x, y-1});
    }
    return reachable;
}
