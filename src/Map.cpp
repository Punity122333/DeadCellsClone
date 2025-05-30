#include "Map.hpp"
#include <random>
#include <raylib.h>
#include <stack>
#include <vector>
#include <algorithm>
#include <tuple> // Added for std::tuple

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

    int numTiles = 0;
    {
        // Count number of tile images in ../resources/ named tileXXX.png
        for (int i = 0; ; ++i) {
            char path[64];
            snprintf(path, sizeof(path), "../resources/tile%03d.png", i);
            FILE* f = fopen(path, "r");
            if (!f) break;
            fclose(f);
            numTiles++;
        }
    }
    for (int i = 0; i < numTiles; ++i) {
        char path[64];
        snprintf(path, sizeof(path), "../resources/tile%03d.png", i);
        tileTextures.push_back(LoadTexture(path));
    }
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
            tiles[x][y] = 1; // Fill everything with solid tiles initially
        }
    }

    for (const Room& room : rooms) {
        for (int x = room.startX; x <= room.endX; ++x) {
            for (int y = room.startY; y <= room.endY; ++y) {
                if (x > 0 && x < width - 1 && y > 0 && y < height - 1) {
                    tiles[x][y] = 0; // Carve out the rooms
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

            // Connect horizontally
            if (c + 1 < num_cols) {
                Room* room2_ptr = room_grid[c+1][r];
                if (room2_ptr != nullptr && room1_ptr != room2_ptr) {
                    if (std::uniform_int_distribution<>(0, 99)(gen) < 20) {
                        continue; // 20% chance to skip horizontal connection
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
                                    tiles[x][y] = 0; // Clear horizontal hallway
                                }
                            }
                        }
                    }
                }
            }

            // Connect vertically
            if (r + 1 < num_rows) {
                Room* room2_ptr = room_grid[c][r+1];
                if (room2_ptr != nullptr && room1_ptr != room2_ptr) {
                    if (std::uniform_int_distribution<>(0, 99)(gen) < 20) {
                        continue; // 20% chance to skip vertical connection
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
                                    tiles[x][y] = 0; // Clear vertical hallway
                                }
                            }
                        }

                        int tileType = (std::uniform_int_distribution<>(0, 1)(gen) == 0) ? 2 : 3; // Randomly choose ladder or rope

                        // Place ladder/rope in the middle of the vertical hallway
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

    // Generate content for each room (platforms, internal ladders/ropes)
    for (const Room& room : rooms) {
        generateRoomContent(room, gen);
    }

    // Apply all collected ladders and ropes to the map
    for (const auto& ladder : ladders_to_place) {
        for (int y = ladder.y1; y <= ladder.y2; ++y) {
            if (ladder.x > 0 && ladder.x < width - 1 && y > 0 && y < height - 1) {
                if (tiles[ladder.x][y] == 0) { // Only place if the tile is empty
                    tiles[ladder.x][y] = 2;
                }
            }
        }
    }
    for (const auto& rope : ropes_to_place) {
        for (int y = rope.y1; y <= rope.y2; ++y) {
            if (rope.x > 0 && rope.x < width - 1 && y > 0 && y < height - 1) {
                if (tiles[rope.x][y] == 0) { // Only place if the tile is empty
                    tiles[rope.x][y] = 3;
                }
            }
        }
    }
}

