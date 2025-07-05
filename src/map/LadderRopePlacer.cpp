#include "map/LadderRopePlacer.hpp"
#include <cstdio>

using namespace MapConstants;

void LadderRopePlacer::placeLaddersAndRopes(Map& map, const std::vector<Ladder>& ladders_to_place,
                                           const std::vector<Rope>& ropes_to_place) {
    printf("[Map] Placing %zu ladders and %zu ropes...\n", ladders_to_place.size(), ropes_to_place.size());
    
    int ladders_placed = 0;
    int ropes_placed = 0;
    
    for (const auto& ladder : ladders_to_place) {
        for (int y_coord = ladder.y1; y_coord <= ladder.y2; ++y_coord) {
            if (map.isInsideBounds(ladder.x, y_coord)) {
                if (map.tiles[ladder.x][y_coord] == EMPTY_TILE_VALUE) {
                    map.tiles[ladder.x][y_coord] = LADDER_TILE_VALUE;
                    map.isOriginalSolid[ladder.x][y_coord] = false;
                    ladders_placed++;
                }
            }
        }
    }
    
    for (const auto& rope : ropes_to_place) {
        for (int y_coord = rope.y1; y_coord <= rope.y2; ++y_coord) {
            if (map.isInsideBounds(rope.x, y_coord)) {
                if (map.tiles[rope.x][y_coord] == EMPTY_TILE_VALUE) {
                    map.tiles[rope.x][y_coord] = ROPE_TILE_VALUE;
                    map.isOriginalSolid[rope.x][y_coord] = false;
                    ropes_placed++;
                }
            }
        }
    }
    
    printf("[Map] Placed %d ladder tiles and %d rope tiles\n", ladders_placed, ropes_placed);
}
