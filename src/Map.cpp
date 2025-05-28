#include "Map.hpp"
#include <random>

struct Map::Ladder { int x, y1, y2; };
struct Map::Rope { int x, y1, y2; };
struct Map::Platform { int startX, endX, y; };

Map::Map(int w, int h) : width(w), height(h), tiles(w, std::vector<int>(h, 0)) {
    std::random_device rd;
    std::mt19937 gen(rd());

    std::vector<Ladder> allLadders;
    std::vector<Rope> allRopes;
    std::vector<Platform> allPlatforms;

    placeBorders();
    placeLadders(allLadders, gen);
    placeRopes(allLadders, allRopes, gen);
    connectAllPlatforms(allLadders, allRopes, allPlatforms, gen);
    placeExtraPlatforms(allLadders, allPlatforms, gen);
    placeWalls(gen);
}

void Map::placeBorders() {
    for (int x = 0; x < width; x++) {
        tiles[x][height - 1] = 1;
        tiles[x][0] = 1;
    }
    for (int y = 0; y < height; y++) {
        tiles[0][y] = 1;
        tiles[width - 1][y] = 1;
    }
}

void Map::placeLadders(std::vector<Ladder>& allLadders, std::mt19937& gen) {
    int laddersToPlace = std::max(40, height / 2);
    std::uniform_int_distribution<> ladderXDist(2, width - 3);
    std::uniform_int_distribution<> ladderYDist(3, height - 6);
    std::uniform_int_distribution<> ladderLenDist(14, 25);
    const int minHorizontalSpacing = 4;

    for (int i = 0; i < laddersToPlace; ++i) {
        int x = ladderXDist(gen);
        int y1 = ladderYDist(gen);
        int len = ladderLenDist(gen);
        int y2 = y1 + len;
        if (y2 >= height - 2) y2 = height - 3;
        if (y2 <= y1) continue;

        bool overlaps = false;
        for (const Ladder& ladder : allLadders) {
            if (std::abs(ladder.x - x) < minHorizontalSpacing) {
                overlaps = true;
                break;
            }
        }
        if (overlaps) continue;

        allLadders.push_back({x, y1, y2});
        for (int y = y1; y <= y2; ++y) {
            if (tiles[x][y] == 0) tiles[x][y] = 2;
        }
    }
}

void Map::placeRopes(const std::vector<Ladder>& allLadders, std::vector<Rope>& allRopes, std::mt19937& gen) {
    int ropesToPlace = std::max(40, height / 10);
    std::uniform_int_distribution<> ropeXDist(2, width - 3);
    std::uniform_int_distribution<> ropeYDist(3, height - 6);
    std::uniform_int_distribution<> ropeLenDist(8, 14);
    const int minHorizontalSpacing = 4;

    for (int i = 0; i < ropesToPlace; ++i) {
        int x = ropeXDist(gen);
        int y1 = ropeYDist(gen);
        int len = ropeLenDist(gen);
        int y2 = y1 + len;
        if (y2 >= height - 2) y2 = height - 3;
        if (y2 <= y1) continue;

        bool overlaps = false;
        for (const Ladder& ladder : allLadders) {
            if (std::abs(ladder.x - x) < minHorizontalSpacing) {
                overlaps = true;
                break;
            }
        }
        if (!overlaps) {
            for (const Rope& rope : allRopes) {
                if (std::abs(rope.x - x) < minHorizontalSpacing) {
                    overlaps = true;
                    break;
                }
            }
        }
        if (overlaps) continue;

        allRopes.push_back({x, y1, y2});
        for (int y = y1; y <= y2; ++y) {
            if (tiles[x][y] == 0) tiles[x][y] = 3;
        }
    }
}

