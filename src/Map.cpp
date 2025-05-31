// Map.cpp
#include "Map.hpp"
#include <raylib.h>
#include <stack>
#include <vector>
#include <algorithm>
#include <tuple> 
#include <random> // Added for std::random_device and std::mt19937

namespace {
}

Map::Map(int w, int h) : 
    width(w), 
    height(h), 
    tiles(w, std::vector<int>(h, 0)), 
    transitionTimers(w, std::vector<float>(h, 0.0f)),
    isOriginalSolid(w, std::vector<bool>(h, false))
{
    std::random_device rd;
    std::mt19937 gen(rd());

    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            tiles[x][y] = 0;
            isOriginalSolid[x][y] = false; 
        }
    }

    int numTiles = 0;
    {
        // Dynamically count tile textures
        for (int i = 0; ; ++i) {
            char path[64];
            snprintf(path, sizeof(path), "../resources/tiles/tile%03d.png", i);
            FILE* f = fopen(path, "r");
            if (!f) break; // Break if file doesn't exist
            fclose(f);
            numTiles++;
        }
    }
    // Load all tile textures
    for (int i = 0; i < numTiles; ++i) {
        char path[64];
        snprintf(path, sizeof(path), "../resources/tiles/tile%03d.png", i);
        tileTextures.push_back(LoadTexture(path));
    }

    // Place initial borders
    placeBorders();
    // Generate rooms and connections within the map
    generateRoomsAndConnections(gen);

    // After initial map generation, mark which tiles are 'original solid'
    // These tiles will not be affected by the Conway Automata rules.
    // This loop must run AFTER all initial solid tiles (borders, rooms, internal platforms/walls) are laid out.
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            if (tiles[x][y] == 1) { // If it's a solid tile (wall/floor)
                isOriginalSolid[x][y] = true;
            } else {
                isOriginalSolid[x][y] = false;
            }
        }
    }
}

// Places solid borders around the map
void Map::placeBorders() {
    for (int x = 0; x < width; x++) {
        tiles[x][height - 1] = 1; // Bottom border
        tiles[x][0] = 1;         // Top border
    }
    for (int y = 0; y < height; y++) {
        tiles[0][y] = 1;         // Left border
        tiles[width - 1][y] = 1; // Right border
    }
}

