#include "map/RoomGenerator.hpp"
#include "map/RoomContentGenerator.hpp"
#include "map/RoomGridGenerator.hpp"
#include "map/RoomConnectionGenerator.hpp"
#include "map/LadderRopePlacer.hpp"
#include "core/GlobalThreadPool.hpp"
#include <cstdio>
#include <future>
#include <thread>
#include <algorithm>
#include <atomic>
#include <chrono>

using namespace MapConstants;

void RoomGenerator::generateRoomsAndConnections(Map& map, std::mt19937& gen, std::function<void(float)> progressCallback) {
    try {
        auto start = std::chrono::high_resolution_clock::now();
        
        int num_cols = (map.getWidth() - 2) / (MIN_ROOM_SLOT_WIDTH_CONST + SLOT_GAP_SIZE_CONST);
        int num_rows = (map.getHeight() - 2) / (MIN_ROOM_SLOT_HEIGHT_CONST + SLOT_GAP_SIZE_CONST);

        if (num_cols <= 0 || num_rows <= 0) {
            if (progressCallback) progressCallback(1.0f);
            return;
        }

        std::vector<Room> rooms_vector;
        std::vector<Ladder> ladders_to_place;
        std::vector<Rope> ropes_to_place;
        std::vector<std::vector<Room*>> room_grid(num_cols, std::vector<Room*>(num_rows, nullptr));

        if (progressCallback) progressCallback(0.7f);
        printf("[Map] Creating room grid...\n");
        RoomGridGenerator::createRoomGrid(map, gen, rooms_vector, room_grid, num_cols, num_rows);
        map.generatedRooms = rooms_vector;

        auto after_room_grid = std::chrono::high_resolution_clock::now();
        auto room_grid_time = std::chrono::duration_cast<std::chrono::milliseconds>(after_room_grid - start).count();
        printf("[Map] Room grid created in %ld ms\n", room_grid_time);

        if (progressCallback) progressCallback(0.75f);
        
        printf("[Map] Filling tiles with default values...\n");
        size_t numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0) numThreads = 2;
        
        size_t chunkSizeX = (map.getWidth() - 2 + numThreads - 1) / numThreads;
        std::vector<std::future<void>> tileFillFutures;
        
        for (size_t t = 0; t < numThreads; ++t) {
            size_t startX = 1 + t * chunkSizeX;
            size_t endX = std::min(startX + chunkSizeX, (size_t)(map.getWidth() - 1));
            
            if (startX < endX) {
                tileFillFutures.push_back(GlobalThreadPool::getInstance().getMainPool().enqueue([&map, startX, endX]() {
                    for (size_t x_coord = startX; x_coord < endX; ++x_coord) {
                        for (int y_coord = 1; y_coord < map.getHeight() - 1; ++y_coord) {
                            map.tiles[x_coord][y_coord] = DEFAULT_TILE_VALUE;
                            map.isOriginalSolid[x_coord][y_coord] = true;
                        }
                    }
                }));
            }
        }
        
        for (auto& future : tileFillFutures) {
            future.get();
        }
        
        auto after_tile_fill = std::chrono::high_resolution_clock::now();
        auto tile_fill_time = std::chrono::duration_cast<std::chrono::milliseconds>(after_tile_fill - after_room_grid).count();
        printf("[Map] Tile filling completed in %ld ms\n", tile_fill_time);

        if (progressCallback) progressCallback(0.8f);
        printf("[Map] Clearing room areas...\n");
        RoomGridGenerator::clearRoomAreas(map, rooms_vector);
        
        auto after_clear_rooms = std::chrono::high_resolution_clock::now();
        auto clear_rooms_time = std::chrono::duration_cast<std::chrono::milliseconds>(after_clear_rooms - after_tile_fill).count();
        printf("[Map] Room areas cleared in %ld ms\n", clear_rooms_time);
        
        if (progressCallback) progressCallback(0.85f);
        printf("[Map] Creating connections...\n");
        RoomConnectionGenerator::createConnections(map, gen, room_grid, num_cols, num_rows, ladders_to_place, ropes_to_place);
        
        auto after_connections = std::chrono::high_resolution_clock::now();
        auto connections_time = std::chrono::duration_cast<std::chrono::milliseconds>(after_connections - after_clear_rooms).count();
        printf("[Map] Connections created in %ld ms\n", connections_time);
        
        if (progressCallback) progressCallback(0.9f);
        printf("[Map] Generating room content...\n");
        generateAllRoomContent(map, rooms_vector, gen);
        
        auto after_content = std::chrono::high_resolution_clock::now();
        auto content_time = std::chrono::duration_cast<std::chrono::milliseconds>(after_content - after_connections).count();
        printf("[Map] Room content generated in %ld ms\n", content_time);
        
        if (progressCallback) progressCallback(0.95f);
        printf("[Map] Placing ladders and ropes...\n");
        LadderRopePlacer::placeLaddersAndRopes(map, ladders_to_place, ropes_to_place);
        
        auto after_ladders = std::chrono::high_resolution_clock::now();
        auto ladders_time = std::chrono::duration_cast<std::chrono::milliseconds>(after_ladders - after_content).count();
        printf("[Map] Ladders and ropes placed in %ld ms\n", ladders_time);
        
        printf("[Map] Protecting empty tiles near walls...\n");
        protectEmptyTilesNearWalls(map);
        
        auto after_protect = std::chrono::high_resolution_clock::now();
        auto protect_time = std::chrono::duration_cast<std::chrono::milliseconds>(after_protect - after_ladders).count();
        printf("[Map] Wall protection completed in %ld ms\n", protect_time);
        
        auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(after_protect - start).count();
        printf("[Map] Total room generation time: %ld ms\n", total_time);
    } catch (...) {

        if (progressCallback) progressCallback(1.0f);
        throw; 
    }

    size_t numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 2;
    
    std::atomic<int> chestCount{0};
    size_t chunkSizeX = (map.getWidth() + numThreads - 1) / numThreads;
    std::vector<std::future<void>> chestCountFutures;
    
    for (size_t t = 0; t < numThreads; ++t) {
        size_t startX = t * chunkSizeX;
        size_t endX = std::min(startX + chunkSizeX, (size_t)map.getWidth());
        
        if (startX < endX) {
            chestCountFutures.push_back(GlobalThreadPool::getInstance().getMainPool().enqueue(
                [&map, &chestCount, startX, endX]() {
                    int localCount = 0;
                    for (size_t x = startX; x < endX; ++x) {
                        for (int y = 0; y < map.getHeight(); ++y) {
                            if (map.tiles[x][y] == CHEST_TILE_VALUE) {
                                localCount++;
                            }
                        }
                    }
                    chestCount += localCount;
                }));
        }
    }

    for (auto& future : chestCountFutures) {
        future.get();
    }
    
}

