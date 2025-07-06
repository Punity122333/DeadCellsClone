#include "Game.hpp"
#include "raylib.h"
#include "FishEyeGradient.hpp"
#include "core/Core.hpp"
#include <filesystem>

namespace GamePaths {
    constexpr const char* Icon = "../resources/icon/CellularAutomata.png";
    constexpr const char* Tile = "../resources/tiles/tile%03d.png";
    constexpr const char* BloomShader = "../shader/bloom.fs";
    constexpr const char* ChromaticAberrationShader = "../shader/chromatic_aberration.fs";
}

void Game::initializeResources() {
    auto& resourceManager = Core::GetResourceManager();
    
    Color innerCyan = { 0, 80, 80, 255 };
    Color outerPrussianBlue = { 0, 30, 50, 255 };
    fisheyeBackground = CreateFisheyeGradient(screenWidth, screenHeight, innerCyan, outerPrussianBlue);

    std::vector<std::string> tilePaths;
    int numTiles = 0;
    for (int i = 0; ; ++i) {
        char path_buffer[64];
        snprintf(path_buffer, sizeof(path_buffer), GamePaths::Tile, i);
        if (!std::filesystem::exists(path_buffer)) break;
        tilePaths.push_back(path_buffer);
        numTiles++;
    }
    
    resourceManager.preloadBatch(tilePaths);
    
    tileTextureHandles.clear();
    tileTextures.clear();
    for (const auto& path : tilePaths) {
        auto handle = resourceManager.getTexture(path);
        if (handle.isValid()) {
            tileTextureHandles.push_back(handle);
            tileTextures.push_back(*handle.get());
        }
    }

    sceneTexture = LoadRenderTexture(screenWidth, screenHeight);
    
    bloomShaderHandle = resourceManager.loadShader("", GamePaths::BloomShader);
    chromaticAberrationShaderHandle = resourceManager.loadShader("", GamePaths::ChromaticAberrationShader);
    
    if (bloomShaderHandle.isValid()) {
        bloomShader = *bloomShaderHandle.get();
        activeShader = bloomShaderHandle.get();
    }
    if (chromaticAberrationShaderHandle.isValid()) {
        chromaticAberrationShader = *chromaticAberrationShaderHandle.get();
    }
}

void Game::showResourceStats() {
    auto& resourceManager = Core::GetResourceManager();
    auto stats = resourceManager.getMemoryStats();
}
