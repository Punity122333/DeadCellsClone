#include "map/RoomGenerator.hpp"

#include "map/RoomContentGenerator.hpp"
#include <cstdio>

using namespace MapConstants;

void RoomGenerator::generateRoomsAndConnections(Map& map, std::mt19937& gen) {
    int num_cols = (map.getWidth() - 2) / (MIN_ROOM_SLOT_WIDTH_CONST + SLOT_GAP_SIZE_CONST);
    int num_rows = (map.getHeight() - 2) / (MIN_ROOM_SLOT_HEIGHT_CONST + SLOT_GAP_SIZE_CONST);

    if (num_cols <= 0 || num_rows <= 0) {
        return;
    }

    std::vector<Room> rooms_vector;
    std::vector<Ladder> ladders_to_place;
    std::vector<Rope> ropes_to_place;
    std::vector<std::vector<Room*>> room_grid(num_cols, std::vector<Room*>(num_rows, nullptr));

    createRoomGrid(map, gen, rooms_vector, room_grid, num_cols, num_rows);
    map.generatedRooms = rooms_vector;

    for (int x_coord = 1; x_coord < map.getWidth() - 1; ++x_coord) {
        for (int y_coord = 1; y_coord < map.getHeight() - 1; ++y_coord) {
            map.tiles[x_coord][y_coord] = DEFAULT_TILE_VALUE;
            map.isOriginalSolid[x_coord][y_coord] = true;
        }
    }

    clearRoomAreas(map, rooms_vector);
    createConnections(map, gen, room_grid, num_cols, num_rows, ladders_to_place, ropes_to_place);
    generateAllRoomContent(map, rooms_vector, gen);
    placeLaddersAndRopes(map, ladders_to_place, ropes_to_place);
    protectEmptyTilesNearWalls(map);

    
    int chestCount = 0;
    for (int x = 0; x < map.getWidth(); ++x) {
        for (int y = 0; y < map.getHeight(); ++y) {
            if (map.tiles[x][y] == CHEST_TILE_VALUE) {
                chestCount++;
               
            }
        }
    }
    
}

void RoomGenerator::createRoomGrid(Map& map, std::mt19937& gen, std::vector<Room>& rooms_vector,
                                  std::vector<std::vector<Room*>>& room_grid, int num_cols, int num_rows) {
    for (int r_idx = 0; r_idx < num_rows; ++r_idx) {
        for (int c_idx = 0; c_idx < num_cols; ++c_idx) {
            Room* current_grid_room_ptr = room_grid[c_idx][r_idx];
            if (current_grid_room_ptr != nullptr) {
                continue;
            }

            if (rollPercent(gen) < ROOM_PLACEMENT_SKIP_CHANCE_PERCENT) {
                continue;
            }

            int current_x = 1 + c_idx * (MIN_ROOM_SLOT_WIDTH_CONST + SLOT_GAP_SIZE_CONST);
            int current_y = 1 + r_idx * (MIN_ROOM_SLOT_HEIGHT_CONST + SLOT_GAP_SIZE_CONST);

            bool is_hall = (rollPercent(gen) < LARGE_HALL_CREATION_CHANCE_PERCENT);
            int room_width_slots = 1;
            int room_height_slots = 1;

            if (is_hall && c_idx + 1 < num_cols && r_idx + 1 < num_rows &&
                room_grid[c_idx+1][r_idx] == nullptr && room_grid[c_idx][r_idx+1] == nullptr && room_grid[c_idx+1][r_idx+1] == nullptr) {

                int potential_hall_end_x = current_x + (2 * MIN_ROOM_SLOT_WIDTH_CONST + SLOT_GAP_SIZE_CONST) - 1;
                int potential_hall_end_y = current_y + (2 * MIN_ROOM_SLOT_HEIGHT_CONST + SLOT_GAP_SIZE_CONST) - 1;

                if (potential_hall_end_x < map.getWidth() - 1 && potential_hall_end_y < map.getHeight() - 1) {
                    room_width_slots = 2;
                    room_height_slots = 2;
                }
            }

            std::uniform_int_distribution<> roomWidthVariationDist(0, MAX_ROOM_WIDTH_RANDOM_VARIATION);
            int extra_width = roomWidthVariationDist(gen);

            int room_pixel_w = room_width_slots * MIN_ROOM_SLOT_WIDTH_CONST + (room_width_slots - 1) * SLOT_GAP_SIZE_CONST + extra_width;
            int room_pixel_h = room_height_slots * MIN_ROOM_SLOT_HEIGHT_CONST + (room_height_slots - 1) * SLOT_GAP_SIZE_CONST;

            if (current_x + room_pixel_w >= map.getWidth() - 1 || current_y + room_pixel_h >= map.getHeight() - 1) {
                continue;
            }
            
            Room::Type room_type_enum = Room::NORMAL;
            int room_type_roll = rollPercent(gen);

            if (room_type_roll < ROOM_TYPE_TREASURE_CHANCE_THRESHOLD_PERCENT) {
                room_type_enum = Room::TREASURE;
            } else if (room_type_roll < ROOM_TYPE_SHOP_CHANCE_THRESHOLD_PERCENT) {
                room_type_enum = Room::SHOP;
            }

            rooms_vector.emplace_back(current_x, current_y, current_x + room_pixel_w - 1, current_y + room_pixel_h - 1, room_type_enum);
            Room* room_ptr = &rooms_vector.back();

            for (int dr = 0; dr < room_height_slots; ++dr) {
                for (int dc = 0; dc < room_width_slots; ++dc) {
                    room_grid[c_idx + dc][r_idx + dr] = room_ptr;
                }
            }
        }
    }
}

