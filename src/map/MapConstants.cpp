#include "map/MapConstants.hpp"
#include <random>

namespace MapConstants {
    int rollPercent(std::mt19937& gen) {
        static std::uniform_int_distribution<> dist(0, 99);
        return dist(gen);
    }
}
