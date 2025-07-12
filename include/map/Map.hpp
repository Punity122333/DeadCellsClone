#ifndef MAP_HPP
#define MAP_HPP

#include <raylib.h>
#include <vector>
#include <random>
#include <mutex>
#include <functional>

// Forward declaration
class Player;

using ProgressCallback = std::function<void(float)>;

namespace MapConstants {
    constexpr int TILE_HIGHLIGHT_CREATE = 11;
    constexpr int TILE_HIGHLIGHT_DELETE = 12;
    constexpr float HIGHLIGHT_TIME = 2.0f;
    constexpr float GLITCH_TIME = 0.5f;
    constexpr float BLINK_CYCLE_TIME = 1.0f;
    constexpr float MIN_HIGHLIGHT_OPACITY = 0.1f;
    constexpr int ROOM_PLACEMENT_SKIP_CHANCE_PERCENT = 2;
    constexpr int HALLWAY_CREATION_SKIP_PERCENT = 20;
    constexpr int MIN_ROOM_SLOT_WIDTH_CONST = 25;
    constexpr int MIN_ROOM_SLOT_HEIGHT_CONST = 20;
    constexpr int SLOT_GAP_SIZE_CONST = 5;
    constexpr int HORIZONTAL_DOOR_HEIGHT_CONST = 6;
    constexpr int VERTICAL_HALLWAY_WIDTH_CONST = 6;
    constexpr int DEFAULT_TILE_VALUE = 1;
    constexpr int EMPTY_TILE_VALUE = 0;
    constexpr int PLATFORM_TILE_VALUE = 6;
    constexpr int WALL_TILE_VALUE = 1;
    constexpr int LADDER_TILE_VALUE = 2;
    constexpr int ROPE_TILE_VALUE = 3;
    constexpr int TREASURE_TILE_VALUE = 4;
    constexpr int SHOP_TILE_VALUE = 5;
    constexpr int CHEST_TILE_VALUE = 7;
    constexpr int WALL_PLACEMENT_CHANCE_MAX_ROLL = 3;
    constexpr int MIN_WALL_VERTICAL_GAP_SIZE = 3;
    constexpr int MAX_WALL_VERTICAL_GAP_SIZE = 5;
    constexpr int MIN_RANDOM_PLATFORMS_IN_ROOM = 1;
    constexpr int MAX_RANDOM_PLATFORMS_IN_ROOM = 2;
    constexpr int MIN_RANDOM_PLATFORM_LENGTH = 4;
    constexpr int MIN_GENERATED_LADDER_LENGTH = 8;
    constexpr int MAX_GENERATED_LADDER_LENGTH = 15;
    constexpr int GENERATED_LADDER_EXCLUSION_ZONE = 1;
    constexpr int MIN_LADDERS_PER_ROOM_SHAFT_AREA = 1;
    constexpr int MAX_LADDERS_PER_ROOM_SHAFT_AREA = 2;
    constexpr int LADDER_OR_ROPE_ROLL_MAX = 1;
    constexpr int MAX_EXTRA_TREASURES_IN_ROOM = 2;
    constexpr int MAX_EXTRA_SHOP_ITEMS_IN_ROOM = 2;
    constexpr int LARGE_HALL_CREATION_CHANCE_PERCENT = 15;
    constexpr int MAX_ROOM_WIDTH_RANDOM_VARIATION = 5;
    constexpr int ROOM_TYPE_TREASURE_CHANCE_THRESHOLD_PERCENT = 25;
    constexpr int ROOM_TYPE_SHOP_CHANCE_THRESHOLD_PERCENT = 40;
    constexpr int TILE_TEMP_CREATE_A = 101;
    constexpr int TILE_TEMP_DELETE = 102;
    constexpr int TILE_TEMP_CREATE_B = 103;
    constexpr int MIN_CONWAY_CHUNK_SIZE_X = 6;
    constexpr int MAX_CONWAY_CHUNK_SIZE_X = 12;
    constexpr int MIN_CONWAY_CHUNK_SIZE_Y = 1;
    constexpr int MAX_CONWAY_CHUNK_SIZE_Y = 2;
    constexpr int CHUNK_ALIVE_ROLL_MAX = 9;
    constexpr int CHUNK_ALIVE_SUCCESS_ROLL = 0;
    constexpr int FLOOR_TILE_VALUE = 13;
    constexpr int PROTECTED_EMPTY_TILE_VALUE = 14;
    constexpr int LAVA_TILE_VALUE = 15;
    constexpr int LAVA_POCKET_CHANCE_PERCENT = 20;
    constexpr float LAVA_DAMAGE_PER_SECOND = 25.0f;
    constexpr float LAVA_DAMAGE_INTERVAL = 0.5f;

