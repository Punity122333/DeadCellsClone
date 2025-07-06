#ifndef ROOM_GRID_GENERATOR_HPP
#define ROOM_GRID_GENERATOR_HPP

#include "map/Map.hpp"
#include "core/FastRNG.hpp"
#include <random>
#include <vector>

struct GridRegion {
    int start_col;
    int end_col;
    int start_row;
    int end_row;
    uint64_t seed;
};

class RoomGridGenerator {
public:
    static void createRoomGrid(Map& map, std::mt19937& gen, std::vector<Room>& rooms_vector,
                              std::vector<std::vector<Room*>>& room_grid, int num_cols, int num_rows);
    
    static void createRoomGridOptimized(Map& map, FastRNG& rng, std::vector<Room>& rooms_vector,
                                       std::vector<std::vector<Room*>>& room_grid, int num_cols, int num_rows);
    
    static void clearRoomAreas(Map& map, const std::vector<Room>& rooms_vector);

private:
    static void processRegion(const GridRegion& region, Map& map, 
                             std::vector<std::vector<Room*>>& local_room_grid,
                             std::vector<Room>& local_rooms, int num_cols, int num_rows);
};

#endif
