#include "map/RoomConnectionGenerator.hpp"
#include "core/GlobalThreadPool.hpp"
#include <set>
#include <algorithm>
#include <climits>
#include <cmath>
#include <limits>
#include <cstdio>
#include <future>
#include <atomic>
#include <thread>

using namespace MapConstants;

namespace {
    Vector2 getRoomCenter(Room* room) {
        return {static_cast<float>(room->startX + room->endX) / 2.0f, 
                static_cast<float>(room->startY + room->endY) / 2.0f};
    }

    float getRoomDistance(Room* a, Room* b) {
        Vector2 ca = getRoomCenter(a);
        Vector2 cb = getRoomCenter(b);
        return std::abs(ca.x - cb.x) + std::abs(ca.y - cb.y);
    }

    std::unique_ptr<KDNode> buildKDTree(std::vector<Room*> rooms, int depth = 0) {
        if (rooms.empty()) return nullptr;
        int axis = depth % 2;

        std::sort(rooms.begin(), rooms.end(), [axis](Room* a, Room* b) {
            return (axis == 0 ? getRoomCenter(a).x : getRoomCenter(a).y) <
                   (axis == 0 ? getRoomCenter(b).x : getRoomCenter(b).y);
        });

        size_t median = rooms.size() / 2;
        auto node = std::make_unique<KDNode>(rooms[median], axis);

        std::vector<Room*> left_rooms(rooms.begin(), rooms.begin() + median);
        std::vector<Room*> right_rooms(rooms.begin() + median + 1, rooms.end());

        node->left = buildKDTree(left_rooms, depth + 1);
        node->right = buildKDTree(right_rooms, depth + 1);

        return node;
    }

    Room* findNearestRoom(const std::unique_ptr<KDNode>& node, Room* target, Room* best = nullptr,
                          float bestDist = std::numeric_limits<float>::max(), int depth = 0) {
        if (!node) return best;

        float dist = getRoomDistance(target, node->room);
        if (dist < bestDist && target != node->room) {
            best = node->room;
            bestDist = dist;
        }

        int axis = depth % 2;
        float diff = (axis == 0 ? getRoomCenter(target).x - getRoomCenter(node->room).x
                                : getRoomCenter(target).y - getRoomCenter(node->room).y);

        std::unique_ptr<KDNode> const& first = (diff < 0 ? node->left : node->right);
        std::unique_ptr<KDNode> const& second = (diff < 0 ? node->right : node->left);

        best = findNearestRoom(first, target, best, bestDist, depth + 1);
        if (std::abs(diff) < bestDist) {
            best = findNearestRoom(second, target, best, bestDist, depth + 1);
        }

        return best;
    }
}

std::vector<std::pair<Room*, Room*>> RoomConnectionGenerator::findMinimumSpanningConnections(const std::vector<Room*>& rooms) {
    if (rooms.size() <= 1) return {};
    
    printf("[Map] Optimizing connections for %zu rooms...\n", rooms.size());
    
    std::vector<std::pair<Room*, Room*>> connections;
    
    if (rooms.size() > 50) {
        printf("[Map] Using grid-based approach for large room count\n");
        
        printf("[Map] Sorting rooms by position...\n");
        std::vector<Room*> sorted_rooms = rooms;
        std::sort(sorted_rooms.begin(), sorted_rooms.end(), [](Room* a, Room* b) {
            Vector2 ca = {static_cast<float>(a->startX + a->endX) / 2.0f, 
                         static_cast<float>(a->startY + a->endY) / 2.0f};
            Vector2 cb = {static_cast<float>(b->startX + b->endX) / 2.0f, 
                         static_cast<float>(b->startY + b->endY) / 2.0f};
            if (std::abs(ca.y - cb.y) > 50) return ca.y < cb.y;
            return ca.x < cb.x;
        });
        printf("[Map] Room sorting completed\n");
        
        printf("[Map] Creating chain connections...\n");
        for (size_t i = 0; i < sorted_rooms.size() - 1; ++i) {
            connections.emplace_back(sorted_rooms[i], sorted_rooms[i + 1]);
            
            if (i % 50 == 0 || i == sorted_rooms.size() - 2) {
                printf("[Map] Created %zu/%zu chain connections\n", i + 1, sorted_rooms.size() - 1);
            }
        }
        
        printf("[Map] Adding cross-connections...\n");
        size_t cross_connections = std::min(size_t(10), rooms.size() / 10);
        for (size_t i = 0; i < cross_connections && i < sorted_rooms.size() - 2; ++i) {
            size_t target_idx = (i * 7) % (sorted_rooms.size() - 1);
            if (target_idx != i && target_idx != i + 1) {
                connections.emplace_back(sorted_rooms[i], sorted_rooms[target_idx]);
            }
        }
        printf("[Map] Cross-connections completed\n");
    } else {
        printf("[Map] Using nearest-neighbor approach for small room count\n");
        
        std::set<Room*> connected;
        connected.insert(rooms[0]);
        
        while (connected.size() < rooms.size()) {
            Room* bestUnconnected = nullptr;
            Room* bestConnected = nullptr;
            float bestDistance = std::numeric_limits<float>::max();
            
            for (Room* unconnected : rooms) {
                if (connected.find(unconnected) != connected.end()) continue;
                
                for (Room* connectedRoom : connected) {
                    float dist = getRoomDistance(unconnected, connectedRoom);
                    if (dist < bestDistance) {
                        bestDistance = dist;
                        bestUnconnected = unconnected;
                        bestConnected = connectedRoom;
                    }
                }
            }
            
            if (bestUnconnected) {
                connections.emplace_back(bestConnected, bestUnconnected);
                connected.insert(bestUnconnected);
                
                if (connected.size() % 10 == 0 || connected.size() == rooms.size()) {
                    printf("[Map] Connected %zu/%zu rooms\n", connected.size(), rooms.size());
                }
            }
        }
    }
    
    printf("[Map] Created %zu connections efficiently\n", connections.size());
    return connections;
}

