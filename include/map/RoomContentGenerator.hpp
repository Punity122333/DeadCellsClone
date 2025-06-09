#ifndef ROOM_CONTENT_GENERATOR_HPP
#define ROOM_CONTENT_GENERATOR_HPP

#include "map/Map.hpp"
#include <random>

class RoomContentGenerator {
public:
    static void generateRoomContent(Map& map, const Room& room, std::mt19937& gen);
    static void generateTreasureRoomContent(Map& map, const Room& room, std::mt19937& gen);
    static void generateShopRoomContent(Map& map, const Room& room, std::mt19937& gen);

private:
    static void generatePlatforms(Map& map, const Room& room, std::mt19937& gen);
    static void generateWalls(Map& map, const Room& room, std::mt19937& gen);
    static void generateLaddersAndRopes(Map& map, const Room& room, std::mt19937& gen);
};

#endif
