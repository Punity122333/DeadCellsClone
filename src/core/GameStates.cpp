#include "core/GameStates.hpp"
#include "core/Core.hpp"
#include "Game.hpp"
#include <raymath.h>

namespace Core {

// State IDs
const std::string PlayingState::STATE_ID = "playing";
const std::string PausedState::STATE_ID = "paused";
const std::string MenuState::STATE_ID = "menu";
const std::string GameOverState::STATE_ID = "game_over";
const std::string LoadingState::STATE_ID = "loading";

// PlayingState implementation
PlayingState::PlayingState(Game* game) : m_game(game) {}

void PlayingState::onEnter() {
    // Initialize game systems
    auto& eventManager = GetEventManager();
    GameStateChangedEvent event;
    event.previousState = "";
    event.newState = STATE_ID;
    eventManager.queueEvent(event);
}

void PlayingState::onExit() {
    // Could trigger events here if needed
}

void PlayingState::onPause() {
    // Game is paused, no special handling needed
}

void PlayingState::onResume() {
    // Game is resumed from pause
}

void PlayingState::update(float deltaTime) {
    // Game logic update would go here
    // This would typically delegate to the main Game class
}

void PlayingState::render(float interpolation) {
    // Game rendering would go here
    // This would typically delegate to the main Game class
}

void PlayingState::handleInput(float deltaTime) {
    auto& inputManager = GetInputManager();
    
    if (inputManager.isActionPressed(InputAction::PAUSE)) {
        // Would push pause state via state manager
        // StateManager would need to be globally accessible
    }
}

const std::string& PlayingState::getStateId() const {
    return STATE_ID;
}

// PausedState implementation
PausedState::PausedState() : m_selectedOption(0) {
    m_menuOptions = {"Resume", "Settings", "Quit to Menu"};
}

void PausedState::onEnter() {
    // Pause state entered
}

void PausedState::onExit() {
    // Pause state exited
}

void PausedState::update(float deltaTime) {
    // Update any pause menu animations
}

void PausedState::render(float interpolation) {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    // Semi-transparent overlay
    DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.5f));
    
    // Pause menu background
    int menuWidth = 300;
    int menuHeight = 200;
    int menuX = (screenWidth - menuWidth) / 2;
    int menuY = (screenHeight - menuHeight) / 2;
    
    DrawRectangle(menuX, menuY, menuWidth, menuHeight, DARKGRAY);
    DrawRectangleLines(menuX, menuY, menuWidth, menuHeight, WHITE);
    
    // Title
    const char* title = "PAUSED";
    int titleWidth = MeasureText(title, 30);
    DrawText(title, menuX + (menuWidth - titleWidth) / 2, menuY + 20, 30, WHITE);
    
    // Menu options
    for (size_t i = 0; i < m_menuOptions.size(); ++i) {
        Color textColor = (i == m_selectedOption) ? YELLOW : WHITE;
        int optionY = menuY + 70 + i * 30;
        
        const char* option = m_menuOptions[i].c_str();
        int optionWidth = MeasureText(option, 20);
        DrawText(option, menuX + (menuWidth - optionWidth) / 2, optionY, 20, textColor);
    }
}

void PausedState::handleInput(float deltaTime) {
    auto& inputManager = GetInputManager();
    
    if (inputManager.isActionPressed(InputAction::MENU_UP)) {
        m_selectedOption = (m_selectedOption - 1 + m_menuOptions.size()) % m_menuOptions.size();
    } else if (inputManager.isActionPressed(InputAction::MENU_DOWN)) {
        m_selectedOption = (m_selectedOption + 1) % m_menuOptions.size();
    } else if (inputManager.isActionPressed(InputAction::MENU_SELECT) || 
               inputManager.isActionPressed(InputAction::INTERACT)) {
        // Handle selection
        switch (m_selectedOption) {
            case 0: // Resume
                // Pop this state to return to game
                break;
            case 1: // Settings
                // Could push settings state
                break;
            case 2: // Quit to Menu
                // Change to menu state
                break;
        }
    } else if (inputManager.isActionPressed(InputAction::PAUSE) || 
               inputManager.isActionPressed(InputAction::MENU_BACK)) {
        // Resume game
    }
}