// Generates rooms, hallways, and connections using a grid-based approach
void Map::generateRoomsAndConnections(std::mt19937& gen) {
    const int MIN_ROOM_SLOT_WIDTH = 25;
    const int MIN_ROOM_SLOT_HEIGHT = 20;
    const int SLOT_GAP_SIZE = 5;

    const int HORIZONTAL_DOOR_HEIGHT = 6; 
    const int VERTICAL_HALLWAY_WIDTH = 6; 

    // Calculate number of potential room slots in the grid
    int num_cols = (width - 2) / (MIN_ROOM_SLOT_WIDTH + SLOT_GAP_SIZE);
    int num_rows = (height - 2) / (MIN_ROOM_SLOT_HEIGHT + SLOT_GAP_SIZE);

    if (num_cols <= 0 || num_rows <= 0) {
        return; // Not enough space for rooms
    }

    std::vector<Map::Room> rooms;
    std::vector<Map::Ladder> ladders_to_place;
    std::vector<Map::Rope> ropes_to_place;

    // Grid to keep track of which room occupies which slot
    std::vector<std::vector<Map::Room*>> room_grid(num_cols, std::vector<Map::Room*>(num_rows, nullptr));

    // Iterate through grid slots to place rooms
    for (int r = 0; r < num_rows; ++r) {
        for (int c = 0; c < num_cols; ++c) {
            // If this slot is already part of a larger room, skip
            if (room_grid[c][r] != nullptr) {
                continue;
            }

            // Randomly decide if a room should be placed here
            if (std::uniform_int_distribution<>(0, 99)(gen) < 15) {
                continue; // 15% chance to skip placing a room in this slot
            }

            // Calculate pixel coordinates for the current slot
            int current_x = 1 + c * (MIN_ROOM_SLOT_WIDTH + SLOT_GAP_SIZE);
            int current_y = 1 + r * (MIN_ROOM_SLOT_HEIGHT + SLOT_GAP_SIZE);

            // Randomly decide if this should be a larger "hall" room
            bool is_hall = (std::uniform_int_distribution<>(0, 99)(gen) < 15);
            int room_w_units = 1; // Default room size is 1x1 slot
            int room_h_units = 1;

            // Attempt to make a 2x2 hall if conditions allow
            if (is_hall && c + 1 < num_cols && r + 1 < num_rows &&
                room_grid[c+1][r] == nullptr && room_grid[c][r+1] == nullptr && room_grid[c+1][r+1] == nullptr) {
                
                int potential_hall_end_x = current_x + (2 * MIN_ROOM_SLOT_WIDTH + SLOT_GAP_SIZE) - 1;
                int potential_hall_end_y = current_y + (2 * MIN_ROOM_SLOT_HEIGHT + SLOT_GAP_SIZE) - 1;

                // Check if the 2x2 hall would fit within map boundaries
                if (potential_hall_end_x < width - 1 && potential_hall_end_y < height - 1) {
                    room_w_units = 2;
                    room_h_units = 2;
                }
            }

            // Add some random width variation to rooms
            std::uniform_int_distribution<> roomWidthVariationDist(0, 5);
            int extra_width = roomWidthVariationDist(gen);

            // Calculate actual pixel dimensions of the room
            int room_pixel_w = room_w_units * MIN_ROOM_SLOT_WIDTH + (room_w_units - 1) * SLOT_GAP_SIZE + extra_width;
            int room_pixel_h = room_h_units * MIN_ROOM_SLOT_HEIGHT + (room_h_units - 1) * SLOT_GAP_SIZE;

            // Ensure room fits within map boundaries
            if (current_x + room_pixel_w >= width - 1 || current_y + room_pixel_h >= height - 1) {
                continue;
            }

            // Create new room and add to list
            Map::Room new_room = {current_x, current_y, current_x + room_pixel_w - 1, current_y + room_pixel_h - 1};
            rooms.push_back(new_room);
            Map::Room* room_ptr = &rooms.back(); // Get pointer to the newly added room

            // Mark the grid slots as occupied by this room
            for (int dr = 0; dr < room_h_units; ++dr) {
                for (int dc = 0; dc < room_w_units; ++dc) {
                    room_grid[c + dc][r + dr] = room_ptr;
                }
            }
        }
    }

    // Initialize all non-border tiles as solid (1)
    // Rooms and hallways will then carve out empty space (0)
    for (int x = 1; x < width - 1; ++x) {
        for (int y = 1; y < height - 1; ++y) {
            tiles[x][y] = 1;
        }
    }

    // Carve out the empty space for each generated room
    for (const Map::Room& room : rooms) {
        for (int x = room.startX; x <= room.endX; ++x) {
            for (int y = room.startY; y <= room.endY; ++y) {
                // Ensure carving stays within map bounds
                if (x > 0 && x < width - 1 && y > 0 && y < height - 1) {
                    tiles[x][y] = 0; // Set to empty
                }
            }
        }
    }

    // Connect rooms with hallways and place ladders/ropes
    for (int r = 0; r < num_rows; ++r) {
        for (int c = 0; c < num_cols; ++c) {
            Map::Room* room1_ptr = room_grid[c][r];
            if (room1_ptr == nullptr) {
                continue; // No room in this slot
            }

            // Try to connect horizontally with the room to the right
            if (c + 1 < num_cols) {
                Map::Room* room2_ptr = room_grid[c+1][r];
                // Check if there's a different room in the adjacent slot
                if (room2_ptr != nullptr && room1_ptr != room2_ptr) {
                    // Randomly skip connection
                    if (std::uniform_int_distribution<>(0, 99)(gen) < 20) {
                        continue; 
                    }

                    // Find vertical overlap between the two rooms
                    int hallway_y_min_overlap = std::max(room1_ptr->startY, room2_ptr->startY);
                    int hallway_y_max_overlap = std::min(room1_ptr->endY, room2_ptr->endY);

                    // If sufficient overlap for a horizontal door/hallway
                    if (hallway_y_max_overlap - hallway_y_min_overlap + 1 >= HORIZONTAL_DOOR_HEIGHT) {
                        // Determine the vertical extent of the hallway
                        int hall_y_end = std::min(room1_ptr->endY, room2_ptr->endY);
                        int hall_y_start = hall_y_end - HORIZONTAL_DOOR_HEIGHT + 1;

                        // Adjust start to be within overlap
                        hall_y_start = std::max(hall_y_start, hallway_y_min_overlap);

                        // Define the horizontal extent of the hallway
                        int clear_x_start = room1_ptr->endX;
                        int clear_x_end = room2_ptr->startX;

                        // Carve out the horizontal hallway
                        for (int x = clear_x_start; x <= clear_x_end; ++x) {
                            for (int y = hall_y_start; y <= hall_y_end; ++y) {
                                if (x > 0 && x < width - 1 && y > 0 && y < height - 1) {
                                    tiles[x][y] = 0; // Set to empty
                                }
                            }
                        }
                    }
                }
            }

            // Try to connect vertically with the room below
            if (r + 1 < num_rows) {
                Map::Room* room2_ptr = room_grid[c][r+1];
                // Check if there's a different room in the adjacent slot
                if (room2_ptr != nullptr && room1_ptr != room2_ptr) {
                    // Randomly skip connection
                    if (std::uniform_int_distribution<>(0, 99)(gen) < 20) {
                        continue; 
                    }

                    // Find horizontal overlap between the two rooms
                    int hallway_x_min_overlap = std::max(room1_ptr->startX, room2_ptr->startX);
                    int hallway_x_max_overlap = std::min(room1_ptr->endX, room2_ptr->endX);

                    // If sufficient overlap for a vertical hallway
                    if (hallway_x_max_overlap - hallway_x_min_overlap + 1 >= VERTICAL_HALLWAY_WIDTH) {
                        // Determine the horizontal extent of the hallway
                        int hall_x_start = hallway_x_min_overlap +
                                           (hallway_x_max_overlap - hallway_x_min_overlap + 1 - VERTICAL_HALLWAY_WIDTH) / 2;
                        int hall_x_end = hall_x_start + VERTICAL_HALLWAY_WIDTH - 1;

                        // Define the vertical extent of the hallway
                        int clear_y_start_hall = room1_ptr->endY;
                        int clear_y_end_hall = room2_ptr->startY;

                        // Carve out the vertical hallway
                        for (int y = clear_y_start_hall; y <= clear_y_end_hall; ++y) {
                            for (int x = hall_x_start; x <= hall_x_end; ++x) {
                                if (x > 0 && x < width - 1 && y > 0 && y < height - 1) {
                                    tiles[x][y] = 0; // Set to empty
                                }
                            }
                        }

                        // Randomly choose between ladder (2) or rope (3) for vertical connection
                        int tileType = (std::uniform_int_distribution<>(0, 1)(gen) == 0) ? 2 : 3;

                        // Place ladder/rope in the center of the vertical hallway
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

    // Generate internal content (platforms, walls, internal ladders) for each room
    for (const Map::Room& room : rooms) {
        generateRoomContent(room, gen);
    }

    // Place all collected ladders onto the map
    for (const auto& ladder : ladders_to_place) {
        for (int y = ladder.y1; y <= ladder.y2; ++y) {
            if (ladder.x > 0 && ladder.x < width - 1 && y > 0 && y < height - 1) {
                if (tiles[ladder.x][y] == 0) { // Only place if the tile is empty
                    tiles[ladder.x][y] = 2; // Ladder tile type
                }
            }
        }
    }
    // Place all collected ropes onto the map
    for (const auto& rope : ropes_to_place) {
        for (int y = rope.y1; y <= rope.y2; ++y) {
            if (rope.x > 0 && rope.x < width - 1 && y > 0 && y < height - 1) {
                if (tiles[rope.x][y] == 0) { // Only place if the tile is empty
                    tiles[rope.x][y] = 3; // Rope tile type
                }
            }
        }
    }
}

// Generates internal features within a given room, like platforms and internal walls/ladders
void Map::generateRoomContent(const Map::Room& room, std::mt19937& gen) {
    // Place a main horizontal platform in the middle of the room
    int platY = (room.startY + room.endY) / 2;
    if (platY > room.startY + 1 && platY < room.endY - 1) { // Ensure platform is not too close to room edges
        for (int x = room.startX + 2; x <= room.endX - 2; ++x) {
            if (tiles[x][platY] == 0) { // Only place if the tile is empty
                tiles[x][platY] = 1; // Solid tile type
            }
        }
    }

    // Randomly place a vertical wall with a gap
    std::uniform_int_distribution<> wallChance(0, 3);
    if (wallChance(gen) == 0) { // 25% chance to place a wall
        int wxMin = room.startX + 4;
        int wxMax = room.endX - 4;
        if (wxMax >= wxMin) { // Ensure there's space for a wall
            std::uniform_int_distribution<> wxDist(wxMin, wxMax);
            int wx = wxDist(gen); // Random X position for the wall

            std::uniform_int_distribution<> gapSizeDist(3, 5);
            int gapSize = gapSizeDist(gen); // Random size for the gap in the wall

            int gapStartMin = room.startY + 5;
            int gapStartMax = room.endY - 5 - (gapSize - 1);
            if (gapStartMax >= gapStartMin) { // Ensure there's space for the gap
                std::uniform_int_distribution<> gapDist(gapStartMin, gapStartMax);
                int gapStart = gapDist(gen); // Random Y position for the gap start

                // Place the wall, leaving a gap
                for (int y = room.startY + 4; y <= room.endY - 4; ++y) {
                    bool inGap = (y >= gapStart && y < gapStart + gapSize);
                    if (inGap) continue; // Skip if within the gap

                    if (tiles[wx][y] == 0) { // Only place if the tile is empty
                        tiles[wx][y] = 1; // Solid tile type
                    }
                }
            }
        }
    }

    // Place additional random platforms
    std::uniform_int_distribution<> platCountDist(1, 2);
    int extraPlats = platCountDist(gen); // 1 or 2 extra platforms
    const int platMinLen = 4;
    int platMaxLen = std::max(platMinLen, (room.endX - room.startX) / 2); // Max length is half room width
    std::uniform_int_distribution<> platLenDist(platMinLen, platMaxLen);
    std::uniform_int_distribution<> platYDist(room.startY + 2, room.endY - 2); // Random Y position for platform

    for (int i = 0; i < extraPlats; ++i) {
        int platLen = platLenDist(gen);
        int platStartMin = room.startX + 2;
        int platStartMax = room.endX - 2 - platLen + 1;
        if (platStartMax < platStartMin) { // Ensure platform can be placed
            continue;
        }

        std::uniform_int_distribution<> platStartDist(platStartMin, platStartMax);
        int pxStart = platStartDist(gen); // Random X start position for platform
        int py = platYDist(gen); // Random Y position for platform

        // Place the platform
        for (int x = pxStart; x < pxStart + platLen; ++x) {
            if (py > room.startY + 1 && py < room.endY - 1 && tiles[x][py] == 0) {
                tiles[x][py] = 1; // Solid tile type
            }
        }
    }

    // Generate internal ladders/ropes within the room
    const int MIN_INTERNAL_LADDER_LENGTH = 8;
    const int MAX_INTERNAL_LADDER_LENGTH = 15;
    const int LADDER_EXCLUSION_ZONE = 1; // Prevents ladders from being too close horizontally

    std::vector<std::tuple<int, int, int>> potential_shafts; // Stores (x, top_y, bottom_y) of clear vertical shafts

    // Find potential vertical shafts (empty columns between platforms)
    for (int x_col = room.startX + 1; x_col <= room.endX - 1; ++x_col) {
        std::vector<int> platform_ys_in_col;
        // Collect Y coordinates of platforms in this column
        for (int y_row = room.startY + 1; y_row <= room.endY - 1; ++y_row) {
            if (tiles[x_col][y_row] == 1) {
                platform_ys_in_col.push_back(y_row);
            }
        }

        std::sort(platform_ys_in_col.begin(), platform_ys_in_col.end());

        // Check pairs of platforms to find vertical gaps
        for (size_t i = 0; i < platform_ys_in_col.size(); ++i) {
            for (size_t j = i + 1; j < platform_ys_in_col.size(); ++j) {
                int upper_plat_y = platform_ys_in_col[i];
                int lower_plat_y = platform_ys_in_col[j];

                if (lower_plat_y > upper_plat_y + 1) { // Ensure there's at least one empty tile between platforms
                    int shaft_top_y = upper_plat_y + 1;
                    int shaft_bottom_y = lower_plat_y - 1;
                    int shaft_height = shaft_bottom_y - shaft_top_y + 1;

                    if (shaft_height >= MIN_INTERNAL_LADDER_LENGTH) { // Check if shaft is long enough
                        bool is_shaft_clear = true;
                        // Verify the entire shaft is empty
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

    std::shuffle(potential_shafts.begin(), potential_shafts.end(), gen); // Randomize order of shafts

    int num_ladders_to_attempt = std::uniform_int_distribution<>(1, 2)(gen); // Try to place 1 or 2 ladders
    int ladders_placed = 0;

    std::vector<bool> column_occupied(width, false); // To track columns where ladders are placed

    // Iterate through potential shafts to place ladders
    for (const auto& shaft : potential_shafts) {
        if (ladders_placed >= num_ladders_to_attempt) {
            break; // Stop if enough ladders are placed
        }

        int x_col = std::get<0>(shaft);
        int shaft_top_y = std::get<1>(shaft);
        int shaft_bottom_y = std::get<2>(shaft);
        int shaft_height = shaft_bottom_y - shaft_top_y + 1;

        bool can_place_here = true;
        // Check exclusion zone to prevent ladders from being too close
        for (int check_x = std::max(room.startX + 1, x_col - LADDER_EXCLUSION_ZONE);
             check_x <= std::min(room.endX - 1, x_col + LADDER_EXCLUSION_ZONE); ++check_x) {
            if (column_occupied[check_x]) {
                can_place_here = false;
                break;
            }
        }

        if (can_place_here) {
            // Determine actual min/max length for the ladder based on shaft height
            int actual_min_length_for_dist = std::min(MIN_INTERNAL_LADDER_LENGTH, shaft_height);
            int actual_max_length_for_dist = std::min(MAX_INTERNAL_LADDER_LENGTH, shaft_height);

            // Adjust if min becomes greater than max due to shaft height
            if (actual_min_length_for_dist > actual_max_length_for_dist) {
                actual_min_length_for_dist = actual_max_length_for_dist;
            }

            if (actual_min_length_for_dist <= actual_max_length_for_dist) {
                std::uniform_int_distribution<> ladderRopeLengthDist(actual_min_length_for_dist, actual_max_length_for_dist);
                int random_length = ladderRopeLengthDist(gen); // Random length for the ladder

                // Determine start Y for the ladder within the shaft
                std::uniform_int_distribution<> ladderYStartDist(shaft_top_y, shaft_bottom_y - random_length + 1);
                int ladder_y_start = ladderYStartDist(gen);
                int ladder_y_end = ladder_y_start + random_length - 1;

                int tileType = (std::uniform_int_distribution<>(0, 1)(gen) == 0) ? 2 : 3; // Randomly choose ladder or rope

                // Place the ladder/rope
                for (int y = ladder_y_start; y <= ladder_y_end; ++y) {
                    if (tiles[x_col][y] == 0) { // Only place if empty
                        tiles[x_col][y] = tileType;
                    }
                }
                ladders_placed++;

                // Mark columns in the exclusion zone as occupied
                for (int mark_x = std::max(room.startX + 1, x_col - LADDER_EXCLUSION_ZONE);
                     mark_x <= std::min(room.endX - 1, x_col + LADDER_EXCLUSION_ZONE); ++mark_x) {
                    column_occupied[mark_x] = true;
                }
            }
        }
    }
}

// Draws the map tiles to the screen
void Map::draw() const {
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            if (tiles[x][y] == 1) { // Solid tile
                // Determine texture index based on surrounding tiles for proper tiling
                bool top    = (y > 0)            && tiles[x][y - 1] == 1;
                bool bottom = (y < height - 1)   && tiles[x][y + 1] == 1;
                bool left   = (x > 0)            && tiles[x - 1][y] == 1;
                bool right  = (x < width - 1)    && tiles[x + 1][y] == 1;
                bool topLeft     = (x > 0 && y > 0)                   && tiles[x - 1][y - 1] == 1;
                bool topRight    = (x < width - 1 && y > 0)           && tiles[x + 1][y - 1] == 1;
                bool bottomLeft  = (x > 0 && y < height - 1)          && tiles[x - 1][y + 1] == 1;
                bool bottomRight = (x < width - 1 && y < height - 1)  && tiles[x + 1][y + 1] == 1;

                int idx = 0; // Default texture index

                // Logic to select the correct tile texture for seamless rendering
                // This is a simplified example; a full tile-mapping system would be more complex.
                if (!top && !left && !right && !bottom) idx = 15; // Isolated
                else if (!top && !left && !right)       idx = 4; // Top only
                else if (!top && !left && !bottom)      idx = 5; // Top-left corner
                else if (!top && !right && !bottom)     idx = 7; // Top-right corner
                else if (!left && !right && !bottom)    idx = 24; // Bottom only
                else if (!top && !left)                 idx = 0; // Top-left edge
                else if (!top && !right)                idx = 3; // Top-right edge
                else if (!top)                          idx = 1; // Top edge
                else if (!right && !left)               idx = 14; // Horizontal line
                else if (!right && !bottom)             idx = 103; // Bottom-right corner
                else if (!left && !bottom)              idx = 104; // Bottom-left corner
                else if (!right)                        idx = 13; // Right edge
                else if (!left)                         idx = 10; // Left edge
                else if (!bottom)                       idx = 105; // Bottom edge
                else                                    idx = 11; // Fully surrounded

                DrawTexture(tileTextures[idx], x * 32, y * 32, WHITE);
            } else if (tiles[x][y] == 4) { // Glitching-in tile (becomes solid)
                // Draw with increasing alpha to show transition
                float alpha = transitionTimers[x][y] / 0.5f;
                if (alpha > 1.0f) alpha = 1.0f;
                int r = GetRandomValue(180, 255);
                int g = GetRandomValue(0, 255);
                int b = GetRandomValue(180, 255);
                Color glitchColor = { (unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)(alpha * 255) };
                DrawRectangle(x * 32, y * 32, 32, 32, glitchColor);
            } else if (tiles[x][y] == 5) { // Glitching-out tile (becomes empty)
                // Draw with decreasing alpha to show transition
                float alpha = 1.0f - (transitionTimers[x][y] / 0.5f);
                if (alpha < 0.0f) alpha = 0.0f;
                int r = GetRandomValue(0, 255);
                int g = GetRandomValue(180, 255);
                int b = GetRandomValue(0, 180);
                Color glitchColor = { (unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)(alpha * 255) };
                DrawRectangle(x * 32, y * 32, 32, 32, glitchColor);
            } else if (tiles[x][y] == 2) { // Ladder tile
                DrawRectangle(x * 32 + 10, y * 32, 12, 32, GOLD); // Simple gold rectangle
            } else if (tiles[x][y] == 3) { // Rope tile
                DrawRectangle(x * 32 + 14, y * 32, 4, 32, SKYBLUE); // Simple sky blue rectangle
            }
        }
    }
}

// Applies Conway's Game of Life rules to dynamic (non-original) tiles
void Map::applyConwayAutomata() {
    TraceLog(LOG_INFO, "Conway Automata step!");

    std::vector<std::vector<int>> nextTiles = tiles; // Create a copy for the next state
    std::random_device rd;
    std::mt19937 gen(rd());

    std::vector<std::pair<int, int>> dynamicTiles;
    // Collect only dynamic tiles (not original solid, ladders, or ropes)
    for (int x = 1; x < width - 1; ++x) {
        for (int y = 1; y < height - 1; ++y) {
            // IMPORTANT: Conway rules are NOT applied to tiles marked as isOriginalSolid,
            // nor to ladders (2) or ropes (3). This ensures the core room structure remains intact.
            if (!isOriginalSolid[x][y] && tiles[x][y] != 2 && tiles[x][y] != 3) {
                dynamicTiles.push_back({x, y});
            }
        }
    }

    std::shuffle(dynamicTiles.begin(), dynamicTiles.end(), gen); // Randomize processing order

    // Process a subset of dynamic tiles to make changes less abrupt
    size_t numToProcess = dynamicTiles.size() * 3 / 10; // Process 30% of dynamic tiles
    if (numToProcess == 0 && !dynamicTiles.empty()) {
        numToProcess = 1; // Ensure at least one tile is processed if available
    }

    // Apply Conway's rules to the selected dynamic tiles
    for (size_t i = 0; i < numToProcess; ++i) {
        int x = dynamicTiles[i].first;
        int y = dynamicTiles[i].second;

        int liveNeighbors = 0;
        // Count live neighbors (solid tiles or glitching-in tiles)
        for (int dx = -1; dx <= 1; ++dx) {
            for (int dy = -1; dy <= 1; ++dy) {
                if (dx == 0 && dy == 0) continue; // Skip self
                int nx = x + dx, ny = y + dy;

                if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                    // A neighbor is "live" if it's solid (1) or currently glitching in (4)
                    // Removed 'isOriginalSolid[nx][ny]' from this check as it's for filtering, not Conway's rule.
                    if (tiles[nx][ny] == 1 || tiles[nx][ny] == 4) {
                        liveNeighbors++;
                    }
                }
            }
        }

        bool currentTileIsAlive = (tiles[x][y] == 1 || tiles[x][y] == 4); // Current tile is "live" if solid or glitching in

        if (currentTileIsAlive) {
            // Rule 1 & 3: Underpopulation or Overpopulation -> Dies
            if (liveNeighbors < 2 || liveNeighbors > 3) {
                if (tiles[x][y] != 5) { // If not already glitching out
                    nextTiles[x][y] = 5; // Start glitching out
                    transitionTimers[x][y] = 0.0f;
                }
            } else {
                // Rule 2: Lives on to the next generation
                if (tiles[x][y] == 5) { // If it was glitching out but now meets living conditions
                    nextTiles[x][y] = 4; // Start glitching in (reversing)
                    transitionTimers[x][y] = 0.0f;
                } else { 
                    nextTiles[x][y] = tiles[x][y]; // Maintain current state (1 or 4)
                }
            }
        } else { // Current tile is "dead" (empty)
            // Rule 4: Reproduction -> Becomes alive
            if (liveNeighbors == 3) {
                if (tiles[x][y] != 4) { // If not already glitching in
                    nextTiles[x][y] = 4; // Start glitching in
                    transitionTimers[x][y] = 0.0f;
                }
            } else {
                // Stays dead
                if (tiles[x][y] == 4) { // If it was glitching in but now meets dead conditions
                    nextTiles[x][y] = 5; // Start glitching out (reversing)
                    transitionTimers[x][y] = 0.0f;
                } else { 
                    nextTiles[x][y] = tiles[x][y]; // Maintain current state (0 or 5)
                }
            }
        }
    }
    tiles = nextTiles; // Update the map with the new state
}

// Updates the transition timers for glitching tiles
void Map::updateTransitions(float dt) {
    const float TRANSITION_TIME = 0.5f; // Time for a tile to fully transition
    for (int x = 1; x < width - 1; ++x) {
        for (int y = 1; y < height - 1; ++y) {
            if (tiles[x][y] == 4 || tiles[x][y] == 5) { // If tile is in a transition state
                transitionTimers[x][y] += dt;
                if (transitionTimers[x][y] >= TRANSITION_TIME) {
                    if (tiles[x][y] == 4) {
                        tiles[x][y] = 1; // Glitching in becomes solid
                        // IMPORTANT FIX: Mark newly formed solid tiles as original solid
                        // This prevents them from being affected by Conway's rules in subsequent steps.
                        isOriginalSolid[x][y] = true; 
                    }
                    if (tiles[x][y] == 5) tiles[x][y] = 0; // Glitching out becomes empty
                    transitionTimers[x][y] = 0.0f; // Reset timer
                }
            } else {
                transitionTimers[x][y] = 0.0f; // Reset timer for non-transitioning tiles
            }
        }
    }
}

// Checks if a given position collides with a solid ground tile
bool Map::collidesWithGround(Vector2 pos) const {
    int tx = static_cast<int>(pos.x / 32);
    int ty = static_cast<int>(pos.y / 32);
    if (tx >= 0 && tx < width && ty >= 0 && ty < height) {
        return tiles[tx][ty] == 1; // Check if it's a solid tile
    }
    return false;
}

// Checks if a tile at (x, y) is solid
bool Map::isSolidTile(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) return false;
    return tiles[x][y] == 1;
}

// Checks if a tile at (x, y) is a ladder
bool Map::isLadderTile(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) return false;
    return tiles[x][y] == 2;
}

// Checks if a tile at (x, y) is a rope
bool Map::isRopeTile(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) return false;
    return tiles[x][y] == 3;
}

// Returns the height of the map in tiles
int Map::getHeight() const {
    return height;
}

// Destructor: Unloads all loaded textures
Map::~Map() {
    for (const auto& texture : tileTextures) {
        UnloadTexture(texture);
    }
}

// Finds a suitable empty spawn point for a player
Vector2 Map::findEmptySpawn() const {
    int totalEmpty = countEmptyTiles(); // Total empty tiles in the map
    // Iterate through tiles to find a suitable spawn point
    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            // A suitable spawn point is an empty tile with a solid tile directly below it
            if (tiles[x][y] == 0 && y + 1 < height && tiles[x][y + 1] == 1) {
                // Check if a significant portion of empty tiles are reachable from this point
                int reachable = countReachableEmptyTiles(x, y);
                if (reachable >= totalEmpty * 0.8f) { // If 80% or more empty tiles are reachable
                    return { x * 32.0f, y * 32.0f }; // Return pixel coordinates
                }
            }
        }
    }
    return { 32.0f, 32.0f }; // Fallback spawn point
}

// Checks if a tile at (x, y) is empty
bool Map::isTileEmpty(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) return false;
    return tiles[x][y] == 0;
}

// Counts the total number of empty tiles in the map
int Map::countEmptyTiles() const {
    int count = 0;
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            if (tiles[x][y] == 0) count++;
    return count;
}

// Counts the number of empty tiles reachable from a starting point using a flood fill algorithm
int Map::countReachableEmptyTiles(int startX, int startY) const {
    std::vector<std::vector<bool>> visited(width, std::vector<bool>(height, false));
    std::stack<std::pair<int, int>> s;
    s.push({startX, startY});
    int reachable = 0;

    while (!s.empty()) {
        auto [x, y] = s.top(); s.pop();
        // Boundary and visited checks
        if (x < 0 || x >= width || y < 0 || y >= height) continue;
        if (visited[x][y]) continue;
        if (tiles[x][y] != 0) continue; // Only traverse empty tiles
        
        visited[x][y] = true;
        reachable++;

        // Push neighbors to stack
        s.push({x+1, y});
        s.push({x-1, y});
        s.push({x, y+1});
        s.push({x, y-1});
    }
    return reachable;
}
