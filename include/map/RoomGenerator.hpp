#ifndef ROOM_GENERATOR_HPP
#define ROOM_GENERATOR_HPP

#include "map/Map.hpp"
#include <random>
#include <vector>

class RoomGenerator {
public:
    static void generateRoomsAndConnections(Map& map, std::mt19937& gen);

private:
    static void createRoomGrid(Map& map, std::mt19937& gen, std::vector<Room>& rooms_vector,
                              std::vector<std::vector<Room*>>& room_grid, int num_cols, int num_rows);
    static void clearRoomAreas(Map& map, const std::vector<Room>& rooms_vector);
    static void createConnections(Map& map, std::mt19937& gen, const std::vector<std::vector<Room*>>& room_grid,
                                 int num_cols, int num_rows, std::vector<Ladder>& ladders_to_place,
                                 std::vector<Rope>& ropes_to_place);
    static void placeLaddersAndRopes(Map& map, const std::vector<Ladder>& ladders_to_place,
                                   const std::vector<Rope>& ropes_to_place);
    static void generateAllRoomContent(Map& map, const std::vector<Room>& rooms_vector, std::mt19937& gen);
    static void protectEmptyTilesNearWalls(Map& map);
};

#endif
