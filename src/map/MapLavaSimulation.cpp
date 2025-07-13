#include "map/Map.hpp"
#include <algorithm>
#include <cmath>
#include <vector>

using namespace MapConstants;

void Map::updateLavaFlow(float dt) {
    
    static int updateCounter = 0;
    static constexpr int UPDATE_INTERVAL = 15;
    
    updateCounter++;
    if (updateCounter < UPDATE_INTERVAL) {
        return;
    }
    updateCounter = 0;
    
    static constexpr float GRAVITY_ACCELERATION = 0.15f;
    static constexpr float FLOW_DAMPING = 0.98f;
    static constexpr float MIN_FLOW_THRESHOLD = 0.0001f;
    static constexpr float PRESSURE_EQUALIZATION = 0.15f;
    static constexpr float HORIZONTAL_FLOW_RATE = 0.8f;
    static constexpr float HORIZONTAL_SPREAD_RATE = 0.6f;
    static constexpr float MAX_COMPRESSION = 1.02f;
    static constexpr float EVAPORATION_RATE = 0.9999f;
    
    auto newMass = std::vector<std::vector<float>>(width, std::vector<float>(height, 0.0f));
    auto newFlow = std::vector<std::vector<float>>(width, std::vector<float>(height, 0.0f));
    
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            if (isLavaTile(x, y)) {
                newMass[x][y] = lavaGrid[x][y].mass;
                newFlow[x][y] = lavaGrid[x][y].flow;
            }
        }
    }
    
    for (int x = 0; x < width; ++x) {
        for (int y = height - 2; y >= 0; --y) {
            if (!isLavaTile(x, y) || newMass[x][y] <= LAVA_MIN_MASS) continue;
            
            int belowY = y + 1;
            if (belowY >= height) continue;
            
            bool canFlowDown = false;
            float belowMass = 0.0f;
            
            int belowTile = tiles[x][belowY];
            if (belowTile == EMPTY_TILE_VALUE || belowTile == PROTECTED_EMPTY_TILE_VALUE || 
                belowTile == LADDER_TILE_VALUE || belowTile == ROPE_TILE_VALUE) {
                canFlowDown = true;
                belowMass = 0.0f;
            } else if (isLavaTile(x, belowY)) {
                canFlowDown = true;
                belowMass = newMass[x][belowY];
            } else if (isSolidTile(x, belowY)) {
                canFlowDown = false;
            }
            
            if (canFlowDown) {
                if (belowMass < LAVA_MAX_MASS) {
                    float availableSpace = LAVA_MAX_MASS - belowMass;
                    float flowDown = std::min(newMass[x][y] * 0.25f, availableSpace);
                    
                    if (flowDown > 0.0f) {
                        newMass[x][y] -= flowDown;
                        newMass[x][belowY] += flowDown;
                        newFlow[x][y] = std::max(newFlow[x][y], flowDown);
                        
                        int belowTile = tiles[x][belowY];
                        if ((belowTile == EMPTY_TILE_VALUE || belowTile == PROTECTED_EMPTY_TILE_VALUE || 
                             belowTile == LADDER_TILE_VALUE || belowTile == ROPE_TILE_VALUE) && 
                            newMass[x][belowY] > LAVA_MIN_MASS) {
                            tiles[x][belowY] = LAVA_TILE_VALUE;
                            isOriginalSolid[x][belowY] = false;
                        }
                    }
                }
            }
        }
    }
    
    for (int pass = 0; pass < 6; ++pass) {
        for (int x = 1; x < width - 1; ++x) {
            for (int y = 0; y < height; ++y) {
                if (!isLavaTile(x, y) || newMass[x][y] <= LAVA_MIN_MASS) continue;
                
                bool hasSupport = false;
                if (y + 1 < height) {
                    hasSupport = isSolidTile(x, y + 1) || 
                                (isLavaTile(x, y + 1) && newMass[x][y + 1] > 0.01f);
                }
                if (y >= height - 1) hasSupport = true;
                
                for (int dx = -1; dx <= 1; dx += 2) {
                    int neighborX = x + dx;
                    if (neighborX < 0 || neighborX >= width) continue;
                    
                    bool canFlowSideways = false;
                    float neighborMass = 0.0f;
                    
                    int neighborTile = tiles[neighborX][y];
                    if (neighborTile == EMPTY_TILE_VALUE || neighborTile == PROTECTED_EMPTY_TILE_VALUE || 
                        neighborTile == LADDER_TILE_VALUE || neighborTile == ROPE_TILE_VALUE) {
                        bool neighborHasSupport = false;
                        if (y + 1 < height) {
                            neighborHasSupport = isSolidTile(neighborX, y + 1) || 
                                               (isLavaTile(neighborX, y + 1) && newMass[neighborX][y + 1] > 0.01f);
                        }
                        if (y >= height - 1) neighborHasSupport = true;
                        
                        if (neighborHasSupport || newMass[x][y] > 0.05f) {
                            canFlowSideways = true;
                            neighborMass = 0.0f;
                        }
                    } else if (isLavaTile(neighborX, y)) {
                        canFlowSideways = true;
                        neighborMass = newMass[neighborX][y];
                    }
                    
                    if (canFlowSideways) {
                        float heightDiff = newMass[x][y] - neighborMass;
                        
                        if (heightDiff > MIN_FLOW_THRESHOLD) {
                            float flowAmount = heightDiff * HORIZONTAL_FLOW_RATE * dt;
                            
                            if (hasSupport) {
                                flowAmount = std::min(flowAmount, newMass[x][y] * 0.7f);
                            } else {
                                flowAmount = std::min(flowAmount, newMass[x][y] * 0.5f);
                            }
                            
                            flowAmount = std::min(flowAmount, LAVA_MAX_MASS - neighborMass);
                            
                            if (flowAmount > MIN_FLOW_THRESHOLD) {
                                newMass[x][y] -= flowAmount;
                                newMass[neighborX][y] += flowAmount;
                                newFlow[x][y] = std::max(newFlow[x][y], flowAmount);
                                newFlow[neighborX][y] = std::max(newFlow[neighborX][y], flowAmount);
                                
                                int neighborTile = tiles[neighborX][y];
                                if ((neighborTile == EMPTY_TILE_VALUE || neighborTile == PROTECTED_EMPTY_TILE_VALUE || 
                                     neighborTile == LADDER_TILE_VALUE || neighborTile == ROPE_TILE_VALUE) && 
                                    flowAmount > LAVA_MIN_MASS) {
                                    tiles[neighborX][y] = LAVA_TILE_VALUE;
                                    isOriginalSolid[neighborX][y] = false;
                                }
                            }
                        }
                    }
                }
            }
        }
        
        for (int x = 0; x < width; ++x) {
            for (int y = 0; y < height; ++y) {
                if (isLavaTile(x, y) && newMass[x][y] > 0.03f) {
                    
                    for (int dx = -1; dx <= 1; dx += 2) {
                        int neighborX = x + dx;
                        if (neighborX < 0 || neighborX >= width) continue;
                        
                        int neighborTile = tiles[neighborX][y];
                        if (neighborTile == EMPTY_TILE_VALUE || neighborTile == PROTECTED_EMPTY_TILE_VALUE || 
                            neighborTile == LADDER_TILE_VALUE || neighborTile == ROPE_TILE_VALUE) {
                            
                            bool neighborCanReceive = true;
                            if (y + 1 < height) {
                                neighborCanReceive = isSolidTile(neighborX, y + 1) || 
                                                   (isLavaTile(neighborX, y + 1) && newMass[neighborX][y + 1] > 0.01f) ||
                                                   newMass[x][y] > 0.08f;
                            }
                            
                            if (neighborCanReceive && newMass[x][y] > 0.05f) {
                                float spreadAmount = newMass[x][y] * HORIZONTAL_SPREAD_RATE * dt;
                                spreadAmount = std::min(spreadAmount, newMass[x][y] * 0.6f);
                                
                                newMass[x][y] -= spreadAmount;
                                newMass[neighborX][y] += spreadAmount;
                                newFlow[x][y] = std::max(newFlow[x][y], spreadAmount);
                                newFlow[neighborX][y] = std::max(newFlow[neighborX][y], spreadAmount);
                                
                                tiles[neighborX][y] = LAVA_TILE_VALUE;
                                isOriginalSolid[neighborX][y] = false;
                            }
                        }
                    }
                }
            }
        }
    }
    
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            if (isLavaTile(x, y)) {
                newMass[x][y] = std::min(newMass[x][y], LAVA_MAX_MASS * MAX_COMPRESSION);
                newFlow[x][y] *= FLOW_DAMPING;
                
                if (newMass[x][y] < LAVA_MIN_MASS * 0.5f) {
                    newMass[x][y] *= EVAPORATION_RATE;
                }
                
                if (newMass[x][y] < LAVA_MIN_MASS * 0.15f) {
                    tiles[x][y] = EMPTY_TILE_VALUE;
                    newMass[x][y] = 0.0f;
                    newFlow[x][y] = 0.0f;
                }
            }
        }
    }
    
    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            if (isLavaTile(x, y)) {
                lavaGrid[x][y].mass = newMass[x][y];
                lavaGrid[x][y].flow = newFlow[x][y];
                lavaGrid[x][y].settled = (newFlow[x][y] < MIN_FLOW_THRESHOLD);
            }
        }
    }
}