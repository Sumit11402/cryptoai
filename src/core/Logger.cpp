#include "core/Logger.hpp"
#include <regex>

namespace crypto {

Logger& Logger::instance() {
    static Logger s_instance;
    return s_instance;
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE:    return "[TRACE]";
        case LogLevel::DEBUG:    return "[DEBUG]";
        case LogLevel::INFO:     return "[INFO]";
        case LogLevel::WARN:     return "[WARN]";
        case LogLevel::ERROR:    return "[ERROR]";
        case LogLevel::CRITICAL: return "[CRITICAL]";
    }
    return "[INFO]";
}

std::string Logger::currentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

std::string Logger::sanitize(const std::string& input) {
    // Mask potential API keys (gsk_*, sk-*, bearer tokens, hex secrets)
    std::string sanitized = input;
    static const std::regex groqRegex("(gsk_[a-zA-Z0-9_-]{8})[a-zA-Z0-9_-]+");
    sanitized = std::regex_replace(sanitized, groqRegex, "$1••••••••••••");

    static const std::regex bearerRegex("(Bearer\\s+)[a-zA-Z0-9_.-]{10,}");
    sanitized = std::regex_replace(sanitized, bearerRegex, "$1••••••••••••");

    static const std::regex secretRegex("([sS]ecret|[kK]ey|[tT]oken)\\s*[:=]\\s*([\"']?)[a-zA-Z0-9_.-]{8,}([\"']?)");
    sanitized = std::regex_replace(sanitized, secretRegex, "$1: $2••••••••••••$3");

    return sanitized;
}

void Logger::log(LogLevel level, const std::string& message) {
    if (level < m_minLevel) return;

    std::string cleanMsg = sanitize(message);
    std::string timeStr = currentTimestamp();
    std::string lvlStr = levelToString(level);

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_buffer.push_back({level, timeStr, cleanMsg});
        if (m_buffer.size() > m_maxCapacity) {
            m_buffer.pop_front();
        }
    }

    std::cout << timeStr << " " << lvlStr << " " << cleanMsg << std::endl;
}

std::vector<LogEntry> Logger::getEntries(size_t maxCount) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_buffer.empty()) return {};

    size_t count = std::min(maxCount, m_buffer.size());
    std::vector<LogEntry> result;
    result.reserve(count);
    auto it = m_buffer.end() - count;
    result.insert(result.end(), it, m_buffer.end());
    return result;
}

void Logger::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_buffer.clear();
}

} // namespace crypto
