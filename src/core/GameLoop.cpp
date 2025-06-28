#include "core/GameLoop.hpp"
#include <raylib.h>
#include <thread>
#include <algorithm>

namespace Core {

GameLoop::GameLoop(int targetFPS, int targetUPS) 
    : m_targetFPS(targetFPS)
    , m_targetUPS(targetUPS)
    , m_running(false)
    , m_vSyncEnabled(false)
    , m_accumulator(0.0)
    , m_fixedDeltaTime(1.0 / targetUPS)
{
    m_metrics.lastMetricUpdate = std::chrono::steady_clock::now();
}

void GameLoop::setUpdateCallback(UpdateCallback callback) {
    m_updateCallback = std::move(callback);
}

void GameLoop::setRenderCallback(RenderCallback callback) {
    m_renderCallback = std::move(callback);
}

bool GameLoop::run() {
    if (!m_updateCallback || !m_renderCallback) {
        return false;
    }

    m_running = true;
    m_lastTime = std::chrono::steady_clock::now();
    
    while (m_running && !WindowShouldClose()) {
        auto currentTime = std::chrono::steady_clock::now();
        auto frameTime = std::chrono::duration<double>(currentTime - m_lastTime).count();
        m_lastTime = currentTime;

        frameTime = std::min(frameTime, 0.25);
        
        m_accumulator += frameTime;

        while (m_accumulator >= m_fixedDeltaTime) {
            m_updateCallback(static_cast<float>(m_fixedDeltaTime));
            m_accumulator -= m_fixedDeltaTime;
            m_metrics.updateCount++;
        }

        float interpolation = static_cast<float>(m_accumulator / m_fixedDeltaTime);
        
        
        m_renderCallback(interpolation);
        
        m_metrics.frameCount++;
        updateMetrics();
        
        if (!m_vSyncEnabled) {
            limitFrameRate();
        }
    }

    return true;
}

void GameLoop::stop() {
    m_running = false;
}

bool GameLoop::isRunning() const {
    return m_running;
}

float GameLoop::getCurrentFPS() const {
    return m_metrics.currentFPS;
}

float GameLoop::getCurrentUPS() const {
    return m_metrics.currentUPS;
}

float GameLoop::getAverageFrameTime() const {
    return m_metrics.averageFrameTime;
}

void GameLoop::setVSync(bool enabled) {
    m_vSyncEnabled = enabled;
    if (enabled) {
        SetTargetFPS(m_targetFPS);
    } else {
        SetTargetFPS(0); // Unlimited FPS
    }
}

void GameLoop::updateMetrics() {
    auto currentTime = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<float>(currentTime - m_metrics.lastMetricUpdate).count();
    
    if (elapsed >= 1.0f) { 
        m_metrics.currentFPS = m_metrics.frameCount / elapsed;
        m_metrics.currentUPS = m_metrics.updateCount / elapsed;
        m_metrics.averageFrameTime = (elapsed * 1000.0f) / m_metrics.frameCount;
        
        m_metrics.frameCount = 0;
        m_metrics.updateCount = 0;
        m_metrics.lastMetricUpdate = currentTime;
    }
}

void GameLoop::limitFrameRate() {
    if (m_targetFPS <= 0) return;
    
    static auto lastFrameTime = std::chrono::steady_clock::now();
    auto currentTime = std::chrono::steady_clock::now();
    
    auto targetFrameDuration = std::chrono::microseconds(1000000 / m_targetFPS);
    auto elapsed = currentTime - lastFrameTime;
    
    if (elapsed < targetFrameDuration) {
        auto sleepTime = targetFrameDuration - elapsed;
        std::this_thread::sleep_for(sleepTime);
    }
    
    lastFrameTime = std::chrono::steady_clock::now();
}

}
