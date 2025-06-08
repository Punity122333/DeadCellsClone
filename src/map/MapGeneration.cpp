#include "map/Map.hpp"
#include <random>
#include <algorithm>
#include <vector>
#include <tuple>

namespace {
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

    int rollPercent(std::mt19937& gen) {
        static std::uniform_int_distribution<> dist(0, 99);
        return dist(gen);
    }
}

bool Map::isInsideBounds(int x, int y) const {
    return x > 0 && x < width - 1 && y > 0 && y < height - 1;
}

void Map::generateRoomContent(const Room& room, std::mt19937& gen) {
    const int room_width = room.endX - room.startX;
    int platY = (room.startY + room.endY) / 2;
    if (platY > room.startY + 1 && platY < room.endY - 1) {
        for (int x = room.startX + 2; x <= room.endX - 2; ++x) {
            if (tiles[x][platY] == EMPTY_TILE_VALUE) {
                tiles[x][platY] = PLATFORM_TILE_VALUE;
                isOriginalSolid[x][platY] = false;
            }
        }
    }

    std::uniform_int_distribution<> wallChance(0, WALL_PLACEMENT_CHANCE_MAX_ROLL);
    if (wallChance(gen) == 0) {
        int wxMin = room.startX + 4;
        int wxMax = room.endX - 4;
        if (wxMax >= wxMin) {
            std::uniform_int_distribution<> wxDist(wxMin, wxMax);
            int wx = wxDist(gen);

            std::uniform_int_distribution<> gapSizeDist(MIN_WALL_VERTICAL_GAP_SIZE, MAX_WALL_VERTICAL_GAP_SIZE);
            int gapSize = gapSizeDist(gen);

            int gapStartMin = room.startY + 5;
            int gapStartMax = room.endY - 5 - (gapSize - 1);
            if (gapStartMax >= gapStartMin) {
                std::uniform_int_distribution<> gapDist(gapStartMin, gapStartMax);
                int gapStart = gapDist(gen);

                for (int y_coord = room.startY + 4; y_coord <= room.endY - 4; ++y_coord) {
                    bool inGap = (y_coord >= gapStart && y_coord < gapStart + gapSize);
                    if (inGap) continue;

                    if (tiles[wx][y_coord] == EMPTY_TILE_VALUE) {
                        tiles[wx][y_coord] = WALL_TILE_VALUE;
                        isOriginalSolid[wx][y_coord] = true;
                    }
                }
            }
        }
    }

    std::uniform_int_distribution<> platCountDist(MIN_RANDOM_PLATFORMS_IN_ROOM, MAX_RANDOM_PLATFORMS_IN_ROOM);
    int extraPlats = platCountDist(gen);
    const int platMinLen = MIN_RANDOM_PLATFORM_LENGTH;
    int platMaxLen = std::max(platMinLen, room_width / 2);

    for (int i = 0; i < extraPlats; ++i) {
        std::uniform_int_distribution<> platLenDist(platMinLen, platMaxLen);
        int platLen = platLenDist(gen);
        int platStartMin = room.startX + 2;
        int platStartMax = room.endX - 2 - platLen + 1;
        if (platStartMax < platStartMin) {
            continue;
        }

        std::uniform_int_distribution<> platStartDist(platStartMin, platStartMax);
        int pxStart = platStartDist(gen);
        std::uniform_int_distribution<> platYDist(room.startY + 2, room.endY - 2);
        int py = platYDist(gen);

        for (int x_coord = pxStart; x_coord < pxStart + platLen; ++x_coord) {
            if (py > room.startY + 1 && py < room.endY - 1 && tiles[x_coord][py] == EMPTY_TILE_VALUE) {
                tiles[x_coord][py] = PLATFORM_TILE_VALUE;
                isOriginalSolid[x_coord][py] = false;
            }
        }
    }

    std::vector<std::tuple<int, int, int>> potential_shafts;

    for (int x_col = room.startX + 1; x_col <= room.endX - 1; ++x_col) {
        std::vector<int> platform_ys_in_col;
        for (int y_row = room.startY + 1; y_row <= room.endY - 1; ++y_row) {
            if (tiles[x_col][y_row] == WALL_TILE_VALUE || tiles[x_col][y_row] == PLATFORM_TILE_VALUE) {
                platform_ys_in_col.push_back(y_row);
            }
        }

        std::sort(platform_ys_in_col.begin(), platform_ys_in_col.end());

        for (size_t i = 0; i < platform_ys_in_col.size(); ++i) {
            for (size_t j = i + 1; j < platform_ys_in_col.size(); ++j) {
                int upper_plat_y = platform_ys_in_col[i];
                int lower_plat_y = platform_ys_in_col[j];

                if (lower_plat_y > upper_plat_y + 1) {
                    int shaft_top_y = upper_plat_y + 1;
                    int shaft_bottom_y = lower_plat_y - 1;
                    int shaft_height = shaft_bottom_y - shaft_top_y + 1;

                    if (shaft_height >= MIN_GENERATED_LADDER_LENGTH) {
                        bool is_shaft_clear = true;
                        for (int y_check = shaft_top_y; y_check <= shaft_bottom_y; ++y_check) {
                            if (tiles[x_col][y_check] != EMPTY_TILE_VALUE) {
                                is_shaft_clear = false;
                                break;
                            }
                        }

                        if (is_shaft_clear) {
                            potential_shafts.emplace_back(x_col, shaft_top_y, shaft_bottom_y);
                        }
                    }
                }
            }
        }
    }

    std::shuffle(potential_shafts.begin(), potential_shafts.end(), gen);

    std::uniform_int_distribution<> numLaddersDist(MIN_LADDERS_PER_ROOM_SHAFT_AREA, MAX_LADDERS_PER_ROOM_SHAFT_AREA);
    int num_ladders_to_attempt = numLaddersDist(gen);
    int ladders_placed = 0;

    std::vector<bool> column_occupied(width, false);

    for (const auto& shaft : potential_shafts) {
        if (ladders_placed >= num_ladders_to_attempt) {
            break;
        }

        int x_col = std::get<0>(shaft);
        int shaft_top_y = std::get<1>(shaft);
        int shaft_bottom_y = std::get<2>(shaft);
        int shaft_height = shaft_bottom_y - shaft_top_y + 1;

        bool can_place_here = true;
        for (int check_x = std::max(room.startX + 1, x_col - GENERATED_LADDER_EXCLUSION_ZONE);
             check_x <= std::min(room.endX - 1, x_col + GENERATED_LADDER_EXCLUSION_ZONE); ++check_x) {
            if (column_occupied[check_x]) {
                can_place_here = false;
                break;
            }
        }

        if (can_place_here) {
            int actual_min_length_for_dist = std::min(MIN_GENERATED_LADDER_LENGTH, shaft_height);
            int actual_max_length_for_dist = std::min(MAX_GENERATED_LADDER_LENGTH, shaft_height);

            if (actual_min_length_for_dist > actual_max_length_for_dist) {
                actual_min_length_for_dist = actual_max_length_for_dist;
            }

            if (actual_min_length_for_dist <= actual_max_length_for_dist) {
                std::uniform_int_distribution<> ladderRopeLengthDist(actual_min_length_for_dist, actual_max_length_for_dist);
                int random_length = ladderRopeLengthDist(gen);

                std::uniform_int_distribution<> ladderYStartDist(shaft_top_y, shaft_bottom_y - random_length + 1);
                int ladder_y_start = ladderYStartDist(gen);
                int ladder_y_end = ladder_y_start + random_length - 1;

                int tileType = (std::uniform_int_distribution<>(0, LADDER_OR_ROPE_ROLL_MAX)(gen) == 0) ? LADDER_TILE_VALUE : ROPE_TILE_VALUE;

                for (int y_coord = ladder_y_start; y_coord <= ladder_y_end; ++y_coord) {
                    if (tiles[x_col][y_coord] == EMPTY_TILE_VALUE) {
                        tiles[x_col][y_coord] = tileType;
                        isOriginalSolid[x_col][y_coord] = false;
                    }
                }
                ladders_placed++;

                for (int mark_x = std::max(room.startX + 1, x_col - GENERATED_LADDER_EXCLUSION_ZONE);
                     mark_x <= std::min(room.endX - 1, x_col + GENERATED_LADDER_EXCLUSION_ZONE); ++mark_x) {
                    column_occupied[mark_x] = true;
                }
            }
        }
    }
}

