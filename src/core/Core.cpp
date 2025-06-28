#include "core/Core.hpp"
#include <iostream>

namespace Core {

static bool g_initialized = false;
static CoreConfig g_config;
static std::unique_ptr<InputManager> g_inputManager;
static std::unique_ptr<ResourceManager> g_resourceManager;
static std::unique_ptr<EventManager> g_eventManager;

void Initialize(bool enableDebugMode) {
    if (g_initialized) {
        std::cerr << "[Core] Warning: Core systems already initialized" << std::endl;
        return;
    }

    g_inputManager = std::make_unique<InputManager>();
    g_resourceManager = std::make_unique<ResourceManager>();
    g_eventManager = std::make_unique<EventManager>();
    
    auto& eventManager = GetEventManager();
    eventManager.setLoggingEnabled(enableDebugMode);
    
    if (enableDebugMode) {
        eventManager.setErrorCallback([](const std::string& error) {
            std::cerr << "[Core::EventManager] " << error << std::endl;
        });
    }

    auto& resourceManager = GetResourceManager();
    resourceManager.setHotReloadEnabled(enableDebugMode);

    g_initialized = true;
    
    if (enableDebugMode) {
        std::cout << "[Core] Core systems initialized with debug mode enabled" << std::endl;
    }
}

void Shutdown() {
    if (!g_initialized) {
        return;
    }

    g_inputManager.reset();
    
    if (g_eventManager) {
        g_eventManager->clearQueue();
        g_eventManager->clearAllSubscribers();
    }
    
    if (g_resourceManager) {
        g_resourceManager->unloadAll();
    }

    g_eventManager.reset();
    g_resourceManager.reset();

    g_initialized = false;
    
    std::cout << "[Core] Core systems shutdown complete" << std::endl;
}

InputManager& GetInputManager() {
    if (!g_inputManager) {
        throw std::runtime_error("Core systems not initialized. Call Core::Initialize() first.");
    }
    return *g_inputManager;
}

bool IsInitialized() {
    return g_initialized;
}

void ApplyConfig(const CoreConfig& config) {
    g_config = config;
    
    if (!g_initialized) {
        return;
    }

    auto& inputManager = GetInputManager();
    inputManager.setInputBuffering(config.enableInputBuffering, config.inputBufferTime);
    inputManager.setGamepadDeadzone(config.gamepadDeadzone);
    inputManager.setMouseSensitivity(config.mouseSensitivity);

    auto& resourceManager = GetResourceManager();
    resourceManager.setHotReloadEnabled(config.enableHotReload);
    resourceManager.setSearchPaths(config.resourcePaths);

    auto& eventManager = GetEventManager();
    eventManager.setLoggingEnabled(config.enableEventLogging);
    
    std::cout << "[Core] Configuration applied successfully" << std::endl;
}

const CoreConfig& GetConfig() {
    return g_config;
}

ResourceManager& GetResourceManager() {
    if (!g_resourceManager) {
        throw std::runtime_error("Core systems not initialized. Call Core::Initialize() first.");
    }
    return *g_resourceManager;
}

EventManager& GetEventManager() {
    if (!g_eventManager) {
        throw std::runtime_error("Core systems not initialized. Call Core::Initialize() first.");
    }
    return *g_eventManager;
}

} // namespace Core