void RoomGenerator::generateAllRoomContent(Map& map, const std::vector<Room>& rooms_vector, std::mt19937& gen) {

    size_t numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 2;

    numThreads = std::min(numThreads, rooms_vector.size());
    
    if (numThreads <= 1 || rooms_vector.size() <= 1) {

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
        return;
    }

    std::vector<std::mt19937> threadGens;
    std::random_device rd;
    for (size_t i = 0; i < numThreads; ++i) {
        threadGens.emplace_back(rd());
    }
    
    size_t roomsPerThread = (rooms_vector.size() + numThreads - 1) / numThreads;
    std::vector<std::future<void>> roomContentFutures;
    
    for (size_t t = 0; t < numThreads; ++t) {
        size_t startIdx = t * roomsPerThread;
        size_t endIdx = std::min(startIdx + roomsPerThread, rooms_vector.size());
        
        if (startIdx < endIdx) {
            roomContentFutures.push_back(GlobalThreadPool::getInstance().getMainPool().enqueue(
                [&map, &rooms_vector, &threadGens, t, startIdx, endIdx]() {
                    auto& threadGen = threadGens[t];
                    for (size_t i = startIdx; i < endIdx; ++i) {
                        const auto& room = rooms_vector[i];
                        if (room.type == Room::TREASURE) {
                            RoomContentGenerator::generateTreasureRoomContent(map, room, threadGen);
                        } else if (room.type == Room::SHOP) {
                            RoomContentGenerator::generateShopRoomContent(map, room, threadGen);
                        } else {
                            RoomContentGenerator::generateRoomContent(map, room, threadGen);
                        }
                    }
                }));
        }
    }

    for (auto& future : roomContentFutures) {
        future.get();
    }
}

void RoomGenerator::protectEmptyTilesNearWalls(Map& map) {
    using namespace MapConstants;
    
    printf("[Map] Starting wall protection optimization...\n");

    std::vector<std::pair<int, int>> wallPositions;
    wallPositions.reserve(map.getWidth() * map.getHeight() / 10); 
    
    for (int x = 0; x < map.getWidth(); ++x) {
        for (int y = 0; y < map.getHeight(); ++y) {
            if (map.tiles[x][y] == WALL_TILE_VALUE) {
                wallPositions.emplace_back(x, y);
            }
        }
    }
    
    printf("[Map] Found %zu wall tiles to process\n", wallPositions.size());
    
    size_t numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 2;

    size_t chunkSize = (wallPositions.size() + numThreads - 1) / numThreads;
    std::vector<std::future<void>> wallProtectionFutures;
    
    for (size_t t = 0; t < numThreads; ++t) {
        size_t startIdx = t * chunkSize;
        size_t endIdx = std::min(startIdx + chunkSize, wallPositions.size());
        
        if (startIdx < endIdx) {
            wallProtectionFutures.push_back(GlobalThreadPool::getInstance().getMainPool().enqueue(
                [&map, &wallPositions, startIdx, endIdx]() {

                    constexpr int PROTECTION_RADIUS = 2; 
                    
                    for (size_t i = startIdx; i < endIdx; ++i) {
                        const auto& [wallX, wallY] = wallPositions[i];

                        for (int dx = -PROTECTION_RADIUS; dx <= PROTECTION_RADIUS; ++dx) {
                            for (int dy = -PROTECTION_RADIUS; dy <= PROTECTION_RADIUS; ++dy) {
                                int nx = wallX + dx;
                                int ny = wallY + dy;

                                if (nx <= 0 || nx >= map.getWidth() - 1 || ny <= 0 || ny >= map.getHeight() - 1) {
                                    continue;
                                }

                                if (map.tiles[nx][ny] == EMPTY_TILE_VALUE) {
                                    map.tiles[nx][ny] = PROTECTED_EMPTY_TILE_VALUE;
                                }
                            }
                        }
                    }
                }));
        }
    }
    

    for (auto& future : wallProtectionFutures) {
        future.get();
    }
    
    printf("[Map] Wall protection optimization complete\n");
}