void Map::generateTreasureRoomContent(const Room& room, std::mt19937& gen) {
    printf("DEBUG: Generating treasure room content for room at (%d,%d) to (%d,%d)\n", 
           room.startX, room.startY, room.endX, room.endY);
    
    const int room_width = room.endX - room.startX;
    const int room_height = room.endY - room.startY;
    int treasureX = room.startX + room_width / 2;
    int treasureY = room.startY + room_height / 2;

    // Create a platform in the middle of the treasure room
    if (isInsideBounds(treasureX, treasureY + 1)) {
        if (tiles[treasureX][treasureY + 1] == EMPTY_TILE_VALUE) {
            tiles[treasureX][treasureY + 1] = DEFAULT_TILE_VALUE; // Use boundary tile value for platform
            isOriginalSolid[treasureX][treasureY + 1] = true;
        }
    }

    // Place the chest on top of the platform
    if (isInsideBounds(treasureX, treasureY)) {
        if (tiles[treasureX][treasureY] == EMPTY_TILE_VALUE) {
            tiles[treasureX][treasureY] = CHEST_TILE_VALUE;
            isOriginalSolid[treasureX][treasureY] = false;
            printf("DEBUG: Placed main chest at (%d,%d)\n", treasureX, treasureY);
        } else {
            printf("DEBUG: Could not place main chest at (%d,%d) - tile value is %d\n", 
                   treasureX, treasureY, tiles[treasureX][treasureY]);
        }
    }

    std::uniform_int_distribution<> extraTreasureDist(0, MAX_EXTRA_TREASURES_IN_ROOM);
    int extraTreasures = extraTreasureDist(gen);

    for (int i = 0; i < extraTreasures; ++i) {
        std::uniform_int_distribution<> xDist(room.startX + 1, room.endX - 1);
        std::uniform_int_distribution<> yDist(room.startY + 1, room.endY - 1);
        int x_coord = xDist(gen);
        int y_coord = yDist(gen);
        if (isInsideBounds(x_coord, y_coord)) {
            
            if (isInsideBounds(x_coord, y_coord + 1) && tiles[x_coord][y_coord + 1] == EMPTY_TILE_VALUE) {
                tiles[x_coord][y_coord + 1] = DEFAULT_TILE_VALUE;
                isOriginalSolid[x_coord][y_coord + 1] = true;
            }
            
            if (tiles[x_coord][y_coord] == EMPTY_TILE_VALUE) {
                tiles[x_coord][y_coord] = CHEST_TILE_VALUE;
                isOriginalSolid[x_coord][y_coord] = false;
            }
        }
    }
}

