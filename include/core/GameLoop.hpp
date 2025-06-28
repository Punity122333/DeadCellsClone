#ifndef CORE_GAMELOOP_HPP
#define CORE_GAMELOOP_HPP

#include <functional>
#include <chrono>

namespace Core {

class GameLoop {
public:
    using UpdateCallback = std::function<void(float deltaTime)>;
    using RenderCallback = std::function<void(float interpolation)>;

    GameLoop(int targetFPS = 60, int targetUPS = 60);
    ~GameLoop() = default;

    void setUpdateCallback(UpdateCallback callback);
    void setRenderCallback(RenderCallback callback);
    bool run();
    void stop();
    bool isRunning() const;
    float getCurrentFPS() const;
    float getCurrentUPS() const;
    float getAverageFrameTime() const;
    void setVSync(bool enabled);

private:
    struct PerformanceMetrics {
        float currentFPS = 0.0f;
        float currentUPS = 0.0f;
        float averageFrameTime = 0.0f;
        int frameCount = 0;
        int updateCount = 0;
        std::chrono::steady_clock::time_point lastMetricUpdate;
    };

    int m_targetFPS;
    int m_targetUPS;
    bool m_running;
    bool m_vSyncEnabled;
    
    UpdateCallback m_updateCallback;
    RenderCallback m_renderCallback;
    
    PerformanceMetrics m_metrics;
    
    std::chrono::steady_clock::time_point m_lastTime;
    double m_accumulator;
    double m_fixedDeltaTime;
    
    void updateMetrics();
    void limitFrameRate();
};

} 

#endif
