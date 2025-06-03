#include "map/Map.hpp"
#include <random>
#include <algorithm>
#include <vector>
#include <tuple>



void Map::generateRoomContent(const Room& room, std::mt19937& gen) {
    int platY = (room.startY + room.endY) / 2;
    if (platY > room.startY + 1 && platY < room.endY - 1) {
        for (int x = room.startX + 2; x <= room.endX - 2; ++x) {
            if (tiles[x][platY] == 0) {
                tiles[x][platY] = 6;
                isOriginalSolid[x][platY] = false;
            }
        }
    }

    std::uniform_int_distribution<> wallChance(0, 3);
    if (wallChance(gen) == 0) {
        int wxMin = room.startX + 4;
        int wxMax = room.endX - 4;
        if (wxMax >= wxMin) {
            std::uniform_int_distribution<> wxDist(wxMin, wxMax);
            int wx = wxDist(gen);

            std::uniform_int_distribution<> gapSizeDist(3, 5);
            int gapSize = gapSizeDist(gen);

            int gapStartMin = room.startY + 5;
            int gapStartMax = room.endY - 5 - (gapSize - 1);
            if (gapStartMax >= gapStartMin) {
                std::uniform_int_distribution<> gapDist(gapStartMin, gapStartMax);
                int gapStart = gapDist(gen);

                for (int y = room.startY + 4; y <= room.endY - 4; ++y) {
                    bool inGap = (y >= gapStart && y < gapStart + gapSize);
                    if (inGap) continue;

                    if (tiles[wx][y] == 0) {
                        tiles[wx][y] = 1;
                        isOriginalSolid[wx][y] = true;
                    }
                }
            }
        }
    }

    std::uniform_int_distribution<> platCountDist(1, 2);
    int extraPlats = platCountDist(gen);
    const int platMinLen = 4;
    int platMaxLen = std::max(platMinLen, (room.endX - room.startX) / 2);

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

        for (int x = pxStart; x < pxStart + platLen; ++x) {
            if (py > room.startY + 1 && py < room.endY - 1 && tiles[x][py] == 0) {
                tiles[x][py] = 6;
                isOriginalSolid[x][py] = false;
            }
        }
    }

    const int MIN_INTERNAL_LADDER_LENGTH = 8;
    const int MAX_INTERNAL_LADDER_LENGTH = 15;
    const int LADDER_EXCLUSION_ZONE = 1;

    std::vector<std::tuple<int, int, int>> potential_shafts;

    for (int x_col = room.startX + 1; x_col <= room.endX - 1; ++x_col) {
        std::vector<int> platform_ys_in_col;
        for (int y_row = room.startY + 1; y_row <= room.endY - 1; ++y_row) {
            if (tiles[x_col][y_row] == 1 || tiles[x_col][y_row] == 6) {
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

                    if (shaft_height >= MIN_INTERNAL_LADDER_LENGTH) {
                        bool is_shaft_clear = true;
                        for (int y_check = shaft_top_y; y_check <= shaft_bottom_y; ++y_check) {
                            if (tiles[x_col][y_check] != 0) {
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

    int num_ladders_to_attempt = std::uniform_int_distribution<>(1, 2)(gen);
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
        for (int check_x = std::max(room.startX + 1, x_col - LADDER_EXCLUSION_ZONE);
             check_x <= std::min(room.endX - 1, x_col + LADDER_EXCLUSION_ZONE); ++check_x) {
            if (column_occupied[check_x]) {
                can_place_here = false;
                break;
            }
        }

        if (can_place_here) {
            int actual_min_length_for_dist = std::min(MIN_INTERNAL_LADDER_LENGTH, shaft_height);
            int actual_max_length_for_dist = std::min(MAX_INTERNAL_LADDER_LENGTH, shaft_height);

            if (actual_min_length_for_dist > actual_max_length_for_dist) {
                actual_min_length_for_dist = actual_max_length_for_dist;
            }

            if (actual_min_length_for_dist <= actual_max_length_for_dist) {
                std::uniform_int_distribution<> ladderRopeLengthDist(actual_min_length_for_dist, actual_max_length_for_dist);
                int random_length = ladderRopeLengthDist(gen);

                std::uniform_int_distribution<> ladderYStartDist(shaft_top_y, shaft_bottom_y - random_length + 1);
                int ladder_y_start = ladderYStartDist(gen);
                int ladder_y_end = ladder_y_start + random_length - 1;

                int tileType = (std::uniform_int_distribution<>(0, 1)(gen) == 0) ? 2 : 3;

                for (int y = ladder_y_start; y <= ladder_y_end; ++y) {
                    if (tiles[x_col][y] == 0) {
                        tiles[x_col][y] = tileType;
                        isOriginalSolid[x_col][y] = false;
                    }
                }
                ladders_placed++;

                for (int mark_x = std::max(room.startX + 1, x_col - LADDER_EXCLUSION_ZONE);
                     mark_x <= std::min(room.endX - 1, x_col + LADDER_EXCLUSION_ZONE); ++mark_x) {
                    column_occupied[mark_x] = true;
                }
            }
        }
    }
}

void Map::generateTreasureRoomContent(const Room& room, std::mt19937& gen) {
    int treasureX = room.startX + (room.endX - room.startX) / 2;
    int treasureY = room.startY + (room.endY - room.startY) / 2;

    if (treasureX > room.startX && treasureX < room.endX &&
        treasureY > room.startY && treasureY < room.endY) {
        if (tiles[treasureX][treasureY] == 0) {
            tiles[treasureX][treasureY] = 4;
            isOriginalSolid[treasureX][treasureY] = false;
        }
    }

    std::uniform_int_distribution<> extraTreasureDist(0, 2);
    int extraTreasures = extraTreasureDist(gen);

    for (int i = 0; i < extraTreasures; ++i) {
        std::uniform_int_distribution<> xDist(room.startX + 1, room.endX - 1);
        std::uniform_int_distribution<> yDist(room.startY + 1, room.endY - 1);
        int x = xDist(gen);
        int y = yDist(gen);
        if (tiles[x][y] == 0) {
            tiles[x][y] = 4;
            isOriginalSolid[x][y] = false;
        }
    }
}

void Map::generateShopRoomContent(const Room& room, std::mt19937& gen) {
    int shopX = room.startX + (room.endX - room.startX) / 2;
    int shopY = room.startY + (room.endY - room.startY) / 2;

    if (shopX > room.startX && shopX < room.endX &&
        shopY > room.startY && shopY < room.endY) {
        if (tiles[shopX][shopY] == 0) {
            tiles[shopX][shopY] = 5;
            isOriginalSolid[shopX][shopY] = false;
        }
    }

    std::uniform_int_distribution<> extraShopItemsDist(0, 2);
    int extraShopItems = extraShopItemsDist(gen);

    for (int i = 0; i < extraShopItems; ++i) {
        std::uniform_int_distribution<> xDist(room.startX + 1, room.endX - 1);
        std::uniform_int_distribution<> yDist(room.startY + 1, room.endY - 1);
        int x = xDist(gen);
        int y = yDist(gen);
        if (tiles[x][y] == 0) {
            tiles[x][y] = 5;
            isOriginalSolid[x][y] = false;
        }
    }
}

void Map::generateRoomsAndConnections(std::mt19937& gen) {
    const int MIN_ROOM_SLOT_WIDTH = 25;
    const int MIN_ROOM_SLOT_HEIGHT = 20;
    const int SLOT_GAP_SIZE = 5;

    const int HORIZONTAL_DOOR_HEIGHT = 6;
    const int VERTICAL_HALLWAY_WIDTH = 6;

    int num_cols = (width - 2) / (MIN_ROOM_SLOT_WIDTH + SLOT_GAP_SIZE);
    int num_rows = (height - 2) / (MIN_ROOM_SLOT_HEIGHT + SLOT_GAP_SIZE);

    if (num_cols <= 0 || num_rows <= 0) {
        return;
    }

    std::vector<Room> rooms;
    std::vector<Ladder> ladders_to_place;
    std::vector<Rope> ropes_to_place;

    std::vector<std::vector<Room*>> room_grid(num_cols, std::vector<Room*>(num_rows, nullptr));

    for (int r = 0; r < num_rows; ++r) {
        for (int c = 0; c < num_cols; ++c) {
            Room* current_grid_room_ptr = room_grid[c][r];
            if (current_grid_room_ptr != nullptr) {
                continue;
            }

            if (std::uniform_int_distribution<>(0, 99)(gen) < 15) {
                continue;
            }

            int current_x = 1 + c * (MIN_ROOM_SLOT_WIDTH + SLOT_GAP_SIZE);
            int current_y = 1 + r * (MIN_ROOM_SLOT_HEIGHT + SLOT_GAP_SIZE);

            bool is_hall = (std::uniform_int_distribution<>(0, 99)(gen) < 15);
            int room_w_units = 1;
            int room_h_units = 1;

            if (is_hall && c + 1 < num_cols && r + 1 < num_rows &&
                room_grid[c+1][r] == nullptr && room_grid[c][r+1] == nullptr && room_grid[c+1][r+1] == nullptr) {

                int potential_hall_end_x = current_x + (2 * MIN_ROOM_SLOT_WIDTH + SLOT_GAP_SIZE) - 1;
                int potential_hall_end_y = current_y + (2 * MIN_ROOM_SLOT_HEIGHT + SLOT_GAP_SIZE) - 1;

                if (potential_hall_end_x < width - 1 && potential_hall_end_y < height - 1) {
                    room_w_units = 2;
                    room_h_units = 2;
                }
            }

            std::uniform_int_distribution<> roomWidthVariationDist(0, 5);
            int extra_width = roomWidthVariationDist(gen);

            int room_pixel_w = room_w_units * MIN_ROOM_SLOT_WIDTH + (room_w_units - 1) * SLOT_GAP_SIZE + extra_width;
            int room_pixel_h = room_h_units * MIN_ROOM_SLOT_HEIGHT + (room_h_units - 1) * SLOT_GAP_SIZE;

            if (current_x + room_pixel_w >= width - 1 || current_y + room_pixel_h >= height - 1) {
                continue;
            }

            Room new_room = {current_x, current_y, current_x + room_pixel_w - 1, current_y + room_pixel_h - 1};
            
            std::uniform_int_distribution<> roomTypeDist(0, 99);
            int type_roll = roomTypeDist(gen);

            if (type_roll < 10) {
                new_room.type = Room::TREASURE;
            } else if (type_roll < 20) {
                new_room.type = Room::SHOP;
            } else {
                new_room.type = Room::NORMAL;
            }

            rooms.push_back(new_room);
            Room* room_ptr = &rooms.back();

            for (int dr = 0; dr < room_h_units; ++dr) {
                for (int dc = 0; dc < room_w_units; ++dc) {
                    room_grid[c + dc][r + dr] = room_ptr;
                }
            }
        }
    }

    for (int x = 1; x < width - 1; ++x) {
        for (int y = 1; y < height - 1; ++y) {
            tiles[x][y] = 1;
            isOriginalSolid[x][y] = true;
        }
    }

    for (const Room& room : rooms) {
        for (int x = room.startX; x <= room.endX; ++x) {
            for (int y = room.startY; y <= room.endY; ++y) {
                if (x > 0 && x < width - 1 && y > 0 && y < height - 1) {
                    tiles[x][y] = 0;
                    isOriginalSolid[x][y] = false;
                }
            }
        }
    }

    for (int r = 0; r < num_rows; ++r) {
        for (int c = 0; c < num_cols; ++c) {
            Room* room1_ptr = room_grid[c][r];
            if (room1_ptr == nullptr) {
                continue;
            }

            if (c + 1 < num_cols) {
                Room* room2_ptr = room_grid[c+1][r];
                if (room2_ptr != nullptr && room1_ptr != room2_ptr) {
                    if (std::uniform_int_distribution<>(0, 99)(gen) < 20) {
                        continue;
                    }

                    int hallway_y_min_overlap = std::max(room1_ptr->startY, room2_ptr->startY);
                    int hallway_y_max_overlap = std::min(room1_ptr->endY, room2_ptr->endY);

                    if (hallway_y_max_overlap - hallway_y_min_overlap + 1 >= HORIZONTAL_DOOR_HEIGHT) {
                        int hall_y_end = std::min(room1_ptr->endY, room2_ptr->endY);
                        int hall_y_start = hall_y_end - HORIZONTAL_DOOR_HEIGHT + 1;

                        hall_y_start = std::max(hall_y_start, hallway_y_min_overlap);

                        int clear_x_start = room1_ptr->endX;
                        int clear_x_end = room2_ptr->startX;

                        for (int x = clear_x_start; x <= clear_x_end; ++x) {
                            for (int y = hall_y_start; y <= hall_y_end; ++y) {
                                if (x > 0 && x < width - 1 && y > 0 && y < height - 1) {
                                    tiles[x][y] = 0;
                                    isOriginalSolid[x][y] = false;
                                }
                            }
                        }
                    }
                }
            }

            if (r + 1 < num_rows) {
                Room* room2_ptr = room_grid[c][r+1];
                if (room2_ptr != nullptr && room1_ptr != room2_ptr) {
                    if (std::uniform_int_distribution<>(0, 99)(gen) < 20) {
                        continue;
                    }

                    int hallway_x_min_overlap = std::max(room1_ptr->startX, room2_ptr->startX);
                    int hallway_x_max_overlap = std::min(room1_ptr->endX, room2_ptr->endX);

                    if (hallway_x_max_overlap - hallway_x_min_overlap + 1 >= VERTICAL_HALLWAY_WIDTH) {
                        int hall_x_start = hallway_x_min_overlap +
                                           (hallway_x_max_overlap - hallway_x_min_overlap + 1 - VERTICAL_HALLWAY_WIDTH) / 2;
                        int hall_x_end = hall_x_start + VERTICAL_HALLWAY_WIDTH - 1;

                        int clear_y_start_hall = room1_ptr->endY;
                        int clear_y_end_hall = room2_ptr->startY;

                        for (int y = clear_y_start_hall; y <= clear_y_end_hall; ++y) {
                            for (int x = hall_x_start; x <= hall_x_end; ++x) {
                                if (x > 0 && x < width - 1 && y > 0 && y < height - 1) {
                                    tiles[x][y] = 0;
                                    isOriginalSolid[x][y] = false;
                                }
                            }
                        }

                        int tileType = (std::uniform_int_distribution<>(0, 1)(gen) == 0) ? 2 : 3;

                        std::uniform_int_distribution<> ladderRopeXDist(hall_x_start + 1, hall_x_end - 1);
                        int ladder_rope_x = ladderRopeXDist(gen);

                        int ladder_y_actual_start = room1_ptr->endY;
                        int ladder_y_actual_end = room2_ptr->startY;

                        if (tileType == 2) {
                            ladders_to_place.push_back({ladder_rope_x, ladder_y_actual_start, ladder_y_actual_end});
                        } else {
                            ropes_to_place.push_back({ladder_rope_x, ladder_y_actual_start, ladder_y_actual_end});
                        }
                    }
                }
            }
        }
    }

    for (const Room& room : rooms) {
        if (room.type == Room::TREASURE) {
            generateTreasureRoomContent(room, gen);
        } else if (room.type == Room::SHOP) {
            generateShopRoomContent(room, gen);
        } else {
            generateRoomContent(room, gen);
        }
    }

    for (const auto& ladder : ladders_to_place) {
        for (int y = ladder.y1; y <= ladder.y2; ++y) {
            if (ladder.x > 0 && ladder.x < width - 1 && y > 0 && y < height - 1) {
                if (tiles[ladder.x][y] == 0) {
                    tiles[ladder.x][y] = 2;
                    isOriginalSolid[ladder.x][y] = false;
                }
            }
        }
    }
    for (const auto& rope : ropes_to_place) {
        for (int y = rope.y1; y <= rope.y2; ++y) {
            if (rope.x > 0 && rope.x < width - 1 && y > 0 && y < height - 1) {
                if (tiles[rope.x][y] == 0) {
                    tiles[rope.x][y] = 3;
                    isOriginalSolid[rope.x][y] = false;
                }
            }
        }
    }
}
