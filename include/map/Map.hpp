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


struct Room {
    int startX, startY, endX, endY;
    enum Type { NORMAL, TREASURE, SHOP }; 
    Type type; 

    Room(int sx, int sy, int ex, int ey, Type t = NORMAL) 
        : startX(sx), startY(sy), endX(ex), endY(ey), type(t) {}
};

struct Ladder {
    int x, y1, y2;

    Ladder(int ladderX, int ladderY1, int ladderY2)
        : x(ladderX), y1(ladderY1), y2(ladderY2) {}
};

struct Rope {
    int x, y1, y2;

    Rope(int ropeX, int ropeY1, int ropeY2)
        : x(ropeX), y1(ropeY1), y2(ropeY2) {}
};

class Map {
public:
    Map(int w, int h);
    ~Map();

    void placeBorders();
    
    void generateRoomsAndConnections(std::mt19937& gen);
    void generateRoomContent(const Room& room, std::mt19937& gen);
    void generateTreasureRoomContent(const Room& room, std::mt19937& gen);
    void generateShopRoomContent(const Room& room, std::mt19937& gen);    
    void draw(const Camera2D& camera) const;
    void applyConwayAutomata();
    void updateTransitions(float dt);

    bool isInsideBounds(int x, int y) const;

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

    const std::vector<Room>& getGeneratedRooms() const;

private:
    int width;
    int height;
    std::vector<std::vector<int>> tiles;
    std::vector<std::vector<float>> transitionTimers;
    std::vector<std::vector<bool>> isOriginalSolid;
    std::vector<std::vector<bool>> isConwayProtected;
    std::vector<Texture2D> tileTextures;
    std::vector<Room> generatedRooms;
};

#endif // MAP_HPP
