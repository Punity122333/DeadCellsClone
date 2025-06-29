#ifndef ROOM_GENERATOR_HPP
#define ROOM_GENERATOR_HPP

#include "map/Map.hpp"
#include <random>
#include <vector>
#include <functional>

class RoomGenerator {
public:
    static void generateRoomsAndConnections(Map& map, std::mt19937& gen, std::function<void(float)> progressCallback = nullptr);

private:
    static void generateAllRoomContent(Map& map, const std::vector<Room>& rooms_vector, std::mt19937& gen);
    static void protectEmptyTilesNearWalls(Map& map);
};

#endif