void Map::generateRoomContent(const Room& room, std::mt19937& gen) {
    // --- Existing Platform Generation (middle platform) ---
    int platY = (room.startY + room.endY) / 2;
    if (platY > room.startY + 1 && platY < room.endY - 1) {
        for (int x = room.startX + 2; x <= room.endX - 2; ++x) {
            if (tiles[x][platY] == 0) {
                tiles[x][platY] = 1;
            }
        }
    }

    // --- Existing Wall Generation with Gap ---
    std::uniform_int_distribution<> wallChance(0, 3);
    if (wallChance(gen) == 0) { // 25% chance to place a vertical wall
        int wxMin = room.startX + 4;
        int wxMax = room.endX - 4;
        if (wxMax >= wxMin) {
            std::uniform_int_distribution<> wxDist(wxMin, wxMax);
            int wx = wxDist(gen); // Random X position for the wall

            std::uniform_int_distribution<> gapSizeDist(3, 5);
            int gapSize = gapSizeDist(gen); // Random gap size

            int gapStartMin = room.startY + 5;
            int gapStartMax = room.endY - 5 - (gapSize - 1);
            if (gapStartMax >= gapStartMin) {
                std::uniform_int_distribution<> gapDist(gapStartMin, gapStartMax);
                int gapStart = gapDist(gen); // Random Y position for the gap start

                for (int y = room.startY + 4; y <= room.endY - 4; ++y) {
                    bool inGap = (y >= gapStart && y < gapStart + gapSize);
                    if (inGap) continue; // Skip if it's within the gap

                    if (tiles[wx][y] == 0) {
                        tiles[wx][y] = 1; // Place wall tile
                    }
                }
            }
        }
    }

    // --- Existing Extra Platforms Generation ---
    std::uniform_int_distribution<> platCountDist(1, 2);
    int extraPlats = platCountDist(gen); // 1 or 2 extra platforms
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
                tiles[x][py] = 1; // Place platform tile
            }
        }
    }

    // --- NEW Internal Ladder/Rope Generation ---
    const int MIN_INTERNAL_LADDER_LENGTH = 5;
    const int MAX_INTERNAL_LADDER_LENGTH = 12;
    const int LADDER_EXCLUSION_ZONE = 1; // Number of columns to exclude on each side of a placed ladder

    // 1. Collect all potential ladder locations (shafts)
    std::vector<std::tuple<int, int, int>> potential_shafts; // x, shaft_top_y, shaft_bottom_y

    for (int x_col = room.startX + 1; x_col <= room.endX - 1; ++x_col) {
        std::vector<int> platform_ys_in_col;
        for (int y_row = room.startY + 1; y_row <= room.endY - 1; ++y_row) {
            if (tiles[x_col][y_row] == 1) { // Found a platform tile in this column
                platform_ys_in_col.push_back(y_row);
            }
        }

        std::sort(platform_ys_in_col.begin(), platform_ys_in_col.end());

        // Iterate through pairs of platforms to find vertical shafts
        for (size_t i = 0; i < platform_ys_in_col.size(); ++i) {
            for (size_t j = i + 1; j < platform_ys_in_col.size(); ++j) {
                int upper_plat_y = platform_ys_in_col[i];
                int lower_plat_y = platform_ys_in_col[j];

                if (lower_plat_y > upper_plat_y + 1) { // There's a gap between platforms
                    int shaft_top_y = upper_plat_y + 1;
                    int shaft_bottom_y = lower_plat_y - 1;
                    int shaft_height = shaft_bottom_y - shaft_top_y + 1;

                    if (shaft_height >= MIN_INTERNAL_LADDER_LENGTH) {
                        // Check if the entire shaft is empty (tile type 0)
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

    // 2. Randomly select a limited number of these locations and place ladders/ropes
    std::shuffle(potential_shafts.begin(), potential_shafts.end(), gen);

    int num_ladders_to_attempt = std::uniform_int_distribution<>(1, 2)(gen); // Try to place 1 or 2 ladders/ropes
    int ladders_placed = 0;

    // Keep track of columns where a ladder/rope has been placed or is too close
    // Initialize with false for all columns
    std::vector<bool> column_occupied(width, false);

    for (const auto& shaft : potential_shafts) {
        if (ladders_placed >= num_ladders_to_attempt) {
            break; // Stop if we've placed enough
        }

        int x_col = std::get<0>(shaft);
        int shaft_top_y = std::get<1>(shaft);
        int shaft_bottom_y = std::get<2>(shaft);
        int shaft_height = shaft_bottom_y - shaft_top_y + 1;

        // Check if this column or its immediate neighbors are already "occupied" by a placed ladder/rope
        bool can_place_here = true;
        // Check exclusion zone around the current x_col
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

            // Ensure the range for distribution is valid (min <= max)
            if (actual_min_length_for_dist > actual_max_length_for_dist) {
                actual_min_length_for_dist = actual_max_length_for_dist; // Adjust if min somehow became greater than max
            }

            // Only proceed if a valid length range exists
            if (actual_min_length_for_dist <= actual_max_length_for_dist) {
                std::uniform_int_distribution<> ladderRopeLengthDist(actual_min_length_for_dist, actual_max_length_for_dist);
                int random_length = ladderRopeLengthDist(gen);

                // Ensure the ladder fits within the shaft
                std::uniform_int_distribution<> ladderYStartDist(shaft_top_y, shaft_bottom_y - random_length + 1);
                int ladder_y_start = ladderYStartDist(gen);
                int ladder_y_end = ladder_y_start + random_length - 1;

                int tileType = (std::uniform_int_distribution<>(0, 1)(gen) == 0) ? 2 : 3; // 2 for ladder, 3 for rope

                for (int y = ladder_y_start; y <= ladder_y_end; ++y) {
                    // Double check if the tile is still empty before placing
                    // (another content generation step might have filled it)
                    if (tiles[x_col][y] == 0) {
                        tiles[x_col][y] = tileType;
                    }
                }
                ladders_placed++;

                // Mark this column and its immediate neighbors as occupied to prevent side-by-side placement
                for (int mark_x = std::max(room.startX + 1, x_col - LADDER_EXCLUSION_ZONE);
                     mark_x <= std::min(room.endX - 1, x_col + LADDER_EXCLUSION_ZONE); ++mark_x) {
                    column_occupied[mark_x] = true;
                }
            }
        }
    }
}


void Map::draw() const {
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            if (tiles[x][y] == 1) {

                bool top    = (y > 0)            && tiles[x][y - 1] == 1;
                bool bottom = (y < height - 1)   && tiles[x][y + 1] == 1;
                bool left   = (x > 0)            && tiles[x - 1][y] == 1;
                bool right  = (x < width - 1)    && tiles[x + 1][y] == 1;
                bool topLeft     = (x > 0 && y > 0)                   && tiles[x - 1][y - 1] == 1;
                bool topRight    = (x < width - 1 && y > 0)           && tiles[x + 1][y - 1] == 1;
                bool bottomLeft  = (x > 0 && y < height - 1)          && tiles[x - 1][y + 1] == 1;
                bool bottomRight = (x < width - 1 && y < height - 1)  && tiles[x + 1][y + 1] == 1;

                int idx = 0;

                if (!top && !left && !right && !bottom) idx = 15;
                else if (!top && !left && !right)       idx = 4;
                else if (!top && !left && !bottom)      idx = 5;
                else if (!top && !right && !bottom)     idx = 7;
                else if (!left && !right && !bottom)    idx = 24;
                else if (!top && !left)                 idx = 0;
                else if (!top && !right)                idx = 3;
                else if (!top && !bottom)               idx = 6;
                else if (!right && !bottom)             idx = 103;
                else if (!left && !bottom)              idx = 104;
                else if (!top)                          idx = 1;
                else if (!right && !left)               idx = 14;
                else if (!right)                        idx = 13;
                else if (!left)                         idx = 10;
                else if (!bottom) idx = 105;

                else                                    idx = 11;


                DrawTexture(tileTextures[idx], x * 32, y * 32, WHITE);
            } else if (tiles[x][y] == 2) {
                DrawRectangle(x * 32 + 10, y * 32, 12, 32, GOLD); // Draw ladders
            } else if (tiles[x][y] == 3) {
                DrawRectangle(x * 32 + 14, y * 32, 4, 32, SKYBLUE); // Draw ropes
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
    // Loop through all loaded textures and unload them
    for (const auto& texture : tileTextures) {
        UnloadTexture(texture);
    }
}

Vector2 Map::findEmptySpawn() const {
    int totalEmpty = countEmptyTiles();
    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            // Check if the tile is empty and the tile below it is solid (ground)
            if (tiles[x][y] == 0 && y + 1 < height && tiles[x][y + 1] == 1) {
                int reachable = countReachableEmptyTiles(x, y);
                if (reachable >= totalEmpty * 0.8f) { // If 80% of empty tiles are reachable from here
                    return { x * 32.0f, y * 32.0f };
                }
            }
        }
    }
    // Fallback: top-left if no suitable spawn found
    return { 32.0f, 32.0f };
}

bool Map::isTileEmpty(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) return false;
    return tiles[x][y] == 0;
}

int Map::countEmptyTiles() const {
    int count = 0;
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            if (tiles[x][y] == 0) count++;
    return count;
}

int Map::countReachableEmptyTiles(int startX, int startY) const {
    std::vector<std::vector<bool>> visited(width, std::vector<bool>(height, false));
    std::stack<std::pair<int, int>> s;
    s.push({startX, startY});
    int reachable = 0;

    while (!s.empty()) {
        auto [x, y] = s.top(); s.pop();
        if (x < 0 || x >= width || y < 0 || y >= height) continue;
        if (visited[x][y]) continue;
        if (tiles[x][y] != 0) continue; // Only count empty tiles as reachable
        visited[x][y] = true;
        reachable++;

        // Add neighbors to stack
        s.push({x+1, y});
        s.push({x-1, y});
        s.push({x, y+1});
        s.push({x, y-1});
    }
    return reachable;
}
