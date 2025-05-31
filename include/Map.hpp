// Map.hpp
#pragma once

#include <vector>
#include <raylib.h>
#include <random>

class Map {
public:
    Map(int w, int h);
    ~Map();

    void draw() const;
    void applyConwayAutomata();
    void updateTransitions(float dt);

    bool collidesWithGround(Vector2 pos) const;
    bool isSolidTile(int x, int y) const;
    bool isLadderTile(int x, int y) const;
    bool isRopeTile(int x, int y) const;
    int getHeight() const;

    Vector2 findEmptySpawn() const;

private:
    int width;
    int height;
    std::vector<std::vector<int>> tiles;
    std::vector<Texture2D> tileTextures;
    std::vector<std::vector<float>> transitionTimers;

    std::vector<std::vector<bool>> isOriginalSolid;

    struct Ladder { int x, y1, y2; };
    struct Rope { int x, y1, y2; };
    struct Room { int startX, startY, endX, endY; };
    struct Hallway { int startX, startY, endX, endY; };

    void placeBorders();
    void generateRoomsAndConnections(std::mt19937& gen);
    void generateRoomContent(const Room& room, std::mt19937& gen);

    bool isTileEmpty(int x, int y) const;
    int countEmptyTiles() const;
    int countReachableEmptyTiles(int startX, int startY) const;
};
