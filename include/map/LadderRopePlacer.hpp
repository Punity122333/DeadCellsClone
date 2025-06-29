#ifndef LADDER_ROPE_PLACER_HPP
#define LADDER_ROPE_PLACER_HPP

#include "map/Map.hpp"
#include <vector>

class LadderRopePlacer {
public:
    static void placeLaddersAndRopes(Map& map, const std::vector<Ladder>& ladders_to_place,
                                   const std::vector<Rope>& ropes_to_place);
};

#endif