void Map::generateShopRoomContent(const Room& room, std::mt19937& gen) {
    const int room_width = room.endX - room.startX;
    const int room_height = room.endY - room.startY;
    int shopX = room.startX + room_width / 2;
    int shopY = room.startY + room_height / 2;

    if (isInsideBounds(shopX, shopY)) {
        if (tiles[shopX][shopY] == EMPTY_TILE_VALUE) {
            tiles[shopX][shopY] = SHOP_TILE_VALUE;
            isOriginalSolid[shopX][shopY] = false;
        }
    }

    std::uniform_int_distribution<> extraShopItemsDist(0, MAX_EXTRA_SHOP_ITEMS_IN_ROOM);
    int extraShopItems = extraShopItemsDist(gen);

    for (int i = 0; i < extraShopItems; ++i) {
        std::uniform_int_distribution<> xDist(room.startX + 1, room.endX - 1);
        std::uniform_int_distribution<> yDist(room.startY + 1, room.endY - 1);
        int x_coord = xDist(gen);
        int y_coord = yDist(gen);
        if (tiles[x_coord][y_coord] == EMPTY_TILE_VALUE) {
            tiles[x_coord][y_coord] = SHOP_TILE_VALUE;
            isOriginalSolid[x_coord][y_coord] = false;
        }
    }
}