const std::string& PausedState::getStateId() const {
    return STATE_ID;
}

// MenuState implementation
MenuState::MenuState() : m_selectedOption(0), m_fadeAlpha(0.0f), m_transitioning(false) {
    m_menuOptions = {"New Game", "Continue", "Settings", "Quit"};
}

void MenuState::onEnter() {
    m_fadeAlpha = 0.0f;
    m_transitioning = false;
}

void MenuState::onExit() {
    // Menu state exited
}

void MenuState::update(float deltaTime) {
    if (m_transitioning) {
        m_fadeAlpha += deltaTime * 2.0f;
        if (m_fadeAlpha >= 1.0f) {
            m_fadeAlpha = 1.0f;
            // Transition complete, change state
        }
    } else {
        // Fade in
        if (m_fadeAlpha < 1.0f) {
            m_fadeAlpha += deltaTime * 2.0f;
            m_fadeAlpha = Clamp(m_fadeAlpha, 0.0f, 1.0f);
        }
    }
}

void MenuState::render(float interpolation) {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    ClearBackground(BLACK);
    
    // Title
    const char* title = "DEAD CELLS CLONE";
    int titleSize = 40;
    int titleWidth = MeasureText(title, titleSize);
    DrawText(title, (screenWidth - titleWidth) / 2, screenHeight / 4, titleSize, 
             Fade(WHITE, m_fadeAlpha));
    
    // Menu options
    int startY = screenHeight / 2;
    for (size_t i = 0; i < m_menuOptions.size(); ++i) {
        Color textColor = (i == m_selectedOption) ? YELLOW : WHITE;
        textColor = Fade(textColor, m_fadeAlpha);
        
        const char* option = m_menuOptions[i].c_str();
        int optionWidth = MeasureText(option, 25);
        DrawText(option, (screenWidth - optionWidth) / 2, startY + i * 40, 25, textColor);
    }
    
    // Transition overlay
    if (m_transitioning) {
        DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, m_fadeAlpha));
    }
}

void MenuState::handleInput(float deltaTime) {
    if (m_transitioning) return;
    
    auto& inputManager = GetInputManager();
    
    if (inputManager.isActionPressed(InputAction::MENU_UP)) {
        m_selectedOption = (m_selectedOption - 1 + m_menuOptions.size()) % m_menuOptions.size();
    } else if (inputManager.isActionPressed(InputAction::MENU_DOWN)) {
        m_selectedOption = (m_selectedOption + 1) % m_menuOptions.size();
    } else if (inputManager.isActionPressed(InputAction::MENU_SELECT) || 
               inputManager.isActionPressed(InputAction::INTERACT)) {
        // Handle selection
        switch (m_selectedOption) {
            case 0: // New Game
                m_transitioning = true;
                break;
            case 1: // Continue
                // Load saved game
                break;
            case 2: // Settings
                // Push settings state
                break;
            case 3: // Quit
                // Quit application
                break;
        }
    }
}

const std::string& MenuState::getStateId() const {
    return STATE_ID;
}

// GameOverState implementation
GameOverState::GameOverState(int finalScore, float survivalTime) 
    : m_finalScore(finalScore)
    , m_survivalTime(survivalTime)
    , m_selectedOption(0)
    , m_displayTimer(0.0f)
{
    m_menuOptions = {"Restart", "Main Menu", "Quit"};
}

void GameOverState::onEnter() {
    m_displayTimer = 0.0f;
}

void GameOverState::onExit() {
    // Game over state exited
}

void GameOverState::update(float deltaTime) {
    m_displayTimer += deltaTime;
}