void RoomGenerator::clearRoomAreas(Map& map, const std::vector<Room>& rooms_vector) {
    for (const auto& room : rooms_vector) {
        for (int x_coord = room.startX; x_coord <= room.endX; ++x_coord) {
            for (int y_coord = room.startY; y_coord <= room.endY; ++y_coord) {
                if (map.isInsideBounds(x_coord, y_coord)) {
                    map.tiles[x_coord][y_coord] = EMPTY_TILE_VALUE;
                    map.isOriginalSolid[x_coord][y_coord] = false;
                }
            }
        }
    }
}

void RoomGenerator::createConnections(Map& map, std::mt19937& gen, const std::vector<std::vector<Room*>>& room_grid,
                                     int num_cols, int num_rows, std::vector<Ladder>& ladders_to_place,
                                     std::vector<Rope>& ropes_to_place) {
    for (int r_idx = 0; r_idx < num_rows; ++r_idx) {
        for (int c_idx = 0; c_idx < num_cols; ++c_idx) {
            Room* room1_ptr = room_grid[c_idx][r_idx];
            if (room1_ptr == nullptr) {
                continue;
            }

            if (c_idx + 1 < num_cols) {
                Room* room2_ptr = room_grid[c_idx+1][r_idx];
                if (room2_ptr != nullptr && room1_ptr != room2_ptr) {
                    if (rollPercent(gen) < HALLWAY_CREATION_SKIP_PERCENT) {
                        continue;
                    }

                    int hallway_y_min_overlap = std::max(room1_ptr->startY, room2_ptr->startY);
                    int hallway_y_max_overlap = std::min(room1_ptr->endY, room2_ptr->endY);

                    if (hallway_y_max_overlap - hallway_y_min_overlap + 1 >= HORIZONTAL_DOOR_HEIGHT_CONST) {
                        int hall_y_end = std::min(room1_ptr->endY, room2_ptr->endY);
                        int hall_y_start = hall_y_end - HORIZONTAL_DOOR_HEIGHT_CONST + 1;

                        hall_y_start = std::max(hall_y_start, hallway_y_min_overlap);

                        int clear_x_start = room1_ptr->endX;
                        int clear_x_end = room2_ptr->startX;

                        for (int x_coord = clear_x_start; x_coord <= clear_x_end; ++x_coord) {
                            for (int y_coord = hall_y_start; y_coord <= hall_y_end; ++y_coord) {
                                if (map.isInsideBounds(x_coord, y_coord)) {
                                    map.tiles[x_coord][y_coord] = EMPTY_TILE_VALUE;
                                    map.isOriginalSolid[x_coord][y_coord] = false;
                                }
                            }
                        }
                    }
                }
            }

            if (r_idx + 1 < num_rows) {
                Room* room2_ptr = room_grid[c_idx][r_idx+1];
                if (room2_ptr != nullptr && room1_ptr != room2_ptr) {
                    if (rollPercent(gen) < HALLWAY_CREATION_SKIP_PERCENT) {
                        continue;
                    }

                    int hallway_x_min_overlap = std::max(room1_ptr->startX, room2_ptr->startX);
                    int hallway_x_max_overlap = std::min(room1_ptr->endX, room2_ptr->endX);

                    if (hallway_x_max_overlap - hallway_x_min_overlap + 1 >= VERTICAL_HALLWAY_WIDTH_CONST) {
                        int hall_x_start = hallway_x_min_overlap +
                                           (hallway_x_max_overlap - hallway_x_min_overlap + 1 - VERTICAL_HALLWAY_WIDTH_CONST) / 2;
                        int hall_x_end = hall_x_start + VERTICAL_HALLWAY_WIDTH_CONST - 1;

                        int clear_y_start_hall = room1_ptr->endY;
                        int clear_y_end_hall = room2_ptr->startY;

                        for (int y_coord = clear_y_start_hall; y_coord <= clear_y_end_hall; ++y_coord) {
                            for (int x_coord = hall_x_start; x_coord <= hall_x_end; ++x_coord) {
                                if (map.isInsideBounds(x_coord, y_coord)) {
                                    map.tiles[x_coord][y_coord] = EMPTY_TILE_VALUE;
                                    map.isOriginalSolid[x_coord][y_coord] = false;
                                }
                            }
                        }

                        int tileType = (std::uniform_int_distribution<>(0, LADDER_OR_ROPE_ROLL_MAX)(gen) == 0) ? LADDER_TILE_VALUE : ROPE_TILE_VALUE;

                        std::uniform_int_distribution<> ladderRopeXDist(hall_x_start + 1, hall_x_end - 1);
                        int ladder_rope_x = ladderRopeXDist(gen);

                        int ladder_y_actual_start = room1_ptr->endY;
                        int ladder_y_actual_end = room2_ptr->startY;

                        if (tileType == LADDER_TILE_VALUE) {
                            ladders_to_place.emplace_back(ladder_rope_x, ladder_y_actual_start, ladder_y_actual_end);
                        } else {
                            ropes_to_place.emplace_back(ladder_rope_x, ladder_y_actual_start, ladder_y_actual_end);
                        }
                    }
                }
            }
        }
    }
}