void Map::connectPlatforms(int x, int y1, int y2, std::vector<Platform>& allPlatforms, std::mt19937& gen, std::uniform_int_distribution<>& platLenDist) {
    int platLenTop = platLenDist(gen);
    int platStartTop = std::max(1, x - platLenTop / 2);
    int platEndTop = std::min(width - 2, platStartTop + platLenTop - 1);
    platStartTop = std::max(1, platEndTop - platLenTop + 1);

    int platformY = std::min(height - 2, y1 + 1);

    if (x > platStartTop) {
        for (int px = platStartTop; px < x; ++px) {
            tiles[px][platformY] = 1;
        }
        if (x - 1 >= platStartTop)
            allPlatforms.push_back({platStartTop, x - 1, platformY});
    }
    if (x < platEndTop) {
        for (int px = x + 1; px <= platEndTop; ++px) {
            tiles[px][platformY] = 1;
        }
        if (x + 1 <= platEndTop)
            allPlatforms.push_back({x + 1, platEndTop, platformY});
    }

    int platLenBot = platLenDist(gen);
    int platStartBot = std::max(1, x - platLenBot / 2);
    int platEndBot = std::min(width - 2, platStartBot + platLenBot - 1);
    platStartBot = std::max(1, platEndBot - platLenBot + 1);

    for (int px = platStartBot; px <= platEndBot; ++px) {
        tiles[px][y2] = 1;
    }
    allPlatforms.push_back({platStartBot, platEndBot, y2});
}

void Map::connectAllPlatforms(const std::vector<Ladder>& allLadders, const std::vector<Rope>& allRopes, std::vector<Platform>& allPlatforms, std::mt19937& gen) {
    std::uniform_int_distribution<> platLenDist(4, 8);
    for (const Ladder& ladder : allLadders) {
        connectPlatforms(ladder.x, ladder.y1, ladder.y2, allPlatforms, gen, platLenDist);
    }
    for (const Rope& rope : allRopes) {
        connectPlatforms(rope.x, rope.y1, rope.y2, allPlatforms, gen, platLenDist);
    }
}

void Map::placeExtraPlatforms(const std::vector<Ladder>& allLadders, std::vector<Platform>& allPlatforms, std::mt19937& gen) {
    std::uniform_int_distribution<> platLenDist(4, 8);
    std::uniform_int_distribution<> platYDist(3, height - 6);
    std::uniform_int_distribution<> ladderXDist(2, width - 3);
    int extraPlatformCount = (width * height) / 80;
    const int playerHeightTiles = 3;
    const int minVerticalSpace = playerHeightTiles * 4;

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
            for (int y = platY + 1; y <= platY + playerHeightTiles && y < height - 1; ++y) {
                if (tiles[x][y] != 0) {
                    overlaps = true;
                    break;
                }
            }
            if (overlaps) break;
        }
        if (!overlaps) {
            for (const Ladder& ladder : allLadders) {
                if (ladder.x >= platStart && ladder.x <= platEnd && platY >= ladder.y1 && platY <= ladder.y2) {
                    overlaps = true;
                    break;
                }
            }
        }
        if (!overlaps) {
            for (const Platform& plat : allPlatforms) {
                if (!(platEnd < plat.startX || platStart > plat.endX)) {
                    if (std::abs(platY - plat.y) < minVerticalSpace) {
                        overlaps = true;
                        break;
                    }
                }
            }
        }
        if (overlaps) continue;

        for (int x = platStart; x <= platEnd; ++x) {
            tiles[x][platY] = 1;
        }
        allPlatforms.push_back({platStart, platEnd, platY});
    }
}

void Map::placeWalls(std::mt19937& gen) {
    std::uniform_int_distribution<> wallXDist(3, width - 4);
    std::uniform_int_distribution<> wallYDist(2, height - 4);
    std::uniform_int_distribution<> tallWallHeightDist(4, 20);

    // Dramatically increase the number of walls
    int wallCount = std::max(100, (width * height) / 250);

    for (int i = 0; i < wallCount; ++i) {
        int x = wallXDist(gen);
        int baseY = wallYDist(gen);
        int wallHeight = tallWallHeightDist(gen);

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
            else if (tiles[x][y] == 3)
                DrawRectangle(x * 32 + 14, y * 32, 4, 32, SKYBLUE);
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

bool Map::isRopeTile(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) return false;
    return tiles[x][y] == 3;
}

int Map::getHeight() const {
    return height;
}
