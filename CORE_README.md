# Core Module Documentation

The core module provides a comprehensive set of production-ready systems for game development. This module transforms the basic game into a professional-grade engine with robust architecture.

## Systems Overview

### 1. GameLoop (`core/GameLoop.hpp`)
**Production-ready game loop with fixed timestep and frame interpolation**

- **Fixed timestep updates**: Ensures deterministic physics and game logic
- **Frame interpolation**: Provides smooth rendering independent of update rate
- **Performance monitoring**: Tracks FPS, UPS, and frame times
- **VSync control**: Optional VSync with manual frame rate limiting

```cpp
Core::GameLoop gameLoop(60, 60); // 60 FPS, 60 UPS

gameLoop.setUpdateCallback([&](float deltaTime) {
    // Fixed timestep game updates
    inputManager.update(deltaTime);
    gameWorld.update(deltaTime);
});

gameLoop.setRenderCallback([&](float interpolation) {
    // Smooth rendering with interpolation
    renderer.render(interpolation);
});

gameLoop.run();
```

### 2. InputManager (`core/InputManager.hpp`)
**Flexible input system with action mapping and device support**

- **Action-based input**: Map multiple keys/buttons to logical actions
- **Multi-device support**: Keyboard, mouse, and gamepad with deadzone handling
- **Input buffering**: Responsive controls with configurable buffer time
- **Customizable bindings**: Runtime key remapping and configuration files

```cpp
auto& input = Core::GetInputManager();

// Set up input callbacks
input.registerCallback([](const Core::InputEvent& event) {
    if (event.action == Core::InputAction::JUMP && event.pressed) {
        player.jump();
    }
});

// Check input states
if (input.isActionHeld(Core::InputAction::MOVE_LEFT)) {
    player.moveLeft();
}
```

### 3. StateManager (`core/StateManager.hpp`)
**Stack-based state management with transitions**

- **State stack**: Push/pop states for menus, overlays, and game modes
- **Automatic lifecycle**: onEnter, onExit, onPause, onResume callbacks
- **Blocking control**: States can block updates/rendering of states below
- **Factory pattern**: Register state factories for easy instantiation

```cpp
Core::StateManager stateManager;

// Register state factories
stateManager.registerState("menu", []() {
    return std::make_unique<MenuState>();
});

stateManager.registerState("playing", []() {
    return std::make_unique<PlayingState>();
});

// State transitions
stateManager.changeState("menu");          // Clear stack and set state
stateManager.pushState("pause", true);     // Push pause over current state
stateManager.popState();                   // Return to previous state
```

### 4. ResourceManager (`core/ResourceManager.hpp`)
**Automatic resource loading with caching and hot-reloading**

- **Automatic caching**: Load once, reference many times
- **Reference counting**: Automatic cleanup when resources are no longer used
- **Hot-reloading**: Development feature for real-time asset updates
- **Search paths**: Flexible file location system
- **Memory tracking**: Monitor resource usage and performance

```cpp
auto& resources = Core::GetResourceManager();

// Load resources with automatic caching
auto texture = resources.loadTexture("player/sprite.png");
auto sound = resources.loadSound("audio/jump.wav");
auto font = resources.loadFont("fonts/game.ttf", 24);

// Resources are automatically managed
// Use texture.get() to access the underlying Texture2D
DrawTexture(*texture, 0, 0, WHITE);
```

### 5. EventManager (`core/EventManager.hpp`)
**Type-safe event system with threading support**

- **Type safety**: Compile-time event type checking
- **Thread-safe**: Safe for multi-threaded environments
- **Priority queuing**: Process events by importance
- **Event filtering**: Block or modify events before processing
- **Performance monitoring**: Track event throughput and processing times

```cpp
auto& events = Core::GetEventManager();

// Subscribe to events
auto subId = events.subscribe<Core::PlayerHealthChangedEvent>([](const auto& event) {
    std::cout << "Health: " << event.newHealth << "/" << event.maxHealth << std::endl;
});

// Queue events for processing
Core::EnemyDefeatedEvent event;
event.enemyType = "Automaton";
event.scoreValue = 100;
events.queueEvent(event, 1); // Priority 1

// Process all queued events
events.processEvents();
```

## Core Integration

### Initialization
```cpp
#include "core/Core.hpp"

int main() {
    // Initialize Raylib first
    InitWindow(1280, 720, "Game");
    
    // Initialize core systems
    Core::Initialize(true); // Enable debug mode
    
    // Configure core systems
    Core::CoreConfig config;
    config.enableHotReload = true;
    config.enableEventLogging = true;
    config.resourcePaths = {"../assets/", "./assets/"};
    Core::ApplyConfig(config);
    
    // Your game loop here
    
    // Cleanup
    Core::Shutdown();
    CloseWindow();
    return 0;
}
```

### Game State Implementation
```cpp
class PlayingState : public Core::GameState {
public:
    void onEnter() override {
        // Initialize game world
        world.reset();
        player.respawn();
    }
    
    void update(float deltaTime) override {
        auto& input = Core::GetInputManager();
        
        // Handle input
        if (input.isActionPressed(Core::InputAction::PAUSE)) {
            // Push pause state
            stateManager.pushState("pause");
        }
        
        // Update game systems
        world.update(deltaTime);
        player.update(deltaTime);
    }
    
    void render(float interpolation) override {
        world.render(interpolation);
        player.render(interpolation);
    }
    
    const std::string& getStateId() const override {
        static const std::string id = "playing";
        return id;
    }
};
```

## Performance Considerations

### Memory Management
- **Resource caching**: Prevents duplicate loading
- **Reference counting**: Automatic cleanup
- **Memory tracking**: Monitor usage in debug builds

### Threading
- **Thread-safe design**: All systems support multi-threading
- **Lock-free where possible**: Minimize synchronization overhead
- **Performance monitoring**: Track system performance

### Optimization Features
- **Input buffering**: Responsive controls
- **Event batching**: Efficient event processing
- **Resource preloading**: Reduce loading hitches
- **Hot-reloading**: Fast iteration in development

## Production Features

### Error Handling
- **Exception safety**: Robust error handling throughout
- **Logging system**: Comprehensive debug information
- **Graceful degradation**: Continue running when possible

### Configuration
- **Runtime configuration**: Adjust settings without recompilation
- **File-based settings**: Save/load configurations
- **Debug modes**: Additional features for development

### Extensibility
- **Plugin architecture**: Easy to extend with new systems
- **Event-driven design**: Loose coupling between systems
- **Factory patterns**: Runtime object creation

## Migration Guide

To integrate the core systems into your existing game:

1. **Include the core header**: `#include "core/Core.hpp"`
2. **Initialize systems**: Call `Core::Initialize()` after Raylib init
3. **Replace input handling**: Use `Core::GetInputManager()` instead of direct Raylib calls
4. **Convert to states**: Implement game modes as `Core::GameState` classes
5. **Use resource manager**: Replace direct resource loading with `Core::GetResourceManager()`
6. **Add event handling**: Use `Core::GetEventManager()` for decoupled communication

## Example Integration

See the updated `Game.cpp` for a complete example of integrating the core systems into an existing game. The integration provides:

- Professional game loop with fixed timestep
- Robust input handling with customizable controls
- State-based game management
- Efficient resource management
- Event-driven architecture

This core module transforms your game from a prototype into a production-ready application with professional architecture and performance characteristics.