void RoomGenerator::placeLaddersAndRopes(Map& map, const std::vector<Ladder>& ladders_to_place,
                                        const std::vector<Rope>& ropes_to_place) {
    for (const auto& ladder : ladders_to_place) {
        for (int y_coord = ladder.y1; y_coord <= ladder.y2; ++y_coord) {
            if (map.isInsideBounds(ladder.x, y_coord)) {
                if (map.tiles[ladder.x][y_coord] == EMPTY_TILE_VALUE || map.tiles[ladder.x][y_coord] == CHEST_TILE_VALUE) {
                    if (map.tiles[ladder.x][y_coord] != CHEST_TILE_VALUE) {
                        map.tiles[ladder.x][y_coord] = LADDER_TILE_VALUE;
                        map.isOriginalSolid[ladder.x][y_coord] = false;
                    }
                }
            }
        }
    }
    for (const auto& rope : ropes_to_place) {
        for (int y_coord = rope.y1; y_coord <= rope.y2; ++y_coord) {
            if (map.isInsideBounds(rope.x, y_coord)) {
                if (map.tiles[rope.x][y_coord] == EMPTY_TILE_VALUE || map.tiles[rope.x][y_coord] == CHEST_TILE_VALUE) {
                    if (map.tiles[rope.x][y_coord] != CHEST_TILE_VALUE) {
                        map.tiles[rope.x][y_coord] = ROPE_TILE_VALUE;
                        map.isOriginalSolid[rope.x][y_coord] = false;
                    }
                }
            }
        }
    }
}

void RoomGenerator::generateAllRoomContent(Map& map, const std::vector<Room>& rooms_vector, std::mt19937& gen) {
    int treasureRoomCount = 0;
    for (const auto& room : rooms_vector) {
        if (room.type == Room::TREASURE) {
            treasureRoomCount++;
            RoomContentGenerator::generateTreasureRoomContent(map, room, gen);
        } else if (room.type == Room::SHOP) {
            RoomContentGenerator::generateShopRoomContent(map, room, gen);
        } else {
            RoomContentGenerator::generateRoomContent(map, room, gen);
        }
    }
    
}

void RoomGenerator::protectEmptyTilesNearWalls(Map& map) {
    using namespace MapConstants;
    for (int x = 0; x < map.getWidth(); ++x) {
        for (int y = 0; y < map.getHeight(); ++y) {
            if (map.tiles[x][y] == WALL_TILE_VALUE) {
                for (int dx = -4; dx <= 4; ++dx) {
                    for (int dy = -4; dy <= 4; ++dy) {
                        int nx = x + dx;
                        int ny = y + dy;
                        if (map.isInsideBounds(nx, ny) && map.tiles[nx][ny] == EMPTY_TILE_VALUE) {
                            map.tiles[nx][ny] = PROTECTED_EMPTY_TILE_VALUE;
                        }
                    }
                }
            }
        }
    }
}
