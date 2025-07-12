#include "map/RoomContentGenerator.hpp"

#include <algorithm>
#include <vector>
#include <tuple>
#include <cstdio>

using namespace MapConstants;

void RoomContentGenerator::generateRoomContent(Map& map, const Room& room, std::mt19937& gen) {
    generatePlatforms(map, room, gen);
    generateWalls(map, room, gen);
    generateLaddersAndRopes(map, room, gen);
    
    // Generate lava pockets in every room for testing
    generateLavaPockets(map, room, gen);
}

void RoomContentGenerator::generatePlatforms(Map& map, const Room& room, std::mt19937& gen) {
    const int room_width = room.endX - room.startX;
    int platY = (room.startY + room.endY) / 2;
    if (platY > room.startY + 1 && platY < room.endY - 1) {
        for (int x = room.startX + 2; x <= room.endX - 2; ++x) {
            if (map.tiles[x][platY] == EMPTY_TILE_VALUE) {
                map.tiles[x][platY] = PLATFORM_TILE_VALUE;
                map.isOriginalSolid[x][platY] = false;
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
            if (py > room.startY + 1 && py < room.endY - 1 && map.tiles[x_coord][py] == EMPTY_TILE_VALUE) {
                map.tiles[x_coord][py] = PLATFORM_TILE_VALUE;
                map.isOriginalSolid[x_coord][py] = false;
            }
        }
    }
}

void RoomContentGenerator::generateWalls(Map& map, const Room& room, std::mt19937& gen) {
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

                    if (map.tiles[wx][y_coord] == EMPTY_TILE_VALUE) {
                        map.tiles[wx][y_coord] = WALL_TILE_VALUE;
                        map.isOriginalSolid[wx][y_coord] = true;
                    }
                }
            }
        }
    }
}

