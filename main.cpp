#include "Game.hpp"
#include "core/Core.hpp"

int main() {
    Core::Initialize(true);
    
    Core::CoreConfig config;
    config.enableHotReload = true;
    config.enableEventLogging = false;
    config.resourcePaths = {"../resources/", "./resources/", "../"};
    Core::ApplyConfig(config);
    
    {
        Game game;
        game.run();
    } // Game destructor is called here
    
    Core::Shutdown();
    return 0;
}