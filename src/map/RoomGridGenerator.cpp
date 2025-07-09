#include "map/RoomGridGenerator.hpp"
#include "core/FastRNG.hpp"
#include "core/GlobalThreadPool.hpp"
#include <thread>
#include <mutex>
#include <future>

using namespace MapConstants;

void RoomGridGenerator::createRoomGrid(Map& map, std::mt19937& gen, std::vector<Room>& rooms_vector,
                                      std::vector<std::vector<Room*>>& room_grid, int num_cols, int num_rows) {
    
    // Reserve removed to prevent potential memory issues
    
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

void RoomGridGenerator::clearRoomAreas(Map& map, const std::vector<Room>& rooms_vector) {
    // Use single-threaded approach to prevent race conditions
    // The performance gain from multithreading room clearing is minimal
    // compared to the risk of race conditions causing freezes
    
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

void RoomGridGenerator::createRoomGridOptimized(Map& map, FastRNG& rng, std::vector<Room>& rooms_vector,
                                               std::vector<std::vector<Room*>>& room_grid, int num_cols, int num_rows) {

    // Reserve removed to prevent potential memory issues
    
    const int hardware_threads = std::thread::hardware_concurrency();
    const int max_threads = std::max(1, std::min(hardware_threads, 8)); 
    
    const int target_chunks_per_thread = 2;
    const int total_target_chunks = max_threads * target_chunks_per_thread;
    const int chunk_cols = std::max(1, (num_cols + total_target_chunks - 1) / total_target_chunks);
    const int chunk_rows = std::max(1, (num_rows + total_target_chunks - 1) / total_target_chunks);
  
    std::vector<GridRegion> regions;
    std::vector<std::vector<Room>> local_room_vectors(max_threads);
    std::vector<std::vector<std::vector<Room*>>> local_room_grids(max_threads);
   
    for (int i = 0; i < max_threads; ++i) {
        local_room_vectors[i].reserve(num_cols * num_rows / max_threads + 1);
        local_room_grids[i].resize(num_cols);
        for (int j = 0; j < num_cols; ++j) {
            local_room_grids[i][j].resize(num_rows, nullptr);
        }
    }
    
  
    int region_count = 0;
    for (int start_row = 0; start_row < num_rows; start_row += chunk_rows) {
        for (int start_col = 0; start_col < num_cols; start_col += chunk_cols) {
            int end_row = std::min(start_row + chunk_rows, num_rows);
            int end_col = std::min(start_col + chunk_cols, num_cols);
            
            uint64_t region_seed = rng.next(); 
            regions.push_back({start_col, end_col, start_row, end_row, region_seed});
            region_count++;
        }
    }
    
    std::vector<std::future<void>> futures;
    
    for (size_t i = 0; i < regions.size(); ++i) {
        int thread_id = i % max_threads;
        
        futures.push_back(GlobalThreadPool::getInstance().getMainPool().enqueue([&, i, thread_id]() {
            processRegion(regions[i], map, local_room_grids[thread_id], 
                         local_room_vectors[thread_id], num_cols, num_rows);
        }));
    }
    
    for (auto& future : futures) {
        future.wait();
    }
    
    // Merge results from all threads
    std::mutex merge_mutex;
    for (int thread_id = 0; thread_id < max_threads; ++thread_id) {
        // Merge rooms
        for (const auto& room : local_room_vectors[thread_id]) {
            rooms_vector.push_back(room);
        }
        
        // Merge room grid pointers (need to update pointers to merged rooms)
        for (int c = 0; c < num_cols; ++c) {
            for (int r = 0; r < num_rows; ++r) {
                if (local_room_grids[thread_id][c][r] != nullptr) {
                    // Find the corresponding room in the merged vector
                    Room* local_room = local_room_grids[thread_id][c][r];
                    for (auto& merged_room : rooms_vector) {
                        if (merged_room.startX == local_room->startX && 
                            merged_room.startY == local_room->startY &&
                            merged_room.endX == local_room->endX && 
                            merged_room.endY == local_room->endY) {
                            room_grid[c][r] = &merged_room;
                            break;
                        }
                    }
                }
            }
        }
    }
}

void RoomGridGenerator::processRegion(const GridRegion& region, Map& map, 
                                     std::vector<std::vector<Room*>>& local_room_grid,
                                     std::vector<Room>& local_rooms, int num_cols, int num_rows) {
    FastRNG local_rng(region.seed);
    
    for (int r_idx = region.start_row; r_idx < region.end_row; ++r_idx) {
        for (int c_idx = region.start_col; c_idx < region.end_col; ++c_idx) {
            // Skip if already has a room (from overlapping regions)
            if (local_room_grid[c_idx][r_idx] != nullptr) {
                continue;
            }

            if (local_rng.rollPercent() < ROOM_PLACEMENT_SKIP_CHANCE_PERCENT) {
                continue;
            }

            int current_x = 1 + c_idx * (MIN_ROOM_SLOT_WIDTH_CONST + SLOT_GAP_SIZE_CONST);
            int current_y = 1 + r_idx * (MIN_ROOM_SLOT_HEIGHT_CONST + SLOT_GAP_SIZE_CONST);

            bool is_hall = (local_rng.rollPercent() < LARGE_HALL_CREATION_CHANCE_PERCENT);
            int room_width_slots = 1;
            int room_height_slots = 1;

            // Check for large hall creation (be careful with bounds)
            if (is_hall && c_idx + 1 < region.end_col && r_idx + 1 < region.end_row &&
                c_idx + 1 < num_cols && r_idx + 1 < num_rows &&
                local_room_grid[c_idx+1][r_idx] == nullptr && 
                local_room_grid[c_idx][r_idx+1] == nullptr && 
                local_room_grid[c_idx+1][r_idx+1] == nullptr) {

                int potential_hall_end_x = current_x + (2 * MIN_ROOM_SLOT_WIDTH_CONST + SLOT_GAP_SIZE_CONST) - 1;
                int potential_hall_end_y = current_y + (2 * MIN_ROOM_SLOT_HEIGHT_CONST + SLOT_GAP_SIZE_CONST) - 1;

                if (potential_hall_end_x < map.getWidth() - 1 && potential_hall_end_y < map.getHeight() - 1) {
                    room_width_slots = 2;
                    room_height_slots = 2;
                }
            }

            int extra_width = local_rng.nextUInt(MAX_ROOM_WIDTH_RANDOM_VARIATION + 1);

            int room_pixel_w = room_width_slots * MIN_ROOM_SLOT_WIDTH_CONST + (room_width_slots - 1) * SLOT_GAP_SIZE_CONST + extra_width;
            int room_pixel_h = room_height_slots * MIN_ROOM_SLOT_HEIGHT_CONST + (room_height_slots - 1) * SLOT_GAP_SIZE_CONST;

            if (current_x + room_pixel_w >= map.getWidth() - 1 || current_y + room_pixel_h >= map.getHeight() - 1) {
                continue;
            }
            
            Room::Type room_type_enum = Room::NORMAL;
            int room_type_roll = local_rng.rollPercent();

            if (room_type_roll < ROOM_TYPE_TREASURE_CHANCE_THRESHOLD_PERCENT) {
                room_type_enum = Room::TREASURE;
            } else if (room_type_roll < ROOM_TYPE_SHOP_CHANCE_THRESHOLD_PERCENT) {
                room_type_enum = Room::SHOP;
            }

            local_rooms.emplace_back(current_x, current_y, current_x + room_pixel_w - 1, current_y + room_pixel_h - 1, room_type_enum);
            Room* room_ptr = &local_rooms.back();


            for (int dr = 0; dr < room_height_slots; ++dr) {
                for (int dc = 0; dc < room_width_slots; ++dc) {
                    if (c_idx + dc < num_cols && r_idx + dr < num_rows) {
                        local_room_grid[c_idx + dc][r_idx + dr] = room_ptr;
                    }
                }
            }
        }
    }
}
