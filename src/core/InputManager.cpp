#include "core/InputManager.hpp"
#include <raymath.h>
#include <algorithm>

namespace Core {

InputManager::InputManager() 
    : m_mousePosition{0, 0}
    , m_lastMousePosition{0, 0}
    , m_mouseDelta{0, 0}
    , m_gamepadDeadzone(0.1f)
    , m_mouseSensitivity(1.0f)
    , m_inputBufferingEnabled(true)
    , m_inputBufferTime(0.1f)
    , m_nextCallbackId(1)
{
    setupDefaultBindings();
}

void InputManager::update(float deltaTime) {

    m_lastMousePosition = m_mousePosition;
    m_mousePosition = GetMousePosition();
    m_mouseDelta = Vector2Subtract(m_mousePosition, m_lastMousePosition);
    m_mouseDelta = Vector2Scale(m_mouseDelta, m_mouseSensitivity);

    updateActionStates(deltaTime);
    triggerCallbacks();
}

void InputManager::bindKey(InputAction action, KeyboardKey key) {
    auto& binding = m_bindings[action];
    auto it = std::find(binding.keys.begin(), binding.keys.end(), key);
    if (it == binding.keys.end()) {
        binding.keys.push_back(key);
    }
}

void InputManager::bindGamepadButton(InputAction action, GamepadButton button, int gamepadId) {
    auto& binding = m_bindings[action];
    auto pair = std::make_pair(button, gamepadId);
    auto it = std::find(binding.gamepadButtons.begin(), binding.gamepadButtons.end(), pair);
    if (it == binding.gamepadButtons.end()) {
        binding.gamepadButtons.push_back(pair);
    }
}

void InputManager::bindGamepadAxis(InputAction action, GamepadAxis axis, int gamepadId) {
    auto& binding = m_bindings[action];
    auto pair = std::make_pair(axis, gamepadId);
    auto it = std::find(binding.gamepadAxes.begin(), binding.gamepadAxes.end(), pair);
    if (it == binding.gamepadAxes.end()) {
        binding.gamepadAxes.push_back(pair);
    }
}

void InputManager::bindMouseButton(InputAction action, MouseButton button) {
    auto& binding = m_bindings[action];
    auto it = std::find(binding.mouseButtons.begin(), binding.mouseButtons.end(), button);
    if (it == binding.mouseButtons.end()) {
        binding.mouseButtons.push_back(button);
    }
}

int InputManager::registerCallback(InputCallback callback) {
    int id = m_nextCallbackId++;
    m_callbacks[id] = std::move(callback);
    return id;
}

void InputManager::removeCallback(int callbackId) {
    m_callbacks.erase(callbackId);
}

bool InputManager::isActionPressed(InputAction action) const {
    auto it = m_actionStates.find(action);
    return it != m_actionStates.end() && it->second.pressed;
}

bool InputManager::isActionReleased(InputAction action) const {
    auto it = m_actionStates.find(action);
    return it != m_actionStates.end() && it->second.released;
}

bool InputManager::isActionHeld(InputAction action) const {
    auto it = m_actionStates.find(action);
    return it != m_actionStates.end() && it->second.held;
}

float InputManager::getActionValue(InputAction action) const {
    auto it = m_actionStates.find(action);
    return it != m_actionStates.end() ? it->second.value : 0.0f;
}

Vector2 InputManager::getMousePosition() const {
    return m_mousePosition;
}

Vector2 InputManager::getMouseDelta() const {
    return m_mouseDelta;
}

void InputManager::setGamepadDeadzone(float deadzone) {
    m_gamepadDeadzone = Clamp(deadzone, 0.0f, 1.0f);
}

void InputManager::setMouseSensitivity(float sensitivity) {
    m_mouseSensitivity = fmaxf(sensitivity, 0.1f);
}

void InputManager::setInputBuffering(bool enabled, float bufferTime) {
    m_inputBufferingEnabled = enabled;
    m_inputBufferTime = fmaxf(bufferTime, 0.0f);
}

bool InputManager::loadBindings(const char* filename) {

    return true; 
}

bool InputManager::saveBindings(const char* filename) {

    return true; 
}

void InputManager::resetToDefault() {
    m_bindings.clear();
    setupDefaultBindings();
}

void InputManager::updateActionStates(float deltaTime) {
    for (auto& [action, binding] : m_bindings) {
        ActionState& state = m_actionStates[action];
        

        bool wasPressed = state.pressed;
        bool wasHeld = state.held;
        state.pressed = false;
        state.released = false;
        
        bool currentlyActive = false;
        float maxValue = 0.0f;

        for (KeyboardKey key : binding.keys) {
            if (isKeyPressed(key)) {
                state.pressed = true;
                currentlyActive = true;
                maxValue = 1.0f;
            } else if (isKeyReleased(key)) {
                state.released = true;
            } else if (isKeyHeld(key)) {
                currentlyActive = true;
                maxValue = 1.0f;
            }
        }

        for (MouseButton button : binding.mouseButtons) {
            if (IsMouseButtonPressed(button)) {
                state.pressed = true;
                currentlyActive = true;
                maxValue = 1.0f;
            } else if (IsMouseButtonReleased(button)) {
                state.released = true;
            } else if (IsMouseButtonDown(button)) {
                currentlyActive = true;
                maxValue = 1.0f;
            }
        }


        for (auto& [button, gamepadId] : binding.gamepadButtons) {
            if (IsGamepadAvailable(gamepadId)) {
                if (IsGamepadButtonPressed(gamepadId, button)) {
                    state.pressed = true;
                    currentlyActive = true;
                    maxValue = 1.0f;
                } else if (IsGamepadButtonReleased(gamepadId, button)) {
                    state.released = true;
                } else if (IsGamepadButtonDown(gamepadId, button)) {
                    currentlyActive = true;
                    maxValue = 1.0f;
                }
            }
        }

        for (auto& [axis, gamepadId] : binding.gamepadAxes) {
            if (IsGamepadAvailable(gamepadId)) {
                float axisValue = GetGamepadAxisMovement(gamepadId, axis);
                axisValue = applyDeadzone(axisValue);
                
                if (fabsf(axisValue) > 0.0f) {
                    currentlyActive = true;
                    maxValue = fmaxf(maxValue, fabsf(axisValue));

                    if (!wasHeld && fabsf(axisValue) > 0.5f) {
                        state.pressed = true;
                    }
                }
            }
        }

        state.held = currentlyActive;
        state.value = maxValue;

        if (m_inputBufferingEnabled && state.pressed) {
            state.bufferTime = m_inputBufferTime;
        } else if (state.bufferTime > 0.0f) {
            state.bufferTime -= deltaTime;
            if (state.bufferTime <= 0.0f) {
                state.bufferTime = 0.0f;
            } else {
                state.pressed = true; 
            }
        }

        if (wasHeld && !currentlyActive) {
            state.released = true;
        }
    }
}

void InputManager::triggerCallbacks() {
    for (auto& [action, state] : m_actionStates) {
        if (state.pressed || state.released || state.held) {
            InputEvent event{
                .action = action,
                .pressed = state.pressed,
                .released = state.released,
                .held = state.held,
                .value = state.value,
                .mousePosition = m_mousePosition,
                .mouseDelta = m_mouseDelta
            };

            for (auto& [id, callback] : m_callbacks) {
                callback(event);
            }
        }
    }
}

void InputManager::setupDefaultBindings() {
    // Movement
    bindKey(InputAction::MOVE_LEFT, KEY_A);
    bindKey(InputAction::MOVE_LEFT, KEY_LEFT);
    bindKey(InputAction::MOVE_RIGHT, KEY_D);
    bindKey(InputAction::MOVE_RIGHT, KEY_RIGHT);
    bindKey(InputAction::MOVE_UP, KEY_W);
    bindKey(InputAction::MOVE_UP, KEY_UP);
    bindKey(InputAction::MOVE_DOWN, KEY_S);
    bindKey(InputAction::MOVE_DOWN, KEY_DOWN);
    bindKey(InputAction::JUMP, KEY_SPACE);

    // Combat
    bindMouseButton(InputAction::ATTACK, MOUSE_BUTTON_LEFT);
    bindKey(InputAction::ATTACK, KEY_J);
    bindKey(InputAction::ATTACK, KEY_X);
    bindMouseButton(InputAction::BLOCK, MOUSE_BUTTON_RIGHT);
    bindKey(InputAction::BLOCK, KEY_C);
    bindKey(InputAction::DODGE, KEY_LEFT_SHIFT);

    // Weapons
    bindKey(InputAction::SWITCH_WEAPON_1, KEY_ONE);
    bindKey(InputAction::SWITCH_WEAPON_2, KEY_TWO);

    // System
    bindKey(InputAction::INTERACT, KEY_E);
    bindKey(InputAction::PAUSE, KEY_ESCAPE);
    bindKey(InputAction::DEBUG_TOGGLE, KEY_TAB);

    // Menu navigation
    bindKey(InputAction::MENU_UP, KEY_UP);
    bindKey(InputAction::MENU_DOWN, KEY_DOWN);
    bindKey(InputAction::MENU_LEFT, KEY_LEFT);
    bindKey(InputAction::MENU_RIGHT, KEY_RIGHT);
    bindKey(InputAction::MENU_SELECT, KEY_ENTER);
    bindKey(InputAction::MENU_BACK, KEY_ESCAPE);

    // Debug/Development
    bindKey(InputAction::QUICK_SAVE, KEY_F5);
    bindKey(InputAction::QUICK_LOAD, KEY_F9);

    // Gamepad bindings
    bindGamepadButton(InputAction::JUMP, GAMEPAD_BUTTON_RIGHT_FACE_DOWN); 
    bindGamepadButton(InputAction::ATTACK, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT); 
    bindGamepadButton(InputAction::BLOCK, GAMEPAD_BUTTON_LEFT_TRIGGER_1); 
    bindGamepadButton(InputAction::DODGE, GAMEPAD_BUTTON_RIGHT_TRIGGER_1); 
    bindGamepadButton(InputAction::PAUSE, GAMEPAD_BUTTON_MIDDLE_RIGHT); 

    // Gamepad axes for movement
    bindGamepadAxis(InputAction::MOVE_LEFT, GAMEPAD_AXIS_LEFT_X);
    bindGamepadAxis(InputAction::MOVE_RIGHT, GAMEPAD_AXIS_LEFT_X);
}

float InputManager::applyDeadzone(float value) const {
    if (fabsf(value) < m_gamepadDeadzone) {
        return 0.0f;
    }

    float sign = (value < 0.0f) ? -1.0f : 1.0f;
    float scaledValue = (fabsf(value) - m_gamepadDeadzone) / (1.0f - m_gamepadDeadzone);
    return sign * scaledValue;
}

bool InputManager::isKeyPressed(KeyboardKey key) const {
    return IsKeyPressed(key);
}

bool InputManager::isKeyReleased(KeyboardKey key) const {
    return IsKeyReleased(key);
}

bool InputManager::isKeyHeld(KeyboardKey key) const {
    return IsKeyDown(key);
}

} 
