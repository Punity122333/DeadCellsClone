#include "map/RoomConnectionGenerator.hpp"
#include <set>
#include <algorithm>
#include <climits>
#include <cmath>


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

} // anonymous namespace

std::vector<std::pair<Room*, Room*>> RoomConnectionGenerator::findMinimumSpanningConnections(const std::vector<Room*>& rooms) {
    std::vector<std::tuple<float, int, int>> edges;
    for (int i = 0; i < rooms.size(); ++i) {
        for (int j = i + 1; j < rooms.size(); ++j) {
            edges.emplace_back(RoomConnectionGenerator::getRoomDistance(rooms[i], rooms[j]), i, j);
        }
    }

    std::sort(edges.begin(), edges.end());
    UnionFind uf(rooms.size());
    std::vector<std::pair<Room*, Room*>> connections;

    for (const auto& [dist, i, j] : edges) {
        if (uf.unite(i, j)) {
            connections.emplace_back(rooms[i], rooms[j]);
        }
    }

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

    int horizontal_y = std::max(room1->endY - 2, room2->endY - 2);
    for (int y_offset = -corridor_width / 2; y_offset <= corridor_width / 2; ++y_offset) {
        spans.emplace_back(x_start, horizontal_y + y_offset, x_end - x_start + 1, EMPTY_TILE_VALUE, true);
    }


    int y_start = std::min(static_cast<int>(c1.y), horizontal_y);
    int y_end = std::max(static_cast<int>(c2.y), horizontal_y);
    for (int x_offset = -corridor_width / 2; x_offset <= corridor_width / 2; ++x_offset) {
        spans.emplace_back(c2.x + x_offset, y_start, y_end - y_start + 1, EMPTY_TILE_VALUE, false);
    }

    if (std::abs(static_cast<int>(c1.y) - horizontal_y) > corridor_width || std::abs(static_cast<int>(c2.y) - horizontal_y) > corridor_width) {
        int tileType = (std::uniform_int_distribution<>(0, LADDER_OR_ROPE_ROLL_MAX)(gen) == 0) ? LADDER_TILE_VALUE : ROPE_TILE_VALUE;
        if (tileType == LADDER_TILE_VALUE) {
            ladders.emplace_back(c2.x, y_start, y_end);
        } else {
            ropes.emplace_back(c2.x, y_start, y_end);
        }
    }

    return {room1, room2, spans, ladders, ropes};
}

void RoomConnectionGenerator::executeCorridorTasks(Map& map, const std::vector<CorridorTask>& tasks,
                                                  std::vector<Ladder>& ladders_to_place, std::vector<Rope>& ropes_to_place) {
    for (const auto& task : tasks) {
        for (const auto& span : task.spans) {
            for (int i = 0; i < span.length; ++i) {
                int x = span.x + (span.isHorizontal ? i : 0);
                int y = span.y + (span.isHorizontal ? 0 : i);
                if (map.isInsideBounds(x, y)) {
                    map.tiles[x][y] = span.tileType;
                    map.isOriginalSolid[x][y] = false;
                }
            }
        }

        for (const auto& ladder : task.ladders) {
            ladders_to_place.push_back(ladder);
        }
        for (const auto& rope : task.ropes) {
            ropes_to_place.push_back(rope);
        }
    }
}

void RoomConnectionGenerator::createConnections(Map& map, std::mt19937& gen, const std::vector<std::vector<Room*>>& room_grid,
                                               int num_cols, int num_rows, std::vector<Ladder>& ladders_to_place,
                                               std::vector<Rope>& ropes_to_place) {
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

    if (unique_rooms.size() <= 1) return;

    auto mst_edges = RoomConnectionGenerator::findMinimumSpanningConnections(unique_rooms);
    std::vector<CorridorTask> tasks;

    for (auto& [room1, room2] : mst_edges) {
        tasks.push_back(RoomConnectionGenerator::generateCorridorTask(room1, room2, gen));
    }

    RoomConnectionGenerator::executeCorridorTasks(map, tasks, ladders_to_place, ropes_to_place);
}
