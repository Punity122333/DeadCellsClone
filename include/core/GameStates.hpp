#ifndef CORE_GAMESTATES_HPP
#define CORE_GAMESTATES_HPP

#include "core/StateManager.hpp"
#include <raylib.h>
#include <vector>
#include <functional>
#include <string>

class Game;
class Player;

namespace Core {

class PlayingState : public GameState {
public:
    PlayingState(Game* game);
    ~PlayingState() override = default;

    void onEnter() override;
    void onExit() override;
    void onPause() override;
    void onResume() override;

    void update(float deltaTime) override;
    void render(float interpolation) override;
    void handleInput(float deltaTime) override;

    const std::string& getStateId() const override;

    static const std::string STATE_ID;

private:
    Game* m_game;
};

class PausedState : public GameState {
public:
    PausedState();
    ~PausedState() override = default;

    void onEnter() override;
    void onExit() override;

    void update(float deltaTime) override;
    void render(float interpolation) override;
    void handleInput(float deltaTime) override;

    bool isBlocking() const override { return true; }
    bool isRenderBlocking() const override { return false; }

    const std::string& getStateId() const override;

    static const std::string STATE_ID;

private:
    int m_selectedOption;
    std::vector<std::string> m_menuOptions;
};

class MenuState : public GameState {
public:
    MenuState();
    ~MenuState() override = default;

    void onEnter() override;
    void onExit() override;

    void update(float deltaTime) override;
    void render(float interpolation) override;
    void handleInput(float deltaTime) override;

    const std::string& getStateId() const override;

    static const std::string STATE_ID;

private:
    int m_selectedOption;
    std::vector<std::string> m_menuOptions;
    float m_fadeAlpha;
    bool m_transitioning;
};

class GameOverState : public GameState {
public:
    GameOverState(int finalScore, float survivalTime);
    ~GameOverState() override = default;

    void onEnter() override;
    void onExit() override;

    void update(float deltaTime) override;
    void render(float interpolation) override;
    void handleInput(float deltaTime) override;

    const std::string& getStateId() const override;

    static const std::string STATE_ID;

private:
    int m_finalScore;
    float m_survivalTime;
    int m_selectedOption;
    std::vector<std::string> m_menuOptions;
    float m_displayTimer;
};


class LoadingState : public GameState {
public:
    LoadingState(std::function<void()> loadFunction, const std::string& nextStateId);
    ~LoadingState() override = default;

    void onEnter() override;
    void onExit() override;

    void update(float deltaTime) override;
    void render(float interpolation) override;
    void handleInput(float deltaTime) override;

    const std::string& getStateId() const override;

    static const std::string STATE_ID;

private:
    std::function<void()> m_loadFunction;
    std::string m_nextStateId;
    bool m_loadingComplete;
    float m_loadingProgress;
    std::string m_loadingText;
    float m_dotAnimation;
    float m_animationTime;
    float m_pulseAnimation;
    float m_rotationAnimation;
};

}

#endif 