Vector2 RoomConnectionGenerator::getRoomCenter(Room* room) {
    return {static_cast<float>(room->startX + room->endX) / 2.0f, 
            static_cast<float>(room->startY + room->endY) / 2.0f};
}

float RoomConnectionGenerator::getRoomDistance(Room* a, Room* b) {
    Vector2 ca = getRoomCenter(a);
    Vector2 cb = getRoomCenter(b);
    return std::abs(ca.x - cb.x) + std::abs(ca.y - cb.y);
}

CorridorTask RoomConnectionGenerator::generateCorridorTask(Room* room1, Room* room2, std::mt19937& gen) {
    Vector2 c1 = getRoomCenter(room1);
    Vector2 c2 = getRoomCenter(room2);

    std::vector<DrawSpan> spans;
    std::vector<Ladder> ladders;
    std::vector<Rope> ropes;

    int corridor_width = 3;

    int x_start = std::min(c1.x, c2.x);
    int x_end = std::max(c1.x, c2.x);
    int horizontal_length = x_end - x_start + 1;

    horizontal_length = std::min(horizontal_length, 200);
    x_end = x_start + horizontal_length - 1;

    if (horizontal_length > 5 && horizontal_length < 500) {
        int horizontal_y = std::max(room1->endY - 2, room2->endY - 2) + 1;
        for (int y_offset = -corridor_width / 2; y_offset <= corridor_width / 2; ++y_offset) {
            spans.emplace_back(x_start, horizontal_y + y_offset, horizontal_length, EMPTY_TILE_VALUE, true);
        }
    }

    int y_start = std::min(static_cast<int>(c1.y), static_cast<int>(c2.y));
    int y_end = std::max(static_cast<int>(c1.y), static_cast<int>(c2.y));
    int vertical_length = y_end - y_start + 1;
    
    vertical_length = std::min(vertical_length, 150);
    y_end = y_start + vertical_length - 1;
    
    if (vertical_length > 5 && vertical_length < 300) {
        for (int x_offset = -corridor_width / 2; x_offset <= corridor_width / 2; ++x_offset) {
            spans.emplace_back(c2.x + x_offset, y_start, vertical_length, EMPTY_TILE_VALUE, false);
        }
    }

    int vertical_diff = std::abs(static_cast<int>(c1.y) - static_cast<int>(c2.y));
    if (vertical_diff > corridor_width && vertical_diff > 8) {
        int max_length = std::min(vertical_diff, 50);
        int actual_y_end = y_start + max_length;
        
        int tileType = (std::uniform_int_distribution<>(0, LADDER_OR_ROPE_ROLL_MAX)(gen) == 0) ? LADDER_TILE_VALUE : ROPE_TILE_VALUE;
        if (tileType == LADDER_TILE_VALUE) {
            ladders.emplace_back(c2.x, y_start, actual_y_end);
        } else {
            ropes.emplace_back(c2.x, y_start, actual_y_end);
        }
    }

    return {room1, room2, spans, ladders, ropes};
}

