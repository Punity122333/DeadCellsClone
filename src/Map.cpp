#include "Map.hpp"
#include <cstdio> // For snprintf
#include <vector>
#include <random> // For std::random_device, std::mt19937


// Constants from the anonymous namespace in Map.hpp are accessible here
// as they are defined in the header that is included.

Map::Map(int w, int h) :
    width(w),
    height(h),
    tiles(w, std::vector<int>(h, 0)),
    transitionTimers(w, std::vector<float>(h, 0.0f)),
    isOriginalSolid(w, std::vector<bool>(h, false)),
    isConwayProtected(w, std::vector<bool>(h, false))
{
    std::random_device rd;
    std::mt19937 gen(rd());

    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            tiles[x][y] = 0;
            isOriginalSolid[x][y] = false;
            isConwayProtected[x][y] = false;
        }
    }

    int numTiles = 0;
    {
        for (int i = 0; ; ++i) {
            char path[64];
            snprintf(path, sizeof(path), "../resources/tiles/tile%03d.png", i);
            FILE* f = fopen(path, "r");
            if (!f) break;
            fclose(f);
            numTiles++;
        }
    }
    for (int i = 0; i < numTiles; ++i) {
        char path[64];
        snprintf(path, sizeof(path), "../resources/tiles/tile%03d.png", i);
        tileTextures.push_back(LoadTexture(path));
    }

    for (int x = 0; x < width; x++) {
        tiles[x][height - 1] = 1; isOriginalSolid[x][height - 1] = true;
        tiles[x][0] = 1;         isOriginalSolid[x][0] = true;
    }
    for (int y = 0; y < height; y++) {
        tiles[0][y] = 1;         isOriginalSolid[0][y] = true;
        tiles[width - 1][y] = 1; isOriginalSolid[width - 1][y] = true;
    }

    generateRoomsAndConnections(gen);

    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            if (isOriginalSolid[x][y]) {
                for (int dx = -2; dx <= 2; ++dx) {
                    for (int dy = -2; dy <= 2; ++dy) {
                        int nx = x + dx;
                        int ny = y + dy;
                        if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                            isConwayProtected[nx][ny] = true;
                        }
                    }
                }
            }
        }
    }
}

void Map::placeBorders() {
}

int Map::getHeight() const {
    return height;
}

Map::~Map() {
    for (const auto& texture : tileTextures) {
        UnloadTexture(texture);
    }
}
