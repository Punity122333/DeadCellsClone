#pragma once
#include "Map.hpp"
#include <raylib.h>

class ScrapHound {
public:
    ScrapHound(Vector2 pos);
    void update(const Map& map, Vector2 playerPos, float dt);
    void draw() const;
    Vector2 getPosition() const;
private:
    Vector2 position;
    Vector2 velocity = {0, 0};
    float speed = 220.0f; // Increased speed
    float gravity = 1000.0f;
    std::vector<Vector2> path; 
    
};