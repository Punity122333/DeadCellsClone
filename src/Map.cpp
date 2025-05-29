#include "Map.hpp"
#include <random>
#include <vector>
#include <algorithm>

struct Map::Ladder { int x, y1, y2; };
struct Map::Rope { int x, y1, y2; };

struct Room { int startX, startY, endX, endY; };
struct Hallway { int startX, startY, endX, endY; };


Map::Map(int w, int h) : width(w), height(h), tiles(w, std::vector<int>(h, 0)) {
    std::random_device rd;
    std::mt19937 gen(rd());

    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            tiles[x][y] = 0;
        }
    }
    
    tileTexture = LoadTexture("../resources/tile000.png");
    placeBorders();
    generateRoomsAndConnections(gen);
}

void Map::placeBorders() {
    for (int x = 0; x < width; x++) {
        tiles[x][height - 1] = 1;
        tiles[x][0] = 1;
    }
    for (int y = 0; y < height; y++) {
        tiles[0][y] = 1;
        tiles[width - 1][y] = 1;
    }
}

void Map::generateRoomsAndConnections(std::mt19937& gen) {
    const int MIN_ROOM_SLOT_WIDTH = 25;
    const int MIN_ROOM_SLOT_HEIGHT = 20;
    const int SLOT_GAP_SIZE = 5;

    const int HORIZONTAL_DOOR_HEIGHT = 3;
    const int VERTICAL_HALLWAY_WIDTH = 3; // Made the vertical hallway narrower

    int num_cols = (width - 2) / (MIN_ROOM_SLOT_WIDTH + SLOT_GAP_SIZE);
    int num_rows = (height - 2) / (MIN_ROOM_SLOT_HEIGHT + SLOT_GAP_SIZE);

    if (num_cols <= 0 || num_rows <= 0) {
        return;
    }

    std::vector<Room> rooms;
    std::vector<Map::Ladder> ladders_to_place;
    std::vector<Map::Rope> ropes_to_place;

    std::vector<std::vector<Room*>> room_grid(num_cols, std::vector<Room*>(num_rows, nullptr));

    for (int r = 0; r < num_rows; ++r) {
        for (int c = 0; c < num_cols; ++c) {
            if (room_grid[c][r] != nullptr) {
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
        }
    }

    for (const Room& room : rooms) {
        for (int x = room.startX; x <= room.endX; ++x) {
            for (int y = room.startY; y <= room.endY; ++y) {
                if (x > 0 && x < width - 1 && y > 0 && y < height - 1) {
                    tiles[x][y] = 0;
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
        generateRoomContent(room, gen);
    }

    for (const auto& ladder : ladders_to_place) {
        for (int y = ladder.y1; y <= ladder.y2; ++y) {
            if (ladder.x > 0 && ladder.x < width - 1 && y > 0 && y < height - 1) {
                if (tiles[ladder.x][y] == 0) {
                    tiles[ladder.x][y] = 2;
                }
            }
        }
    }
    for (const auto& rope : ropes_to_place) {
        for (int y = rope.y1; y <= rope.y2; ++y) {
            if (rope.x > 0 && rope.x < width - 1 && y > 0 && y < height - 1) {
                if (tiles[rope.x][y] == 0) {
                    tiles[rope.x][y] = 3;
                }
            }
            }
        }
    }

void Map::generateRoomContent(const Room& room, std::mt19937& gen) {
    int platY = (room.startY + room.endY) / 2;
    if (platY > room.startY + 1 && platY < room.endY - 1) {
        for (int x = room.startX + 2; x <= room.endX - 2; ++x) {
            if (tiles[x][platY] == 0) {
                tiles[x][platY] = 1;
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
                    }
                }
            }
        }
    }

    std::uniform_int_distribution<> platCountDist(1, 2);
    int extraPlats = platCountDist(gen);
    const int platMinLen = 4;
    int platMaxLen = std::max(platMinLen, (room.endX - room.startX) / 2);
    std::uniform_int_distribution<> platLenDist(platMinLen, platMaxLen);
    std::uniform_int_distribution<> platYDist(room.startY + 2, room.endY - 2);

    for (int i = 0; i < extraPlats; ++i) {
        int platLen = platLenDist(gen);
        int platStartMin = room.startX + 2;
        int platStartMax = room.endX - 2 - platLen + 1;
        if (platStartMax < platStartMin) {
            continue;
        }

        std::uniform_int_distribution<> platStartDist(platStartMin, platStartMax);
        int pxStart = platStartDist(gen);
        int py = platYDist(gen);

        for (int x = pxStart; x < pxStart + platLen; ++x) {
            if (py > room.startY + 1 && py < room.endY - 1 && tiles[x][py] == 0) {
                tiles[x][py] = 1;
            }
        }
    }

    const int MIN_INTERNAL_LADDER_LENGTH = 5;
    const int MAX_INTERNAL_LADDER_LENGTH = 12;

    for (int x_col = room.startX + 1; x_col <= room.endX - 1; ++x_col) {
        std::vector<int> platform_ys_in_col;
        for (int y_row = room.startY + 1; y_row <= room.endY - 1; ++y_row) {
            if (tiles[x_col][y_row] == 1) {
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
                        bool already_placed_nearby = false;
                        if (x_col > room.startX + 1) {
                            for (int y_check = shaft_top_y; y_check <= shaft_bottom_y; ++y_check) {
                                if (tiles[x_col - 1][y_check] == 2 || tiles[x_col - 1][y_check] == 3) {
                                    already_placed_nearby = true;
                                    break;
                                }
                            }
                        }
                        
                        bool current_column_clear = true;
                        for (int y_check = shaft_top_y; y_check <= shaft_bottom_y; ++y_check) {
                            if (tiles[x_col][y_check] != 0) {
                                current_column_clear = false;
                                break;
                            }
                        }

                        if (!already_placed_nearby && current_column_clear) {
                            int actual_min_length_for_dist = std::min(MIN_INTERNAL_LADDER_LENGTH, shaft_height);
                            int actual_max_length_for_dist = std::min(MAX_INTERNAL_LADDER_LENGTH, shaft_height);

                            if (actual_min_length_for_dist > actual_max_length_for_dist) {
                                actual_min_length_for_dist = actual_max_length_for_dist;
                            }

                            std::uniform_int_distribution<> ladderRopeLengthDist(actual_min_length_for_dist, actual_max_length_for_dist);
                            int random_length = ladderRopeLengthDist(gen);

                            std::uniform_int_distribution<> ladderYStartDist(shaft_top_y, shaft_bottom_y - random_length + 1);
                            int ladder_y_start = ladderYStartDist(gen);
                            int ladder_y_end = ladder_y_start + random_length - 1;

                            int tileType = (std::uniform_int_distribution<>(0, 1)(gen) == 0) ? 2 : 3;

                            for (int y = ladder_y_start; y <= ladder_y_end; ++y) {
                                if (tiles[x_col][y] == 0) {
                                    tiles[x_col][y] = tileType;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void Map::draw() const {
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            if (tiles[x][y] == 1) {
                DrawTexture(tileTexture, x * 32, y * 32, WHITE);
            } else if (tiles[x][y] == 2) {
                DrawRectangle(x * 32 + 10, y * 32, 12, 32, GOLD);
            } else if (tiles[x][y] == 3) {
                DrawRectangle(x * 32 + 14, y * 32, 4, 32, SKYBLUE);
            }
        }
    }
}

bool Map::collidesWithGround(Vector2 pos) const {
    int tx = static_cast<int>(pos.x / 32);
    int ty = static_cast<int>(pos.y / 32);
    if (tx >= 0 && tx < width && ty >= 0 && ty < height) {
        return tiles[tx][ty] == 1;
    }
    return false;
}

bool Map::isSolidTile(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) return false;
    return tiles[x][y] == 1;
}

bool Map::isLadderTile(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) return false;
    return tiles[x][y] == 2;
}

bool Map::isRopeTile(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) return false;
    return tiles[x][y] == 3;
}

int Map::getHeight() const {
    return height;
}

Map::~Map() {
    UnloadTexture(tileTexture);
}
