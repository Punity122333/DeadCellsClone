#ifndef ROOM_GRID_GENERATOR_HPP
#define ROOM_GRID_GENERATOR_HPP

#include "map/Map.hpp"
#include "core/FastRNG.hpp"
#include <random>
#include <vector>

class RoomGridGenerator {
public:
    static void createRoomGrid(Map& map, std::mt19937& gen, std::vector<Room>& rooms_vector,
                              std::vector<std::vector<Room*>>& room_grid, int num_cols, int num_rows);
    
    static void createRoomGridOptimized(Map& map, FastRNG& rng, std::vector<Room>& rooms_vector,
                                       std::vector<std::vector<Room*>>& room_grid, int num_cols, int num_rows);
    
    static void clearRoomAreas(Map& map, const std::vector<Room>& rooms_vector);

private:
};

#endif
