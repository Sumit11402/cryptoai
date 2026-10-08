#include "core/SystemMetrics.hpp"
#include "core/ThreadPool.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <unistd.h>

namespace crypto {

SystemMetricsCollector& SystemMetricsCollector::instance() {
    static SystemMetricsCollector s_instance;
    return s_instance;
}

SystemMetricsCollector::SystemMetricsCollector() 
    : m_startTime(std::chrono::steady_clock::now()),
      m_lastSampleTime(std::chrono::steady_clock::now()) {
    sampleHardware();
}

void SystemMetricsCollector::sampleHardware() {
    // 1. Sample CPU usage from /proc/stat
    std::ifstream statFile("/proc/stat");
    if (statFile.is_open()) {
        std::string cpuLabel;
        unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
        if (statFile >> cpuLabel >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal) {
            unsigned long long prevIdleTotal = m_prevIdle + m_prevIowait;
            unsigned long long idleTotal = idle + iowait;

            unsigned long long prevNonIdle = m_prevUser + m_prevNice + m_prevSystem + m_prevIrq + m_prevSoftirq + m_prevSteal;
            unsigned long long nonIdle = user + nice + system + irq + softirq + steal;

            unsigned long long prevTotal = prevIdleTotal + prevNonIdle;
            unsigned long long total = idleTotal + nonIdle;

            unsigned long long totalDiff = total - prevTotal;
            unsigned long long idleDiff = idleTotal - prevIdleTotal;

            if (totalDiff > 0) {
                double cpuUsage = (double)(totalDiff - idleDiff) / (double)totalDiff * 100.0;
                m_metrics.cpuUsagePercent = std::clamp(cpuUsage, 0.0, 100.0);
            }

            m_prevUser = user;
            m_prevNice = nice;
            m_prevSystem = system;
            m_prevIdle = idle;
            m_prevIowait = iowait;
            m_prevIrq = irq;
            m_prevSoftirq = softirq;
            m_prevSteal = steal;
        }
    }

    // 2. Sample Memory from /proc/self/status and /proc/meminfo
    std::ifstream statusFile("/proc/self/status");
    if (statusFile.is_open()) {
        std::string line;
        while (std::getline(statusFile, line)) {
            if (line.rfind("VmRSS:", 0) == 0) {
                std::istringstream iss(line.substr(6));
                double rssKb;
                if (iss >> rssKb) {
                    m_metrics.memoryUsageMb = rssKb / 1024.0;
                }
                break;
            }
        }
    }

    std::ifstream meminfoFile("/proc/meminfo");
    if (meminfoFile.is_open()) {
        std::string line;
        while (std::getline(meminfoFile, line)) {
            if (line.rfind("MemTotal:", 0) == 0) {
                std::istringstream iss(line.substr(9));
                double totalKb;
                if (iss >> totalKb) {
                    m_metrics.memoryTotalMb = totalKb / 1024.0;
                }
                break;
            }
        }
    }

    // 3. Threads
    m_metrics.activeThreads = ThreadPool::instance().getActiveThreadCount() + 1; // +1 for UI thread
}

void SystemMetricsCollector::update(double frameDeltaTime) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Frame timing & FPS
    m_frameCount++;
    m_accumulatedTime += frameDeltaTime;
    m_metrics.frameTimeMs = frameDeltaTime * 1000.0;

    if (m_accumulatedTime >= 0.5) {
        m_metrics.fps = (double)m_frameCount / m_accumulatedTime;
        m_frameCount = 0;
        m_accumulatedTime = 0.0;
    }

    // Hardware sampling once every 1.0s
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastSampleTime).count() >= 1000) {
        sampleHardware();
        m_lastSampleTime = now;

        // Calculate Uptime
        auto uptimeSec = std::chrono::duration_cast<std::chrono::seconds>(now - m_startTime).count();
        int hrs = uptimeSec / 3600;
        int mins = (uptimeSec % 3600) / 60;
        int secs = uptimeSec % 60;

        std::stringstream ss;
        ss << std::setfill('0') << std::setw(2) << hrs << ":"
           << std::setfill('0') << std::setw(2) << mins << ":"
           << std::setfill('0') << std::setw(2) << secs;
        m_metrics.appUptime = ss.str();
    }
}

SystemMetrics SystemMetricsCollector::getMetrics() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_metrics;
}

void SystemMetricsCollector::setWsConnected(bool connected) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_metrics.wsConnected = connected;
}

void SystemMetricsCollector::setWsLatency(int64_t ms) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_metrics.wsLatencyMs = ms;
}

void SystemMetricsCollector::setRestLatency(int64_t ms) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_metrics.restLatencyMs = ms;
}

void SystemMetricsCollector::setAILatency(int64_t ms) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_metrics.aiLatencyMs = ms;
}

void SystemMetricsCollector::incrementDbQueries() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_metrics.dbQueryCount++;
}

void SystemMetricsCollector::setActiveAI(const std::string& provider, const std::string& model) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_metrics.activeAIProvider = provider;
    m_metrics.aiModel = model;
}

} // namespace crypto
