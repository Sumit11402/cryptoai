#pragma once

#include "core/Types.hpp"
#include <chrono>
#include <mutex>

namespace crypto {

class SystemMetricsCollector {
public:
    static SystemMetricsCollector& instance();

    void update(double frameDeltaTime);
    SystemMetrics getMetrics() const;

    void setWsConnected(bool connected);
    void setWsLatency(int64_t ms);
    void setRestLatency(int64_t ms);
    void setAILatency(int64_t ms);
    void incrementDbQueries();
    void setActiveAI(const std::string& provider, const std::string& model);

private:
    SystemMetricsCollector();
    ~SystemMetricsCollector() = default;

    void sampleHardware();

    mutable std::mutex m_mutex;
    SystemMetrics m_metrics;
    std::chrono::steady_clock::time_point m_startTime;
    std::chrono::steady_clock::time_point m_lastSampleTime;

    // CPU sampling state
    unsigned long long m_prevUser{0};
    unsigned long long m_prevNice{0};
    unsigned long long m_prevSystem{0};
    unsigned long long m_prevIdle{0};
    unsigned long long m_prevIowait{0};
    unsigned long long m_prevIrq{0};
    unsigned long long m_prevSoftirq{0};
    unsigned long long m_prevSteal{0};

    // FPS averaging
    double m_accumulatedTime{0.0};
    int m_frameCount{0};
};

} // namespace crypto
