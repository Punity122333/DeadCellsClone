#ifndef CORE_CORE_HPP
#define CORE_CORE_HPP

#include "core/GameLoop.hpp"
#include "core/InputManager.hpp"
#include "core/StateManager.hpp"
#include "core/ResourceManager.hpp"
#include "core/EventManager.hpp"
#include <string>
#include <vector>

namespace Core {

void Initialize(bool enableDebugMode = false);
void Shutdown();
InputManager& GetInputManager();
ResourceManager& GetResourceManager();
EventManager& GetEventManager();
bool IsInitialized();

struct CoreConfig {
    bool enableHotReload = false;
    bool enableEventLogging = false;
    bool enableInputBuffering = true;
    float inputBufferTime = 0.1f;
    float gamepadDeadzone = 0.1f;
    float mouseSensitivity = 1.0f;
    std::vector<std::string> resourcePaths = {"../resources/", "./resources/", "./"};
};

void ApplyConfig(const CoreConfig& config);
const CoreConfig& GetConfig();

} 
#endif 
