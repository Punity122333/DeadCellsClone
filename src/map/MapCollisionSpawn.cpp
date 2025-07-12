#include "map/Map.hpp"
#include <stack>
#include <vector>
#include <cstdio>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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
        // Lava tiles don't block movement but will be handled by damage system
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

bool Map::isLavaTile(int x, int y) const {
    if (!isInsideBounds(x, y)) return false;
    return tiles[x][y] == LAVA_TILE_VALUE;
}

bool Map::checkPlayerLavaContact(Vector2 playerPos, float playerWidth, float playerHeight) const {
    // Check the player's hitbox area for lava contact
    int leftTile = static_cast<int>(playerPos.x / 32);
    int rightTile = static_cast<int>((playerPos.x + playerWidth) / 32);
    int topTile = static_cast<int>(playerPos.y / 32);
    int bottomTile = static_cast<int>((playerPos.y + playerHeight) / 32);
    
    for (int x = leftTile; x <= rightTile; ++x) {
        for (int y = topTile; y <= bottomTile; ++y) {
            if (isLavaTile(x, y) && lavaGrid[x][y].mass > LAVA_MIN_MASS) {
                return true;
            }
        }
    }
    return false;
}

int Map::getTileValue(int x, int y) const {
    if (!isInsideBounds(x, y)) return WALL_TILE_VALUE;
    return tiles[x][y];
}