void Map::generateRoomsAndConnections(std::mt19937& gen) {
    int num_cols = (width - 2) / (MIN_ROOM_SLOT_WIDTH_CONST + SLOT_GAP_SIZE_CONST);
    int num_rows = (height - 2) / (MIN_ROOM_SLOT_HEIGHT_CONST + SLOT_GAP_SIZE_CONST);

    if (num_cols <= 0 || num_rows <= 0) {
        return;
    }

    std::vector<Room> rooms_vector;
    std::vector<Ladder> ladders_to_place;
    std::vector<Rope> ropes_to_place;

    std::vector<std::vector<Room*>> room_grid(num_cols, std::vector<Room*>(num_rows, nullptr));

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

                if (potential_hall_end_x < width - 1 && potential_hall_end_y < height - 1) {
                    room_width_slots = 2;
                    room_height_slots = 2;
                }
            }

            std::uniform_int_distribution<> roomWidthVariationDist(0, MAX_ROOM_WIDTH_RANDOM_VARIATION);
            int extra_width = roomWidthVariationDist(gen);

            int room_pixel_w = room_width_slots * MIN_ROOM_SLOT_WIDTH_CONST + (room_width_slots - 1) * SLOT_GAP_SIZE_CONST + extra_width;
            int room_pixel_h = room_height_slots * MIN_ROOM_SLOT_HEIGHT_CONST + (room_height_slots - 1) * SLOT_GAP_SIZE_CONST;

            if (current_x + room_pixel_w >= width - 1 || current_y + room_pixel_h >= height - 1) {
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

    generatedRooms = rooms_vector;

    for (int x_coord = 1; x_coord < width - 1; ++x_coord) {
        for (int y_coord = 1; y_coord < height - 1; ++y_coord) {
            tiles[x_coord][y_coord] = DEFAULT_TILE_VALUE;
            isOriginalSolid[x_coord][y_coord] = true;
        }
    }

    for (const auto& room : rooms_vector) {
        for (int x_coord = room.startX; x_coord <= room.endX; ++x_coord) {
            for (int y_coord = room.startY; y_coord <= room.endY; ++y_coord) {
                if (isInsideBounds(x_coord, y_coord)) {
                    tiles[x_coord][y_coord] = EMPTY_TILE_VALUE;
                    isOriginalSolid[x_coord][y_coord] = false;
                }
            }
        }
    }

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
                                if (isInsideBounds(x_coord, y_coord)) {
                                    tiles[x_coord][y_coord] = EMPTY_TILE_VALUE;
                                    isOriginalSolid[x_coord][y_coord] = false;
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
                                if (isInsideBounds(x_coord, y_coord)) {
                                    tiles[x_coord][y_coord] = EMPTY_TILE_VALUE;
                                    isOriginalSolid[x_coord][y_coord] = false;
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

    // Place room content (chests, platforms, etc) AFTER all clearing
    int treasureRoomCount = 0;
    for (const auto& room : rooms_vector) {
        if (room.type == Room::TREASURE) {
            treasureRoomCount++;
            generateTreasureRoomContent(room, gen);
        } else if (room.type == Room::SHOP) {
            generateShopRoomContent(room, gen);
        } else {
            generateRoomContent(room, gen);
        }
    }
    printf("DEBUG: Generated %d treasure rooms total\n", treasureRoomCount);

    for (const auto& ladder : ladders_to_place) {
        for (int y_coord = ladder.y1; y_coord <= ladder.y2; ++y_coord) {
            if (isInsideBounds(ladder.x, y_coord)) {
                // Only place ladder if tile is EMPTY or CHEST (so chests are not overwritten)
                if (tiles[ladder.x][y_coord] == EMPTY_TILE_VALUE || tiles[ladder.x][y_coord] == CHEST_TILE_VALUE) {
                    // Don't overwrite chests
                    if (tiles[ladder.x][y_coord] != CHEST_TILE_VALUE) {
                        tiles[ladder.x][y_coord] = LADDER_TILE_VALUE;
                        isOriginalSolid[ladder.x][y_coord] = false;
                    }
                }
            }
        }
    }
    for (const auto& rope : ropes_to_place) {
        for (int y_coord = rope.y1; y_coord <= rope.y2; ++y_coord) {
            if (isInsideBounds(rope.x, y_coord)) {
                // Only place rope if tile is EMPTY or CHEST (so chests are not overwritten)
                if (tiles[rope.x][y_coord] == EMPTY_TILE_VALUE || tiles[rope.x][y_coord] == CHEST_TILE_VALUE) {
                    // Don't overwrite chests
                    if (tiles[rope.x][y_coord] != CHEST_TILE_VALUE) {
                        tiles[rope.x][y_coord] = ROPE_TILE_VALUE;
                        isOriginalSolid[rope.x][y_coord] = false;
                    }
                }
            }
        }
    }
    
    // Final verification: Check if chest tiles still exist after all generation processes
    printf("DEBUG: Final chest verification - scanning entire map for chest tiles:\n");
    int chestCount = 0;
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            if (tiles[x][y] == CHEST_TILE_VALUE) {
                chestCount++;
                printf("DEBUG: Found chest tile at (%d,%d)\n", x, y);
            }
        }
    }
    printf("DEBUG: Total chest tiles found in final map: %d\n", chestCount);
}
