#include "map/Map.hpp"
#include <stack>
#include <vector>

namespace {
    constexpr int TILE_SIZE_INT = 32;
    constexpr float TILE_SIZE_FLOAT = 32.0f;

    constexpr int TILE_ID_EMPTY = 0;
    constexpr int TILE_ID_SOLID_GROUND = 1;
    constexpr int TILE_ID_LADDER = 2;
    constexpr int TILE_ID_ROPE = 3;
    constexpr int TILE_ID_PLATFORM = 6;

    constexpr int BORDER_OFFSET = 1;
    constexpr float MIN_REACHABLE_SPAWN_PERCENTAGE = 0.8f;
    constexpr float DEFAULT_SPAWN_COORD_COMPONENT_FLOAT = TILE_SIZE_FLOAT;
}

bool Map::collidesWithGround(Vector2 pos) const {
    int tx = static_cast<int>(pos.x / TILE_SIZE_INT);
    int ty = static_cast<int>(pos.y / TILE_SIZE_INT);
    if (tx >= 0 && tx < width && ty >= 0 && ty < height) {
        return tiles[tx][ty] == TILE_ID_SOLID_GROUND || tiles[tx][ty] == TILE_ID_PLATFORM || tiles[tx][ty] == TILE_HIGHLIGHT_DELETE;
    }
    return false;
}

bool Map::isSolidTile(int x, int y) const {
    if (!isInsideBounds(x,y)) return false;
    return tiles[x][y] == TILE_ID_SOLID_GROUND || tiles[x][y] == TILE_ID_PLATFORM || tiles[x][y] == TILE_HIGHLIGHT_DELETE;
}

bool Map::isLadderTile(int x, int y) const {
    if (!isInsideBounds(x,y)) return false;
    return tiles[x][y] == TILE_ID_LADDER;
}

bool Map::isRopeTile(int x, int y) const {
    if (!isInsideBounds(x,y)) return false;
    return tiles[x][y] == TILE_ID_ROPE;
}

bool Map::isTileEmpty(int x, int y) const {
    if (!isInsideBounds(x,y)) return false;
    return tiles[x][y] == TILE_ID_EMPTY || tiles[x][y] == TILE_HIGHLIGHT_CREATE;
}

Vector2 Map::findEmptySpawn() const {
    int totalEmpty = countEmptyTiles();
    if (totalEmpty == 0) {
        return {DEFAULT_SPAWN_COORD_COMPONENT_FLOAT, DEFAULT_SPAWN_COORD_COMPONENT_FLOAT};
    }

    for (int y = BORDER_OFFSET; y < height - BORDER_OFFSET; ++y) {
        for (int x = BORDER_OFFSET; x < width - BORDER_OFFSET; ++x) {
            if (tiles[x][y] == TILE_ID_EMPTY && (y + 1 < height) && 
                (tiles[x][y + 1] == TILE_ID_SOLID_GROUND || tiles[x][y + 1] == TILE_ID_PLATFORM)) {
                int reachable = countReachableEmptyTiles(x, y);
                if (reachable >= static_cast<float>(totalEmpty) * MIN_REACHABLE_SPAWN_PERCENTAGE) {
                    return { static_cast<float>(x) * TILE_SIZE_FLOAT, static_cast<float>(y) * TILE_SIZE_FLOAT };
                }
            }
        }
    }
    return {DEFAULT_SPAWN_COORD_COMPONENT_FLOAT, DEFAULT_SPAWN_COORD_COMPONENT_FLOAT};
}

int Map::countEmptyTiles() const {
    int count = 0;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (tiles[x][y] == TILE_ID_EMPTY) {
                count++;
            }
        }
    }
    return count;
}

int Map::countReachableEmptyTiles(int startX, int startY) const {
    std::vector<std::vector<bool>> visited(width, std::vector<bool>(height, false));
    std::stack<std::pair<int, int>> s;
    
    if (!isInsideBounds(startX, startY) || tiles[startX][startY] != TILE_ID_EMPTY) {
        return 0;
    }

    s.push({startX, startY});
    visited[startX][startY] = true;
    int reachable = 0;

    while (!s.empty()) {
        auto current_tile_coords = s.top(); 
        s.pop();
        int current_x = current_tile_coords.first;
        int current_y = current_tile_coords.second;
        
        reachable++;

        const int dx[] = {0, 0, 1, -1};
        const int dy[] = {1, -1, 0, 0};

        for (int i = 0; i < 4; ++i) {
            int next_x = current_x + dx[i];
            int next_y = current_y + dy[i];

            if (isInsideBounds(next_x, next_y) && !visited[next_x][next_y] && tiles[next_x][next_y] == TILE_ID_EMPTY) {
                visited[next_x][next_y] = true;
                s.push({next_x, next_y});
            }
        }
    }
    return reachable;
}
