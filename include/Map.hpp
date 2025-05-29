#pragma once
#include <random>
#include <vector>
#include "raylib.h"

class Map {
public:
    Map(int width, int height);
    ~Map();

    Map(const Map&) = delete;
    Map& operator=(const Map&) = delete;
    Map(Map&&) = delete;
    Map& operator=(Map&&) = delete;
    void draw() const;
    bool collidesWithGround(Vector2 pos) const;
    bool isSolidTile(int x, int y) const;
    bool isLadderTile(int x, int y) const;
    bool isRopeTile(int x, int y) const;
    int getHeight() const;
    std::vector<Texture2D> tileTextures;
    Vector2 findEmptySpawn() const;
    bool isTileEmpty(int x, int y) const;
    int countEmptyTiles() const;
    int countReachableEmptyTiles(int startX, int startY) const;

private:
    int width, height;
    std::vector<std::vector<int>> tiles;
    Texture2D tileTexture;
    Texture2D ladderTexture;
    Texture2D ropeTexture;
    struct Ladder;
    struct Rope;
    struct Platform;

    void placeBorders();
    void generateRoomsAndConnections(std::mt19937& gen);
    void generateRoomContent(const struct Room& room, std::mt19937& gen);
};