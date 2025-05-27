#pragma once
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
    int getHeight() const;

private:
    int width, height;
    std::vector<std::vector<int>> tiles;
};