Vector2 Map::findEmptySpawn() const {
    printf("[Map] Smart spawn finding initiated...\n");

    const int totalEmptyTiles = countEmptyTiles();
    const int targetReachability = static_cast<int>(totalEmptyTiles * MIN_REACHABLE_SPAWN_PERCENTAGE);
    
    printf("[Map] Total empty tiles: %d, Target reachability: %d\n", totalEmptyTiles, targetReachability);

    std::vector<std::pair<int, int>> candidates;
    const int SAMPLE_STEP = 8; 

    for (int y = BORDER_OFFSET; y < height - BORDER_OFFSET - 1; y += SAMPLE_STEP) {
        for (int x = BORDER_OFFSET; x < width - BORDER_OFFSET; x += SAMPLE_STEP) {
            if (tiles[x][y] == EMPTY_TILE_VALUE && 
                y + 1 < height && isSolidTile(x, y + 1)) {
                candidates.push_back({x, y});
            }
        }
    }
    printf("[Map] Found %zu ground-based candidates with fast sampling\n", candidates.size());

    std::vector<std::pair<int, std::pair<int, int>>> candidatesWithScore;
    
    for (const auto& candidate : candidates) {
        int x = candidate.first;
        int y = candidate.second;
        int fastScore = estimateReachabilityFast(x, y);
        candidatesWithScore.push_back({fastScore, {x, y}});
    }

    std::sort(candidatesWithScore.begin(), candidatesWithScore.end(), 
              [](const auto& a, const auto& b) { return a.first > b.first; });
    
    printf("[Map] Pre-screened candidates, testing top performers thoroughly\n");
    
    int bestReachability = 0;
    Vector2 bestSpawn = {DEFAULT_SPAWN_COORD_COMPONENT_FLOAT, DEFAULT_SPAWN_COORD_COMPONENT_FLOAT};

    const size_t MAX_CANDIDATES_TO_TEST = std::min(candidatesWithScore.size(), size_t(10));
    
    for (size_t i = 0; i < MAX_CANDIDATES_TO_TEST; ++i) {
        int x = candidatesWithScore[i].second.first;
        int y = candidatesWithScore[i].second.second;
        
        int reachability = countReachableEmptyTiles(x, y);
        
        if (reachability > bestReachability) {
            bestReachability = reachability;
            bestSpawn = {static_cast<float>(x) * TILE_SIZE_FLOAT, static_cast<float>(y) * TILE_SIZE_FLOAT};

            if (reachability >= targetReachability) {
                printf("[Map] Excellent spawn found at (%.1f, %.1f) with %d/%d reachability\n", 
                       bestSpawn.x, bestSpawn.y, reachability, totalEmptyTiles);
                return bestSpawn;
            }
        }
    }
  
    if (bestReachability > totalEmptyTiles / 4) { 
        printf("[Map] Good spawn found at (%.1f, %.1f) with %d/%d reachability\n", 
               bestSpawn.x, bestSpawn.y, bestReachability, totalEmptyTiles);
        return bestSpawn;
    }

    printf("[Map] No good ground spawn found, searching center area with finer sampling\n");
    
    int centerX = width / 2;
    int centerY = height / 2;
    int searchRadius = std::min(width, height) / 4;
    
    for (int radius = 1; radius <= searchRadius; radius += 2) {
        for (int angle = 0; angle < 360; angle += 30) {
            int x = centerX + static_cast<int>(radius * cos(angle * M_PI / 180.0));
            int y = centerY + static_cast<int>(radius * sin(angle * M_PI / 180.0));
            
            if (isInsideBounds(x, y) && tiles[x][y] == EMPTY_TILE_VALUE) {
                int reachability = countReachableEmptyTiles(x, y);
                if (reachability > bestReachability) {
                    bestReachability = reachability;
                    bestSpawn = {static_cast<float>(x) * TILE_SIZE_FLOAT, static_cast<float>(y) * TILE_SIZE_FLOAT};
                }

                if (reachability >= targetReachability / 2) {
                    printf("[Map] Center spawn found at (%.1f, %.1f) with %d/%d reachability\n", 
                           bestSpawn.x, bestSpawn.y, reachability, totalEmptyTiles);
                    return bestSpawn;
                }
            }
        }
    }

    if (bestReachability == 0) {
        printf("[Map] Last resort: finding any empty tile\n");
        for (int y = BORDER_OFFSET; y < height - BORDER_OFFSET; y += 4) {
            for (int x = BORDER_OFFSET; x < width - BORDER_OFFSET; x += 4) {
                if (tiles[x][y] == EMPTY_TILE_VALUE) {
                    bestSpawn = {static_cast<float>(x) * TILE_SIZE_FLOAT, static_cast<float>(y) * TILE_SIZE_FLOAT};
                    printf("[Map] Fallback spawn at (%.1f, %.1f)\n", bestSpawn.x, bestSpawn.y);
                    return bestSpawn;
                }
            }
        }
    }
    
    printf("[Map] Final spawn selected at (%.1f, %.1f) with %d/%d reachability\n", 
           bestSpawn.x, bestSpawn.y, bestReachability, totalEmptyTiles);
    return bestSpawn;
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

    if (!isInsideBounds(startX, startY) || tiles[startX][startY] != EMPTY_TILE_VALUE) {
        return 0;
    }

    std::vector<std::vector<bool>> visited(width, std::vector<bool>(height, false));
    std::stack<std::pair<int, int>> s;

    s.push({startX, startY});
    visited[startX][startY] = true;
    int reachable = 0;

    static const int dx[] = {0, 0, 1, -1, 1, -1, 1, -1}; 
    static const int dy[] = {1, -1, 0, 0, 1, -1, -1, 1};

    while (!s.empty()) {
        auto [current_x, current_y] = s.top();
        s.pop();
        
        reachable++;

        if (reachable > 10000) { 
            return reachable;
        }

        
        for (int i = 0; i < 8; ++i) {
            int next_x = current_x + dx[i];
            int next_y = current_y + dy[i];

            if (isInsideBounds(next_x, next_y) && !visited[next_x][next_y]) {
                
                if (tiles[next_x][next_y] == EMPTY_TILE_VALUE || 
                    tiles[next_x][next_y] == PLATFORM_TILE_VALUE ||
                    tiles[next_x][next_y] == LAVA_TILE_VALUE) { // Lava is traversable but dangerous
                    visited[next_x][next_y] = true;
                    s.push({next_x, next_y});
                }
            }
        }
    }
    
    return reachable;
}

int Map::estimateReachabilityFast(int startX, int startY) const {
    
    if (!isInsideBounds(startX, startY) || tiles[startX][startY] != EMPTY_TILE_VALUE) {
        return 0;
    }
    
    int estimate = 0;
    const int SAMPLE_RADIUS = 20; 
    
    
    for (int dy = -SAMPLE_RADIUS; dy <= SAMPLE_RADIUS; dy += 2) {
        for (int dx = -SAMPLE_RADIUS; dx <= SAMPLE_RADIUS; dx += 2) {
            int x = startX + dx;
            int y = startY + dy;
            
            if (isInsideBounds(x, y) && 
                (tiles[x][y] == EMPTY_TILE_VALUE || 
                 tiles[x][y] == PLATFORM_TILE_VALUE ||
                 tiles[x][y] == LAVA_TILE_VALUE)) { // Include lava in reachability
                estimate++;
            }
        }
    }
    
    
    float localDensity = static_cast<float>(estimate) / (static_cast<float>(SAMPLE_RADIUS * 2 * SAMPLE_RADIUS * 2) / 4.0f);
    int totalEstimate = static_cast<int>(localDensity * countEmptyTiles());
    
    return std::min(totalEstimate, countEmptyTiles()); 
}
