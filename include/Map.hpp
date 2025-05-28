#pragma once
#include <random>
#include <vector>
#include "raylib.h"

class Map {
public:
    Map(int width, int height);
    void draw() const;
    bool collidesWithGround(Vector2 pos) const;
    bool isSolidTile(int x, int y) const;
    bool isLadderTile(int x, int y) const;
    bool isRopeTile(int x, int y) const;
    int getHeight() const;

private:
    int width, height;
    std::vector<std::vector<int>> tiles;

    struct Ladder;
    struct Rope;
    struct Platform;

    void placeBorders();
    void generateRoomsAndConnections(std::mt19937& gen);
    void generateRoomContent(const struct Room& room, std::mt19937& gen);
};