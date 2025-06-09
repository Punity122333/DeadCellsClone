#ifndef MAP_CONSTANTS_HPP
#define MAP_CONSTANTS_HPP

#include <random>

namespace MapConstants {
    constexpr int ROOM_PLACEMENT_SKIP_CHANCE_PERCENT = 5;
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

    int rollPercent(std::mt19937& gen);
}

#endif
