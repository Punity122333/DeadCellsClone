#pragma once
#include <random>
#include <vector>
#include "raylib.h"

class Map {
public:
    Map(int width, int height);
    void draw() const;
    bool collidesWithGround(Vector2 pos) const;
    // Map.hpp
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
    void placeLadders(std::vector<Ladder>&, std::mt19937&);
    void placeRopes(const std::vector<Ladder>&, std::vector<Rope>&, std::mt19937&);
    void connectPlatforms(int, int, int, std::vector<Platform>&, std::mt19937&, std::uniform_int_distribution<>&);
    void connectAllPlatforms(const std::vector<Ladder>&, const std::vector<Rope>&, std::vector<Platform>&, std::mt19937&);
    void placeExtraPlatforms(const std::vector<Ladder>&, std::vector<Platform>&, std::mt19937&);
    void placeWalls(std::mt19937&);
};