void RoomConnectionGenerator::executeCorridorTasks(Map& map, const std::vector<CorridorTask>& tasks,
                                                  std::vector<Ladder>& ladders_to_place, std::vector<Rope>& ropes_to_place) {
    printf("[Map] Executing %zu corridor tasks...\n", tasks.size());
    
    if (tasks.empty()) return;
    
    std::atomic<size_t> total_tiles_processed{0};
    std::atomic<size_t> total_ladders{0};
    std::atomic<size_t> total_ropes{0};
    
    std::vector<std::vector<Ladder>> thread_ladders;
    std::vector<std::vector<Rope>> thread_ropes;
    
    size_t numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 8;
    numThreads = std::max(static_cast<size_t>(8), std::min(numThreads * 2, static_cast<size_t>(32)));
    numThreads = std::min(numThreads, tasks.size());
    
    thread_ladders.resize(numThreads);
    thread_ropes.resize(numThreads);
    
    printf("[Map] Using %zu threads for corridor processing\n", numThreads);
    
    int map_width = map.getWidth();
    int map_height = map.getHeight();
    
    size_t chunkSize = (tasks.size() + numThreads - 1) / numThreads;
    std::vector<std::future<void>> futures;
    
    for (size_t t = 0; t < numThreads; ++t) {
        size_t startIdx = t * chunkSize;
        size_t endIdx = std::min(startIdx + chunkSize, tasks.size());
        
        if (startIdx < endIdx) {
            futures.push_back(GlobalThreadPool::getInstance().getMainPool().enqueue(
                [&map, &tasks, &thread_ladders, &thread_ropes, &total_tiles_processed, &total_ladders, &total_ropes, 
                 startIdx, endIdx, t, map_width, map_height]() {
                    size_t local_tiles = 0;
                    size_t local_ladders = 0;
                    size_t local_ropes = 0;
                    
                    for (size_t task_idx = startIdx; task_idx < endIdx; ++task_idx) {
                        const auto& task = tasks[task_idx];
                        
                        for (const auto& span : task.spans) {
                            if (span.length <= 0 || span.length > 1000) continue;
                            
                            if (span.isHorizontal) {
                                int start_x = std::max(1, span.x);
                                int end_x = std::min(map_width - 1, span.x + span.length);
                                int y = span.y;
                                
                                if (y > 0 && y < map_height - 1) {
                                    for (int x = start_x; x < end_x; ++x) {
                                        map.tiles[x][y] = span.tileType;
                                        map.isOriginalSolid[x][y] = false;
                                        local_tiles++;
                                    }
                                }
                            } else {
                                int x = span.x;
                                int start_y = std::max(1, span.y);
                                int end_y = std::min(map_height - 1, span.y + span.length);
                                
                                if (x > 0 && x < map_width - 1) {
                                    for (int y = start_y; y < end_y; ++y) {
                                        map.tiles[x][y] = span.tileType;
                                        map.isOriginalSolid[x][y] = false;
                                        local_tiles++;
                                    }
                                }
                            }
                        }

                        for (const auto& ladder : task.ladders) {
                            thread_ladders[t].push_back(ladder);
                            local_ladders++;
                        }
                        for (const auto& rope : task.ropes) {
                            thread_ropes[t].push_back(rope);
                            local_ropes++;
                        }
                    }
                    
                    total_tiles_processed += local_tiles;
                    total_ladders += local_ladders;
                    total_ropes += local_ropes;
                }));
        }
    }
    
    for (auto& future : futures) {
        future.get();
    }
    
    printf("[Map] Merging thread results...\n");
    for (const auto& thread_ladder_vec : thread_ladders) {
        for (const auto& ladder : thread_ladder_vec) {
            ladders_to_place.push_back(ladder);
        }
    }
    for (const auto& thread_rope_vec : thread_ropes) {
        for (const auto& rope : thread_rope_vec) {
            ropes_to_place.push_back(rope);
        }
    }
    
    printf("[Map] Corridor execution complete: %zu tiles processed, %zu ladders, %zu ropes\n", 
           total_tiles_processed.load(), total_ladders.load(), total_ropes.load());
}

void RoomConnectionGenerator::createConnections(Map& map, std::mt19937& gen, const std::vector<std::vector<Room*>>& room_grid,
                                               int num_cols, int num_rows, std::vector<Ladder>& ladders_to_place,
                                               std::vector<Rope>& ropes_to_place) {
    printf("[Map] Starting connection creation process...\n");
    
    std::vector<Room*> unique_rooms;
    std::set<Room*> seen;

    for (int r = 0; r < num_rows; ++r) {
        for (int c = 0; c < num_cols; ++c) {
            Room* room = room_grid[c][r];
            if (room && seen.insert(room).second) {
                unique_rooms.push_back(room);
            }
        }
    }
    
    printf("[Map] Found %zu unique rooms\n", unique_rooms.size());
    if (unique_rooms.size() <= 1) return;

    printf("[Map] Finding minimum spanning connections...\n");
    auto mst_edges = RoomConnectionGenerator::findMinimumSpanningConnections(unique_rooms);
    printf("[Map] MST algorithm completed, found %zu edges\n", mst_edges.size());
    
    std::vector<CorridorTask> tasks;
    printf("[Map] Generating corridor tasks...\n");

    size_t total_spans = 0;
    size_t max_span_length = 0;
    
    for (size_t i = 0; i < mst_edges.size(); ++i) {
        auto& [room1, room2] = mst_edges[i];
        auto task = RoomConnectionGenerator::generateCorridorTask(room1, room2, gen);
        
        for (const auto& span : task.spans) {
            total_spans++;
            max_span_length = std::max(max_span_length, static_cast<size_t>(span.length));
        }
        
        tasks.push_back(std::move(task));
        
        if (i % 10 == 0 || i == mst_edges.size() - 1) {
            printf("[Map] Generated %zu/%zu corridor tasks\n", i + 1, mst_edges.size());
        }
    }
    
    printf("[Map] Generated %zu total spans, max span length: %zu\n", total_spans, max_span_length);

    printf("[Map] Executing corridor tasks...\n");
    RoomConnectionGenerator::executeCorridorTasks(map, tasks, ladders_to_place, ropes_to_place);
    printf("[Map] Corridor task execution completed\n");
}