void GameOverState::render(float interpolation) {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    // Dark overlay
    DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.8f));
    
    // Game Over text
    const char* gameOverText = "GAME OVER";
    int titleSize = 50;
    int titleWidth = MeasureText(gameOverText, titleSize);
    DrawText(gameOverText, (screenWidth - titleWidth) / 2, screenHeight / 4, titleSize, RED);
    
    // Stats
    if (m_displayTimer > 1.0f) {
        char scoreText[64];
        snprintf(scoreText, sizeof(scoreText), "Final Score: %d", m_finalScore);
        int scoreWidth = MeasureText(scoreText, 25);
        DrawText(scoreText, (screenWidth - scoreWidth) / 2, screenHeight / 2 - 50, 25, WHITE);
        
        char timeText[64];
        snprintf(timeText, sizeof(timeText), "Survival Time: %.1fs", m_survivalTime);
        int timeWidth = MeasureText(timeText, 25);
        DrawText(timeText, (screenWidth - timeWidth) / 2, screenHeight / 2 - 20, 25, WHITE);
    }
    
    // Menu options
    if (m_displayTimer > 2.0f) {
        int startY = screenHeight / 2 + 50;
        for (size_t i = 0; i < m_menuOptions.size(); ++i) {
            Color textColor = (i == m_selectedOption) ? YELLOW : WHITE;
            
            const char* option = m_menuOptions[i].c_str();
            int optionWidth = MeasureText(option, 25);
            DrawText(option, (screenWidth - optionWidth) / 2, startY + i * 35, 25, textColor);
        }
    }
}

void GameOverState::handleInput(float deltaTime) {
    if (m_displayTimer < 2.0f) return; // Don't allow input until stats are shown
    
    auto& inputManager = GetInputManager();
    
    if (inputManager.isActionPressed(InputAction::MENU_UP)) {
        m_selectedOption = (m_selectedOption - 1 + m_menuOptions.size()) % m_menuOptions.size();
    } else if (inputManager.isActionPressed(InputAction::MENU_DOWN)) {
        m_selectedOption = (m_selectedOption + 1) % m_menuOptions.size();
    } else if (inputManager.isActionPressed(InputAction::MENU_SELECT) || 
               inputManager.isActionPressed(InputAction::INTERACT)) {
        // Handle selection
        switch (m_selectedOption) {
            case 0: // Restart
                // Change to playing state
                break;
            case 1: // Main Menu
                // Change to menu state
                break;
            case 2: // Quit
                // Quit application
                break;
        }
    }
}

const std::string& GameOverState::getStateId() const {
    return STATE_ID;
}

// LoadingState implementation
LoadingState::LoadingState(std::function<void()> loadFunction, const std::string& nextStateId)
    : m_loadFunction(std::move(loadFunction))
    , m_nextStateId(nextStateId)
    , m_loadingComplete(false)
    , m_loadingProgress(0.0f)
    , m_loadingText("Loading")
    , m_dotAnimation(0.0f)
    , m_animationTime(0.0f)
    , m_pulseAnimation(0.0f)
    , m_rotationAnimation(0.0f)
{
}

void LoadingState::onEnter() {
    m_loadingComplete = false;
    m_loadingProgress = 0.0f;
    m_dotAnimation = 0.0f;
    m_animationTime = 0.0f;
    m_pulseAnimation = 0.0f;
    m_rotationAnimation = 0.0f;
}

void LoadingState::onExit() {
    // Nothing to clean up
}

void LoadingState::update(float deltaTime) {
    m_dotAnimation += deltaTime * 2.0f;
    m_animationTime += deltaTime;
    m_pulseAnimation += deltaTime * 3.0f;
    m_rotationAnimation += deltaTime * 2.0f;
    
    if (!m_loadingComplete) {
        m_loadingProgress += deltaTime * 0.5f;
        
        if (m_loadingProgress >= 1.0f) {
            m_loadingProgress = 1.0f;
            
            if (m_loadFunction) {
                m_loadFunction();
            }
            
            m_loadingComplete = true;
        }
    }
}