    int rollPercent(std::mt19937& gen);
}

struct TileParticle {
    Vector2 position;
    Vector2 velocity;
    Color color;
    float life;
    float maxLife;
    float size;
    
    TileParticle(Vector2 pos, Vector2 vel, Color col, float lifetime, float sz)
        : position(pos), velocity(vel), color(col), life(lifetime), maxLife(lifetime), size(sz) {}
};

struct LavaCell {
    float mass;
    float flow;
    bool settled;
    
    LavaCell() : mass(0.0f), flow(0.0f), settled(false) {}
    LavaCell(float m) : mass(m), flow(0.0f), settled(false) {}
};

struct FluidVertex {
    Vector2 position;
    Color color;
    
    FluidVertex(Vector2 pos, Color col) : position(pos), color(col) {}
};

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

class RoomContentGenerator;
class RoomGenerator;
class RoomGridGenerator;
class RoomConnectionGenerator;
class LadderRopePlacer;

class Map {
    friend class RoomContentGenerator;
    friend class RoomGenerator;
    friend class RoomGridGenerator;
    friend class RoomConnectionGenerator;
    friend class LadderRopePlacer;
    
public:
    Map(int w, int h, const std::vector<Texture2D>& loadedTileTextures, ProgressCallback progressCallback = nullptr);
    ~Map();

    void placeBorders();
    
    void generateRoomsAndConnections(std::mt19937& gen);
    void generateRoomContent(const Room& room, std::mt19937& gen);
    void generateTreasureRoomContent(const Room& room, std::mt19937& gen);
    void generateShopRoomContent(const Room& room, std::mt19937& gen);    
    void draw(const Camera2D& camera) const;
    void drawLavaFluid(const Camera2D& camera) const;
    void applyConwayAutomata();
    void updateTransitions(float dt);
    void updateLavaFlow(float dt);
    void updateParticles(float dt, Vector2 playerPosition = {0, 0});
    void createPopEffect(Vector2 position);
    void createSuctionEffect(Vector2 position);
    bool isLavaTile(int x, int y) const;
    bool checkPlayerLavaContact(Vector2 playerPos, float playerWidth, float playerHeight) const;

    bool isInsideBounds(int x, int y) const;

    bool collidesWithGround(Vector2 pos) const;
    bool isSolidTile(int x, int y) const;
    bool isLadderTile(int x, int y) const;
    bool isRopeTile(int x, int y) const;
    bool isTileEmpty(int x, int y) const;
    int getTileValue(int x, int y) const;

    int getHeight() const;
    int getWidth() const;
    Vector2 findEmptySpawn() const;
    int countEmptyTiles() const;
    int countReachableEmptyTiles(int startX, int startY) const;
    int estimateReachabilityFast(int startX, int startY) const;
    void setTileValue(int x, int y, int value);
    static constexpr int CHUNK_SIZE = 16;
    struct Chunk {
        int startX, startY;
        int endX, endY;
    };
    std::vector<Chunk> chunks;

    const std::vector<Room>& getGeneratedRooms() const;
    
    // Player reference methods
    void setPlayer(Player* player) { playerRef = player; }
    Player* getPlayer() const { return playerRef; }

private:
    int width;
    int height;
    Player* playerRef = nullptr; // Reference to player
    std::vector<std::vector<int>> tiles;
    std::vector<std::vector<LavaCell>> lavaGrid;
    std::vector<std::vector<float>> transitionTimers;
    std::vector<std::vector<int>> cooldownMap;
    std::vector<std::vector<bool>> isOriginalSolid;
    std::vector<std::vector<bool>> isConwayProtected;
    std::vector<Texture2D> tileTextures;
    std::vector<Room> generatedRooms;
    std::vector<TileParticle> particles;
    mutable std::mutex particlesMutex;
    
    // Lava simulation constants
    static constexpr float LAVA_FLOW_RATE = 0.8f;
    static constexpr float LAVA_MIN_FLOW = 0.01f;
    static constexpr float LAVA_MAX_MASS = 1.0f;
    static constexpr float LAVA_MIN_MASS = 0.01f;
};

#endif
