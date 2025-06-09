#include "map/Map.hpp"

#include "map/RoomGenerator.hpp"
#include "map/RoomContentGenerator.hpp"


using namespace MapConstants;

bool Map::isInsideBounds(int x, int y) const {
    return x > 0 && x < width - 1 && y > 0 && y < height - 1;
}

void Map::generateRoomContent(const Room& room, std::mt19937& gen) {
    RoomContentGenerator::generateRoomContent(*this, room, gen);
}

void Map::generateTreasureRoomContent(const Room& room, std::mt19937& gen) {
    RoomContentGenerator::generateTreasureRoomContent(*this, room, gen);
}

void Map::generateShopRoomContent(const Room& room, std::mt19937& gen) {
    RoomContentGenerator::generateShopRoomContent(*this, room, gen);
}

void Map::generateRoomsAndConnections(std::mt19937& gen) {
    RoomGenerator::generateRoomsAndConnections(*this, gen);
}