void LoadingState::render(float interpolation) {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    ClearBackground(BLACK);
    
    std::string loadingDisplay = m_loadingText;
    int dots = (int)(m_dotAnimation) % 4;
    for (int i = 0; i < dots; ++i) {
        loadingDisplay += ".";
    }
    
    int textWidth = MeasureText(loadingDisplay.c_str(), 30);
    DrawText(loadingDisplay.c_str(), (screenWidth - textWidth) / 2, screenHeight / 2 - 50, 30, WHITE);
    
    int barWidth = 400;
    int barHeight = 20;
    int barX = (screenWidth - barWidth) / 2;
    int barY = screenHeight / 2;
    
    DrawRectangle(barX, barY, barWidth, barHeight, DARKGRAY);
    DrawRectangle(barX, barY, (int)(barWidth * m_loadingProgress), barHeight, GREEN);
    DrawRectangleLines(barX, barY, barWidth, barHeight, WHITE);
    
    char progressText[16];
    snprintf(progressText, sizeof(progressText), "%.0f%%", m_loadingProgress * 100.0f);
    int progressWidth = MeasureText(progressText, 20);
    DrawText(progressText, (screenWidth - progressWidth) / 2, barY + 30, 20, WHITE);
    
    float centerX = screenWidth / 2.0f;
    float centerY = screenHeight / 2.0f + 100.0f;
    
    for (int i = 0; i < 8; ++i) {
        float angle = m_rotationAnimation + i * 45.0f * DEG2RAD;
        float radius = 60.0f + 20.0f * sin(m_pulseAnimation + i * 0.5f);
        float x = centerX + cos(angle) * radius;
        float y = centerY + sin(angle) * radius;
        
        float pulseSize = 8.0f + 4.0f * sin(m_pulseAnimation * 2.0f + i * 0.3f);
        Color orbitColor = {
            (unsigned char)(100 + 155 * sin(m_animationTime + i * 0.7f)),
            (unsigned char)(150 + 105 * cos(m_animationTime * 1.2f + i * 0.4f)),
            (unsigned char)(200 + 55 * sin(m_animationTime * 0.8f + i * 0.6f)),
            255
        };
        
        DrawCircle((int)x, (int)y, pulseSize, orbitColor);
        DrawCircleLines((int)x, (int)y, pulseSize + 2.0f, Fade(WHITE, 0.6f));
    }
    
    for (int i = 0; i < 12; ++i) {
        float angle = -m_rotationAnimation * 0.7f + i * 30.0f * DEG2RAD;
        float radius = 35.0f + 10.0f * cos(m_animationTime * 1.5f + i * 0.4f);
        float x = centerX + cos(angle) * radius;
        float y = centerY + sin(angle) * radius;
        
        float size = 3.0f + 2.0f * sin(m_animationTime * 3.0f + i * 0.8f);
        Color sparkleColor = {255, 255, 255, (unsigned char)(100 + 155 * sin(m_animationTime * 2.0f + i))};
        
        DrawCircle((int)x, (int)y, size, sparkleColor);
    }
    
    float waveAmplitude = 15.0f;
    float waveFrequency = 0.02f;
    for (int x = 0; x < screenWidth; x += 4) {
        float waveY = centerY + 150.0f + waveAmplitude * sin(x * waveFrequency + m_animationTime * 2.0f);
        float intensity = 0.3f + 0.4f * sin(x * 0.01f + m_animationTime);
        Color waveColor = {
            (unsigned char)(0 + 100 * intensity),
            (unsigned char)(100 + 155 * intensity),
            (unsigned char)(200 + 55 * intensity),
            (unsigned char)(50 + 100 * intensity)
        };
        
        DrawPixel(x, (int)waveY, waveColor);
        DrawPixel(x, (int)waveY + 1, Fade(waveColor, 0.5f));
    }
    
    float glowRadius = 80.0f + 20.0f * sin(m_pulseAnimation * 0.8f);
    DrawCircleGradient((int)centerX, (int)centerY, glowRadius, 
                      Fade(SKYBLUE, 0.05f), Fade(BLUE, 0.0f));
}

void LoadingState::handleInput(float deltaTime) {
    // No input handling during loading
}

const std::string& LoadingState::getStateId() const {
    return STATE_ID;
}

} // namespace Core
