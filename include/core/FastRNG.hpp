#ifndef FAST_RNG_HPP
#define FAST_RNG_HPP

#include <cstdint>

class FastRNG {
public:
    explicit FastRNG(uint64_t seed = 0);

    uint64_t next();

    uint32_t nextUInt(uint32_t max);
    

    int rollPercent();

    float nextFloat();

    void seed(uint64_t seed);
    
private:
    uint64_t state[4];
    
    static uint64_t rotl(const uint64_t x, int k);
    static uint64_t splitmix64(uint64_t& z);
};

#endif
