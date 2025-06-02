#ifndef MAP_HPP
#define MAP_HPP

#include <raylib.h>
#include <vector>

#include <random>


namespace {
    const int TILE_HIGHLIGHT_CREATE = 11;
    const int TILE_HIGHLIGHT_DELETE = 12;

    const float HIGHLIGHT_TIME = 2.0f;
    const float GLITCH_TIME = 0.5f;
    const float BLINK_CYCLE_TIME = 1.0f;
    const float MIN_HIGHLIGHT_OPACITY = 0.1f;
}

// Moved outside Map class
struct Room {
    int startX, startY, endX, endY;
};

struct Ladder {
    int x, y1, y2;
};

struct Rope {
    int x, y1, y2;
};

class Map {
public:
    Map(int w, int h);
    ~Map();

    void placeBorders();
    // Updated parameter type to use the global Room struct
    void generateRoomsAndConnections(std::mt19937& gen);
    void generateRoomContent(const Room& room, std::mt19937& gen);
    void draw() const;
    void applyConwayAutomata();
    void updateTransitions(float dt);

    bool collidesWithGround(Vector2 pos) const;
    bool isSolidTile(int x, int y) const;
    bool isLadderTile(int x, int y) const;
    bool isRopeTile(int x, int y) const;
    bool isTileEmpty(int x, int y) const;

    int getHeight() const;
    Vector2 findEmptySpawn() const;
    int countEmptyTiles() const;
    int countReachableEmptyTiles(int startX, int startY) const;
    static constexpr int CHUNK_SIZE = 16;
    struct Chunk {
        int startX, startY;
        int endX, endY;
    };
    std::vector<Chunk> chunks;

private:
    int width;
    int height;
    std::vector<std::vector<int>> tiles;
    std::vector<std::vector<float>> transitionTimers;
    std::vector<std::vector<bool>> isOriginalSolid;
    std::vector<std::vector<bool>> isConwayProtected;
    std::vector<Texture2D> tileTextures;
};

#endif // MAP_HPP
