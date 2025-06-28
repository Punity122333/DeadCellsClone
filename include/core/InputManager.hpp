#ifndef CORE_INPUTMANAGER_HPP
#define CORE_INPUTMANAGER_HPP

#include <raylib.h>
#include <unordered_map>
#include <vector>
#include <functional>

namespace Core {

enum class InputAction {
    MOVE_LEFT,
    MOVE_RIGHT,
    MOVE_UP,
    MOVE_DOWN,
    JUMP,
    ATTACK,
    BLOCK,
    DODGE,
    SWITCH_WEAPON_1,
    SWITCH_WEAPON_2,
    INTERACT,
    PAUSE,
    DEBUG_TOGGLE,
    MENU_UP,
    MENU_DOWN,
    MENU_LEFT,
    MENU_RIGHT,
    MENU_SELECT,
    MENU_BACK,
    QUICK_SAVE,
    QUICK_LOAD
};

struct InputEvent {
    InputAction action;
    bool pressed;
    bool released;
    bool held;
    float value;
    Vector2 mousePosition;
    Vector2 mouseDelta;
};

class InputManager {
public:
    using InputCallback = std::function<void(const InputEvent&)>;
    
    InputManager();
    ~InputManager() = default;

    void update(float deltaTime);
    void bindKey(InputAction action, KeyboardKey key);
    void bindGamepadButton(InputAction action, GamepadButton button, int gamepadId = 0);
    void bindGamepadAxis(InputAction action, GamepadAxis axis, int gamepadId = 0);
    void bindMouseButton(InputAction action, MouseButton button);
    int registerCallback(InputCallback callback);
    void removeCallback(int callbackId);
    bool isActionPressed(InputAction action) const;
    bool isActionReleased(InputAction action) const;
    bool isActionHeld(InputAction action) const;
    float getActionValue(InputAction action) const;
    Vector2 getMousePosition() const;
    Vector2 getMouseDelta() const;
    void setGamepadDeadzone(float deadzone);
    void setMouseSensitivity(float sensitivity);
    void setInputBuffering(bool enabled, float bufferTime = 0.1f);
    bool loadBindings(const char* filename);
    bool saveBindings(const char* filename);
    void resetToDefault();

private:
    struct ActionState {
        bool pressed = false;
        bool released = false;
        bool held = false;
        float value = 0.0f;
        float bufferTime = 0.0f;
    };

    struct KeyBinding {
        std::vector<KeyboardKey> keys;
        std::vector<MouseButton> mouseButtons;
        std::vector<std::pair<GamepadButton, int>> gamepadButtons;
        std::vector<std::pair<GamepadAxis, int>> gamepadAxes;
    };

    std::unordered_map<InputAction, ActionState> m_actionStates;
    std::unordered_map<InputAction, KeyBinding> m_bindings;
    std::unordered_map<int, InputCallback> m_callbacks;
    
    Vector2 m_mousePosition;
    Vector2 m_lastMousePosition;
    Vector2 m_mouseDelta;
    
    float m_gamepadDeadzone;
    float m_mouseSensitivity;
    bool m_inputBufferingEnabled;
    float m_inputBufferTime;
    int m_nextCallbackId;

    void updateActionStates(float deltaTime);
    void triggerCallbacks();
    void setupDefaultBindings();
    float applyDeadzone(float value) const;
    bool isKeyPressed(KeyboardKey key) const;
    bool isKeyReleased(KeyboardKey key) const;
    bool isKeyHeld(KeyboardKey key) const;
};

} 

#endif
