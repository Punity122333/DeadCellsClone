#include "map/Map.hpp"
#include <stack>
#include <vector>
#include <cstdio>

using namespace MapConstants;

constexpr int TILE_SIZE_INT = 32;
constexpr float TILE_SIZE_FLOAT = 32.0f;
constexpr int BORDER_OFFSET = 1;
constexpr float MIN_REACHABLE_SPAWN_PERCENTAGE = 0.8f;
constexpr float DEFAULT_SPAWN_COORD_COMPONENT_FLOAT = TILE_SIZE_FLOAT;

bool Map::collidesWithGround(Vector2 pos) const {
    int tx = static_cast<int>(pos.x / TILE_SIZE_INT);
    int ty = static_cast<int>(pos.y / TILE_SIZE_INT);
    if (tx >= 0 && tx < width && ty >= 0 && ty < height) {
        return tiles[tx][ty] == WALL_TILE_VALUE || tiles[tx][ty] == PLATFORM_TILE_VALUE || tiles[tx][ty] == TILE_HIGHLIGHT_DELETE;
    }
    return false;
}

bool Map::isSolidTile(int x, int y) const {
    if (!isInsideBounds(x,y)) return true;
    return tiles[x][y] == WALL_TILE_VALUE || tiles[x][y] == PLATFORM_TILE_VALUE || tiles[x][y] == TILE_HIGHLIGHT_DELETE;
}

bool Map::isLadderTile(int x, int y) const {
    if (!isInsideBounds(x,y)) return false; 
    return tiles[x][y] == LADDER_TILE_VALUE;
}

bool Map::isRopeTile(int x, int y) const {
    if (!isInsideBounds(x,y)) return false;
    return tiles[x][y] == ROPE_TILE_VALUE;
}

bool Map::isTileEmpty(int x, int y) const {
    if (!isInsideBounds(x,y)) return false;
    return tiles[x][y] == EMPTY_TILE_VALUE || tiles[x][y] == TILE_HIGHLIGHT_CREATE;
}

Vector2 Map::findEmptySpawn() const {
    printf("[Map] Finding spawn position...\n");
    printf("[Map] Map dimensions: %dx%d\n", width, height);

    int emptyCount = 0;
    int solidCount = 0;
    int protectedEmptyCount = 0;
    int otherCount = 0;
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (tiles[x][y] == EMPTY_TILE_VALUE) {
                emptyCount++;
            } else if (tiles[x][y] == PROTECTED_EMPTY_TILE_VALUE) {
                protectedEmptyCount++;
            } else if (isSolidTile(x, y)) {
                solidCount++;
            } else {
                otherCount++;
            }
        }
    }
    
    printf("[Map] Tile counts - Empty: %d, Protected Empty: %d, Solid: %d, Other: %d\n", 
           emptyCount, protectedEmptyCount, solidCount, otherCount);
    
    int searchAreaX = width - 2 * BORDER_OFFSET;
    int searchAreaY = height - 2 * BORDER_OFFSET - 1;
    printf("[Map] Search area: %dx%d (excluding borders)\n", searchAreaX, searchAreaY);
    
    int checkedTiles = 0;
    int validEmptyTiles = 0;
    
    for (int y = BORDER_OFFSET; y < height - BORDER_OFFSET - 1; ++y) {
        for (int x = BORDER_OFFSET; x < width - BORDER_OFFSET; ++x) {
            checkedTiles++;
            
            if (tiles[x][y] == EMPTY_TILE_VALUE) {
                validEmptyTiles++;
                
                if (y + 1 < height && isSolidTile(x, y + 1)) {
                    Vector2 spawn = { static_cast<float>(x) * TILE_SIZE_FLOAT, static_cast<float>(y) * TILE_SIZE_FLOAT };
                    printf("[Map] Found spawn position: (%.1f, %.1f) after checking %d tiles\n", spawn.x, spawn.y, checkedTiles);
                    return spawn;
                }
            }
            
            if (checkedTiles % 10000 == 0) {
                printf("[Map] Checked %d tiles, found %d empty tiles so far...\n", checkedTiles, validEmptyTiles);
            }
        }
    }
    
    printf("[Map] First pass complete - checked %d tiles, found %d empty tiles\n", checkedTiles, validEmptyTiles);
    
    printf("[Map] No ground-based spawn found, searching for any empty tile\n");
    checkedTiles = 0;
    validEmptyTiles = 0;
    
    for (int y = BORDER_OFFSET; y < height - BORDER_OFFSET; ++y) {
        for (int x = BORDER_OFFSET; x < width - BORDER_OFFSET; ++x) {
            checkedTiles++;
            
            if (tiles[x][y] == EMPTY_TILE_VALUE) {
                validEmptyTiles++;
                Vector2 spawn = { static_cast<float>(x) * TILE_SIZE_FLOAT, static_cast<float>(y) * TILE_SIZE_FLOAT };
                printf("[Map] Fallback spawn position: (%.1f, %.1f) after checking %d tiles\n", spawn.x, spawn.y, checkedTiles);
                return spawn;
            }

            if (checkedTiles % 10000 == 0) {
                printf("[Map] Fallback search - checked %d tiles, found %d empty tiles so far...\n", checkedTiles, validEmptyTiles);
            }
        }
    }
    
    printf("[Map] Second pass complete - checked %d tiles, found %d empty tiles\n", checkedTiles, validEmptyTiles);

    printf("[Map] No empty tiles found, trying protected empty tiles\n");
    checkedTiles = 0;
    
    for (int y = BORDER_OFFSET; y < height - BORDER_OFFSET; ++y) {
        for (int x = BORDER_OFFSET; x < width - BORDER_OFFSET; ++x) {
            checkedTiles++;
            
            if (tiles[x][y] == PROTECTED_EMPTY_TILE_VALUE) {
                Vector2 spawn = { static_cast<float>(x) * TILE_SIZE_FLOAT, static_cast<float>(y) * TILE_SIZE_FLOAT };
                printf("[Map] Protected empty spawn position: (%.1f, %.1f) after checking %d tiles\n", spawn.x, spawn.y, checkedTiles);
                return spawn;
            }

            if (checkedTiles % 10000 == 0) {
                printf("[Map] Protected empty search - checked %d tiles so far...\n", checkedTiles);
            }
        }
    }
    
    printf("[Map] No valid spawn found anywhere, using default position\n");
    return {DEFAULT_SPAWN_COORD_COMPONENT_FLOAT, DEFAULT_SPAWN_COORD_COMPONENT_FLOAT};
}

int Map::countEmptyTiles() const {
    int count = 0;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (tiles[x][y] == EMPTY_TILE_VALUE) {
                count++;
            }
        }
    }
    return count;
}

int Map::countReachableEmptyTiles(int startX, int startY) const {
    std::vector<std::vector<bool>> visited(width, std::vector<bool>(height, false));
    std::stack<std::pair<int, int>> s;
    
    if (!isInsideBounds(startX, startY) || tiles[startX][startY] != EMPTY_TILE_VALUE) {
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

            if (isInsideBounds(next_x, next_y) && !visited[next_x][next_y] && tiles[next_x][next_y] == EMPTY_TILE_VALUE) {
                visited[next_x][next_y] = true;
                s.push({next_x, next_y});
            }
        }
    }
    return reachable;
}
