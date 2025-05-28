#include "Map.hpp"
#include <random>
#include <vector>
#include <algorithm>

struct Map::Ladder { int x, y1, y2; };
struct Map::Rope { int x, y1, y2; };
struct Map::Platform { int startX, endX, y; };
struct Room { int startX, startY, endX, endY; };

Map::Map(int w, int h) : width(w), height(h), tiles(w, std::vector<int>(h, 0)) {
    std::random_device rd;
    std::mt19937 gen(rd());

    placeBorders();
    generateRoomsAndConnections(gen);
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

void Map::generateRoomsAndConnections(std::mt19937& gen) {
    int roomHeight = 16;
    std::uniform_int_distribution<> roomWidthDist(20, 30);

    std::vector<Room> rooms;
    int y = 1;
    while (y + roomHeight < height - 1) {
        int x = 1;
        while (x + 8 < width - 1) {
            int rw = roomWidthDist(gen);
            if (x + rw >= width - 1) rw = width - 2 - x;
            if (rw < 8) break;
            int sx = x;
            int sy = y;
            int ex = x + rw - 1;
            int ey = y + roomHeight - 1;
            rooms.push_back({sx, sy, ex, ey});
            x += rw;
        }
        y += roomHeight;
    }

    for (const Room& room : rooms) {
        generateRoomContent(room, gen);
    }

    for (size_t i = 0; i < rooms.size(); ++i) {
        for (size_t j = 0; j < rooms.size(); ++j) {
            if (rooms[j].startY == rooms[i].startY && rooms[j].startX == rooms[i].endX + 1) {
                int doorYCenter = (rooms[i].startY + rooms[i].endY) / 2;
                for (int dy = -1; dy <= 1; ++dy) {
                    int doorY = doorYCenter + dy;
                    if (doorY > rooms[i].startY && doorY < rooms[i].endY) {
                        tiles[rooms[i].endX][doorY] = 0;
                        tiles[rooms[j].startX][doorY] = 0;
                    }
                }
            }
        }
    }

    std::uniform_int_distribution<> vertConnDist(1, 2);
    for (size_t i = 0; i < rooms.size(); ++i) {
        for (size_t j = i + 1; j < rooms.size(); ++j) {
            if (rooms[j].startY == rooms[i].endY + 1 &&
                !(rooms[j].endX < rooms[i].startX || rooms[j].startX > rooms[i].endX)) {
                int connType = vertConnDist(gen);
                int connX = std::max(rooms[i].startX, rooms[j].startX) +
                            (std::min(rooms[i].endX, rooms[j].endX) - std::max(rooms[i].startX, rooms[j].startX)) / 2;
                int connY1 = rooms[i].endY - 1;
                int connY2 = rooms[j].startY + 5;
                int tileType = (connType == 1) ? 2 : 3;
                for (int y = connY1; y <= connY2; ++y) {
                    tiles[connX][y] = tileType;
                }
            }
        }
    }
}
void Map::generateRoomContent(const Room& room, std::mt19937& gen) {
    for (int x = room.startX; x <= room.endX; ++x) {
        tiles[x][room.startY] = 1;
        tiles[x][room.endY] = 1;
    }
    for (int y = room.startY; y <= room.endY; ++y) {
        tiles[room.startX][y] = 1;
        tiles[room.endX][y] = 1;
    }

    int platY = (room.startY + room.endY) / 2;
    for (int x = room.startX + 2; x <= room.endX - 2; ++x) {
        if (platY >= room.startY + 2 && platY <= room.endY - 2)
            tiles[x][platY] = 1;
    }

    std::uniform_int_distribution<> platCountDist(1, 2);
    int extraPlats = platCountDist(gen);
    int platMinLen = 4;
    int platMaxLen = std::max(platMinLen, (room.endX - room.startX) / 2);
    std::uniform_int_distribution<> platLenDist(platMinLen, platMaxLen);
    std::uniform_int_distribution<> platYDist(room.startY + 2, room.endY - 2);

    for (int i = 0; i < extraPlats; ++i) {
        int platLen = platLenDist(gen);
        int platStartMin = room.startX + 2;
        int platStartMax = room.endX - 2 - platLen + 1;
        if (platStartMax < platStartMin) continue;
        std::uniform_int_distribution<> platStartDist(platStartMin, platStartMax);
        int pxStart = platStartDist(gen);
        int py = platYDist(gen);
        for (int x = pxStart; x < pxStart + platLen && x <= room.endX - 2; ++x) {
            if (py >= room.startY + 2 && py <= room.endY - 2 && tiles[x][py] == 0)
                tiles[x][py] = 1;
        }
    }

    std::uniform_int_distribution<> wallChance(0, 2);
    if (wallChance(gen) == 0) {
        int wxMin = room.startX + 2;
        int wxMax = room.endX - 2;
        if (wxMax >= wxMin) {
            std::uniform_int_distribution<> wxDist(wxMin, wxMax);
            int wx = wxDist(gen);

            std::uniform_int_distribution<> gapSizeDist(2, 3);
            int gapSize = gapSizeDist(gen);

            int gapStartMin = room.startY + 5;
            int gapStartMax = room.endY - 5 - (gapSize - 1);
            if (gapStartMax >= gapStartMin) {
                std::uniform_int_distribution<> gapDist(gapStartMin, gapStartMax);
                int gapStart = gapDist(gen);

                for (int y = room.startY + 4; y <= room.endY - 4; ++y) {
                    bool inGap = (y >= gapStart && y < gapStart + gapSize);
                    if (inGap) continue;
                    if (tiles[wx][y] == 0)
                        tiles[wx][y] = 1;
                }
            }
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
