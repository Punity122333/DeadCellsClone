#include "enemies/Automaton.hpp"

Rectangle Automaton::getHitbox() const {
    return { position.x, position.y, 32, 32 };
}
