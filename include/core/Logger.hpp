#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <deque>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <iostream>

namespace crypto {

enum class LogLevel {
    TRACE = 0,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    CRITICAL
};

struct LogEntry {
    LogLevel level;
    std::string timestamp;
    std::string message;
};

class Logger {
public:
    static Logger& instance();

    void log(LogLevel level, const std::string& message);
    
    void trace(const std::string& msg) { log(LogLevel::TRACE, msg); }
    void debug(const std::string& msg) { log(LogLevel::DEBUG, msg); }
    void info(const std::string& msg)  { log(LogLevel::INFO, msg); }
    void warn(const std::string& msg)  { log(LogLevel::WARN, msg); }
    void error(const std::string& msg) { log(LogLevel::ERROR, msg); }
    void critical(const std::string& msg) { log(LogLevel::CRITICAL, msg); }

    std::vector<LogEntry> getEntries(size_t maxCount = 200);
    void clear();

    void setMinLevel(LogLevel level) { m_minLevel = level; }
    LogLevel getMinLevel() const { return m_minLevel; }

    static std::string sanitize(const std::string& input);

private:
    Logger() = default;
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string levelToString(LogLevel level);
    std::string currentTimestamp();

    mutable std::mutex m_mutex;
    std::deque<LogEntry> m_buffer;
    const size_t m_maxCapacity{1000};
    LogLevel m_minLevel{LogLevel::INFO};
};

#define LOG_TRACE(msg) crypto::Logger::instance().trace(msg)
#define LOG_DEBUG(msg) crypto::Logger::instance().debug(msg)
#define LOG_INFO(msg)  crypto::Logger::instance().info(msg)
#define LOG_WARN(msg)  crypto::Logger::instance().warn(msg)
#define LOG_ERROR(msg) crypto::Logger::instance().error(msg)
#define LOG_CRITICAL(msg) crypto::Logger::instance().critical(msg)

} // namespace crypto
