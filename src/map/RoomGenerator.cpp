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

using namespace MapConstants;

void RoomGenerator::generateRoomsAndConnections(Map& map, std::mt19937& gen, std::function<void(float)> progressCallback) {
    try {
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
        RoomGridGenerator::createRoomGrid(map, gen, rooms_vector, room_grid, num_cols, num_rows);
        map.generatedRooms = rooms_vector;

        if (progressCallback) progressCallback(0.75f);
        

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

        if (progressCallback) progressCallback(0.8f);
        RoomGridGenerator::clearRoomAreas(map, rooms_vector);
        
        if (progressCallback) progressCallback(0.85f);
        RoomConnectionGenerator::createConnections(map, gen, room_grid, num_cols, num_rows, ladders_to_place, ropes_to_place);
        
        if (progressCallback) progressCallback(0.9f);
        generateAllRoomContent(map, rooms_vector, gen);
        
        if (progressCallback) progressCallback(0.95f);
        LadderRopePlacer::placeLaddersAndRopes(map, ladders_to_place, ropes_to_place);
        protectEmptyTilesNearWalls(map);
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
    

    size_t numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 2;
    
    size_t chunkSizeX = (map.getWidth() + numThreads - 1) / numThreads;
    std::vector<std::future<void>> wallProtectionFutures;
    
    for (size_t t = 0; t < numThreads; ++t) {
        size_t startX = t * chunkSizeX;
        size_t endX = std::min(startX + chunkSizeX, (size_t)map.getWidth());
        
        if (startX < endX) {
            wallProtectionFutures.push_back(GlobalThreadPool::getInstance().getMainPool().enqueue(
                [&map, startX, endX]() {
                    for (size_t x = startX; x < endX; ++x) {
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
                }));
        }
    }
    
    // Wait for all wall protection to complete
    for (auto& future : wallProtectionFutures) {
        future.get();
    }
}
