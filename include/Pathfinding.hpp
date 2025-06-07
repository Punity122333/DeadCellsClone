#pragma once

#include <vector>
#include <raylib.h>


class Map;


struct Node {
    int x, y;
    float g, f;
    Node* parent;
    Node(int x, int y, float g, float f, Node* parent)
        : x(x), y(y), g(g), f(f), parent(parent) {}
};


std::vector<Vector2> FindPathAStar(const Map& map, Vector2 start, Vector2 goal);