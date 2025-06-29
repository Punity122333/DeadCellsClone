#include "core/FastRNG.hpp"
#include <chrono>

FastRNG::FastRNG(uint64_t seed) {
    this->seed(seed == 0 ? std::chrono::high_resolution_clock::now().time_since_epoch().count() : seed);
}

void FastRNG::seed(uint64_t seed) {
    uint64_t z = seed;
    state[0] = splitmix64(z);
    state[1] = splitmix64(z);
    state[2] = splitmix64(z);
    state[3] = splitmix64(z);
}

uint64_t FastRNG::next() {
    const uint64_t result = rotl(state[0] + state[3], 23) + state[0];
    const uint64_t t = state[1] << 17;
    
    state[2] ^= state[0];
    state[3] ^= state[1];
    state[1] ^= state[2];
    state[0] ^= state[3];
    
    state[2] ^= t;
    state[3] = rotl(state[3], 45);
    
    return result;
}

uint32_t FastRNG::nextUInt(uint32_t max) {
    if (max == 0) return 0;
    
    // Use rejection sampling to avoid bias
    uint64_t range = static_cast<uint64_t>(max);
    uint64_t limit = (UINT64_MAX / range) * range;
    
    uint64_t result;
    do {
        result = next();
    } while (result >= limit);
    
    return static_cast<uint32_t>(result % range);
}

int FastRNG::rollPercent() {
    return static_cast<int>(nextUInt(100));
}

float FastRNG::nextFloat() {
    return static_cast<float>(next() >> 11) * 0x1.0p-53f;
}

uint64_t FastRNG::rotl(const uint64_t x, int k) {
    return (x << k) | (x >> (64 - k));
}

uint64_t FastRNG::splitmix64(uint64_t& z) {
    z += 0x9e3779b97f4a7c15;
    uint64_t result = z;
    result = (result ^ (result >> 30)) * 0xbf58476d1ce4e5b9;
    result = (result ^ (result >> 27)) * 0x94d049bb133111eb;
    return result ^ (result >> 31);
}
