#ifndef ROOM_CONNECTION_GENERATOR_HPP
#define ROOM_CONNECTION_GENERATOR_HPP

#include "map/Map.hpp"
#include <random>
#include <vector>
#include <memory>
#include <limits>

// Forward declarations for optimization data structures
struct KDNode;
class UnionFind;

// Span-based drawing for cache efficiency
struct DrawSpan {
    int x, y, length;
    int tileType;
    bool isHorizontal;
    
    DrawSpan(int x, int y, int len, int type, bool horizontal)
        : x(x), y(y), length(len), tileType(type), isHorizontal(horizontal) {}
};

struct CorridorTask {
    Room* room1;
    Room* room2;
    std::vector<DrawSpan> spans;
    std::vector<Ladder> ladders;
    std::vector<Rope> ropes;
};

class RoomConnectionGenerator {
public:
    static void createConnections(Map& map, std::mt19937& gen, const std::vector<std::vector<Room*>>& room_grid,
                                 int num_cols, int num_rows, std::vector<Ladder>& ladders_to_place,
                                 std::vector<Rope>& ropes_to_place);

private:
    // KD-tree for efficient nearest neighbor queries
    static std::unique_ptr<KDNode> buildKDTree(const std::vector<Room*>& rooms, int depth = 0);
    static Room* findNearestRoom(const std::unique_ptr<KDNode>& root, Room* target, Room* best = nullptr, float bestDist = std::numeric_limits<float>::max(), int depth = 0);
    
    // Union-Find for cycle detection and connectivity
    static std::vector<std::pair<Room*, Room*>> findMinimumSpanningConnections(const std::vector<Room*>& rooms);
    
    // Parallel corridor generation
    static CorridorTask generateCorridorTask(Room* room1, Room* room2, std::mt19937& gen);
    static void executeCorridorTasks(Map& map, const std::vector<CorridorTask>& tasks,
                                   std::vector<Ladder>& ladders_to_place, std::vector<Rope>& ropes_to_place);
    
    // Utility functions
    static float getRoomDistance(Room* a, Room* b);
    static Vector2 getRoomCenter(Room* room);
};

// Union-Find data structure for MST
class UnionFind {
private:
    std::vector<int> parent, rank;

public:
    UnionFind(int n) : parent(n), rank(n, 0) {
        for (int i = 0; i < n; i++) parent[i] = i;
    }
    
    int find(int x) {
        if (parent[x] != x) parent[x] = find(parent[x]);
        return parent[x];
    }
    
    bool unite(int x, int y) {
        int px = find(x), py = find(y);
        if (px == py) return false;
        
        if (rank[px] < rank[py]) std::swap(px, py);
        parent[py] = px;
        if (rank[px] == rank[py]) rank[px]++;
        return true;
    }
};

// KD-tree node for 2D spatial queries
struct KDNode {
    Room* room;
    std::unique_ptr<KDNode> left, right;
    int axis; // 0 for x, 1 for y
    
    KDNode(Room* r, int a) : room(r), axis(a) {}
};

#endif