void RoomContentGenerator::generateLaddersAndRopes(Map& map, const Room& room, std::mt19937& gen) {
    std::vector<std::tuple<int, int, int>> potential_shafts;

    for (int x_col = room.startX + 1; x_col <= room.endX - 1; ++x_col) {
        std::vector<int> platform_ys_in_col;
        for (int y_row = room.startY + 1; y_row <= room.endY - 1; ++y_row) {
            if (map.tiles[x_col][y_row] == WALL_TILE_VALUE || map.tiles[x_col][y_row] == PLATFORM_TILE_VALUE) {
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
                            if (map.tiles[x_col][y_check] != EMPTY_TILE_VALUE) {
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

    std::vector<bool> column_occupied(map.getWidth(), false);

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
                    if (map.tiles[x_col][y_coord] == EMPTY_TILE_VALUE) {
                        map.tiles[x_col][y_coord] = tileType;
                        map.isOriginalSolid[x_col][y_coord] = false;
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

void RoomContentGenerator::generateTreasureRoomContent(Map& map, const Room& room, std::mt19937& gen) {
    const int room_width = room.endX - room.startX;
    const int room_height = room.endY - room.startY;
    int treasureX = room.startX + room_width / 2;
    int treasureY = room.startY + room_height / 2;

    if (map.isInsideBounds(treasureX, treasureY + 1)) {
        if (map.tiles[treasureX][treasureY + 1] == EMPTY_TILE_VALUE) {
            map.tiles[treasureX][treasureY + 1] = DEFAULT_TILE_VALUE;
            map.isOriginalSolid[treasureX][treasureY + 1] = true;
        }
    }

    if (map.isInsideBounds(treasureX, treasureY)) {
        if (map.tiles[treasureX][treasureY] == EMPTY_TILE_VALUE) {
            map.tiles[treasureX][treasureY] = CHEST_TILE_VALUE;
            map.isOriginalSolid[treasureX][treasureY] = false;
        }
    }

    std::uniform_int_distribution<> extraTreasureDist(0, MAX_EXTRA_TREASURES_IN_ROOM);
    int extraTreasures = extraTreasureDist(gen);

    for (int i = 0; i < extraTreasures; ++i) {
        std::uniform_int_distribution<> xDist(room.startX + 1, room.endX - 1);
        std::uniform_int_distribution<> yDist(room.startY + 1, room.endY - 1);
        int x_coord = xDist(gen);
        int y_coord = yDist(gen);
        if (map.isInsideBounds(x_coord, y_coord)) {
            if (map.isInsideBounds(x_coord, y_coord + 1) && map.tiles[x_coord][y_coord + 1] == EMPTY_TILE_VALUE) {
                map.tiles[x_coord][y_coord + 1] = DEFAULT_TILE_VALUE;
                map.isOriginalSolid[x_coord][y_coord + 1] = true;
            }
            if (map.tiles[x_coord][y_coord] == EMPTY_TILE_VALUE) {
                map.tiles[x_coord][y_coord] = CHEST_TILE_VALUE;
                map.isOriginalSolid[x_coord][y_coord] = false;
            }
        }
    }
}

void RoomContentGenerator::generateShopRoomContent(Map& map, const Room& room, std::mt19937& gen) {
    const int room_width = room.endX - room.startX;
    const int room_height = room.endY - room.startY;
    int shopX = room.startX + room_width / 2;
    int shopY = room.startY + room_height / 2;

    if (map.isInsideBounds(shopX, shopY)) {
        if (map.tiles[shopX][shopY] == EMPTY_TILE_VALUE) {
            map.tiles[shopX][shopY] = SHOP_TILE_VALUE;
            map.isOriginalSolid[shopX][shopY] = false;
        }
    }

    std::uniform_int_distribution<> extraShopItemsDist(0, MAX_EXTRA_SHOP_ITEMS_IN_ROOM);
    int extraShopItems = extraShopItemsDist(gen);

    for (int i = 0; i < extraShopItems; ++i) {
        std::uniform_int_distribution<> xDist(room.startX + 1, room.endX - 1);
        std::uniform_int_distribution<> yDist(room.startY + 1, room.endY - 1);
        int x_coord = xDist(gen);
        int y_coord = yDist(gen);
        if (map.tiles[x_coord][y_coord] == EMPTY_TILE_VALUE) {
            map.tiles[x_coord][y_coord] = SHOP_TILE_VALUE;
            map.isOriginalSolid[x_coord][y_coord] = false;
        }
    }
}

void RoomContentGenerator::generateLavaPockets(Map& map, const Room& room, std::mt19937& gen) {
    const int room_width = room.endX - room.startX;
    const int room_height = room.endY - room.startY;
    
    // Create 1-2 lava pockets per room
    std::uniform_int_distribution<> pocketCountDist(1, 2);
    int numPockets = pocketCountDist(gen);
    
    for (int pocket = 0; pocket < numPockets; ++pocket) {
        // Determine pocket size (3x2 to 6x3)
        std::uniform_int_distribution<> widthDist(3, std::min(6, room_width / 2));
        std::uniform_int_distribution<> heightDist(2, std::min(3, room_height / 3));
        int pocketWidth = widthDist(gen);
        int pocketHeight = heightDist(gen);
        
        // Find a suitable location for the pocket
        int maxStartX = room.endX - pocketWidth - 2;
        int maxStartY = room.endY - pocketHeight - 2;
        
        if (maxStartX <= room.startX + 2 || maxStartY <= room.startY + 2) {
            continue; // Room too small for pocket
        }
        
        std::uniform_int_distribution<> startXDist(room.startX + 2, maxStartX);
        std::uniform_int_distribution<> startYDist(room.startY + 2, maxStartY);
        int startX = startXDist(gen);
        int startY = startYDist(gen);
        
        // Create the lava pocket container with platform tiles
        for (int x = startX; x < startX + pocketWidth; ++x) {
            for (int y = startY; y < startY + pocketHeight; ++y) {
                // Create the container walls/bottom with platform tiles
                if (x == startX || x == startX + pocketWidth - 1 || 
                    y == startY + pocketHeight - 1) {
                    if (map.tiles[x][y] == EMPTY_TILE_VALUE) {
                        map.tiles[x][y] = PLATFORM_TILE_VALUE;
                        map.isOriginalSolid[x][y] = false;
                    }
                } else {
                    // Fill interior with lava
                    if (map.tiles[x][y] == EMPTY_TILE_VALUE) {
                        map.tiles[x][y] = LAVA_TILE_VALUE;
                        map.isOriginalSolid[x][y] = false;
                        
                        // Initialize lava cell with full mass
                        if (map.lavaGrid.size() > x && map.lavaGrid[x].size() > y) {
                            map.lavaGrid[x][y] = LavaCell(Map::LAVA_MAX_MASS);
                        }
                    }
                }
            }
        }
    }
}
