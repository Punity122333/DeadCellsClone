#include "Map.hpp"
#include <random>

Map::Map(int w, int h) : width(w), height(h), tiles(w, std::vector<int>(h, 0)) {
    std::random_device rd;
    std::mt19937 gen(rd());

    // Borders
    for (int x = 0; x < width; x++) {
        tiles[x][height - 1] = 1;
        tiles[x][0] = 1;
    }
    for (int y = 0; y < height; y++) {
        tiles[0][y] = 1;
        tiles[width - 1][y] = 1;
    }

    struct Ladder { int x, y1, y2; };
    struct Platform { int startX, endX, y; };
    std::vector<Ladder> allLadders;
    std::vector<Platform> allPlatforms;

    // --- Place ladders first ---
    int laddersToPlace = std::max(401, h / 2); // Place at least 401 ladders
    std::uniform_int_distribution<> ladderXDist(2, width - 3);
    std::uniform_int_distribution<> ladderYDist(3, height - 6);
    std::uniform_int_distribution<> ladderLenDist(3, 6);

    for (int i = 0; i < laddersToPlace; ++i) {
        int x = ladderXDist(gen);
        int y1 = ladderYDist(gen);
        int len = ladderLenDist(gen);
        int y2 = y1 + len;
        if (y2 >= height - 2) y2 = height - 3;
        if (y2 <= y1) continue;
        allLadders.push_back({x, y1, y2});
        for (int y = y1; y <= y2; ++y) {
            if (tiles[x][y] == 0) tiles[x][y] = 2;
        }
    }

    // --- Place platforms connected by ladders ---
    std::uniform_int_distribution<> platLenDist(4, 8);
    for (const Ladder& ladder : allLadders) {
        // Place a platform at the top of the ladder
        int platLenTop = platLenDist(gen);
        int platStartTop = std::max(1, ladder.x - platLenTop / 2);
        int platEndTop = std::min(width - 2, platStartTop + platLenTop - 1);
        platStartTop = std::max(1, platEndTop - platLenTop + 1); // ensure length

        for (int x = platStartTop; x <= platEndTop; ++x) {
            tiles[x][ladder.y1] = 1;
        }
        allPlatforms.push_back({platStartTop, platEndTop, ladder.y1});

        // Place a platform at the bottom of the ladder
        int platLenBot = platLenDist(gen);
        int platStartBot = std::max(1, ladder.x - platLenBot / 2);
        int platEndBot = std::min(width - 2, platStartBot + platLenBot - 1);
        platStartBot = std::max(1, platEndBot - platLenBot + 1);

        for (int x = platStartBot; x <= platEndBot; ++x) {
            tiles[x][ladder.y2] = 1;
        }
        allPlatforms.push_back({platStartBot, platEndBot, ladder.y2});
    }

    // --- Optionally, add a few random platforms not connected to ladders ---
    int extraPlatformCount = (width * height) / 80;
    std::uniform_int_distribution<> platYDist(3, height - 6);
    for (int i = 0; i < extraPlatformCount; ++i) {
        int platLen = platLenDist(gen);
        int platY = platYDist(gen);
        int platStart = ladderXDist(gen) - platLen / 2;
        platStart = std::max(1, std::min(width - platLen - 1, platStart));
        int platEnd = platStart + platLen - 1;

        bool overlaps = false;
        for (int x = platStart; x <= platEnd; ++x) {
            if (tiles[x][platY] != 0) {
                overlaps = true;
                break;
            }
        }
        if (overlaps) continue;

        for (int x = platStart; x <= platEnd; ++x) {
            tiles[x][platY] = 1;
        }
        allPlatforms.push_back({platStart, platEnd, platY});
    }
    
    std::uniform_int_distribution<> wallXDist(3, width - 4);
    std::uniform_int_distribution<> wallYDist(2, height - 4);
    std::uniform_int_distribution<> shortWallHeightDist(1, 1);
    std::uniform_int_distribution<> wallCountDist(4, 8);
    int wallCount = wallCountDist(gen);

    for (int i = 0; i < wallCount; ++i) {
        int x = wallXDist(gen);
        int baseY = wallYDist(gen);
        int wallHeight = shortWallHeightDist(gen);

        for (int y = baseY; y < baseY + wallHeight && y < height - 1; ++y) {
            if (tiles[x][y] == 0) tiles[x][y] = 1;
        }
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

bool Map::isLadderTile(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) return false;
    return tiles[x][y] == 2;
}


int Map::getHeight() const {
    return height;
}