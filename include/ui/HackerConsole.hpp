#pragma once

#include <string>
#include <vector>
#include <deque>
#include <mutex>
#include <imgui.h>

namespace crypto {

struct ConsoleLogEntry {
    std::string timestamp;
    std::string tag;
    std::string text;
    ImVec4 color;
};

class HackerConsole {
public:
    static HackerConsole& instance();

    void init();
    void executeCommand(const std::string& commandLine);
    void addLog(const std::string& tag, const std::string& text, const ImVec4& color);

    void render(bool isFullscreen = false);

    bool isDrawerOpen() const { return m_drawerOpen; }
    void toggleDrawer() { m_drawerOpen = !m_drawerOpen; }
    void setDrawerOpen(bool open) { m_drawerOpen = open; }

private:
    HackerConsole();
    ~HackerConsole() = default;

    void printWelcomeBanner();
    void printHelp();
    void printSysInfo();

    std::deque<ConsoleLogEntry> m_logs;
    std::vector<std::string> m_history;
    int m_historyPos{-1};
    char m_inputBuf[512]{""};
    bool m_autoScroll{true};
    bool m_scrollToBottom{false};
    bool m_drawerOpen{false};
    std::mutex m_mutex;
};

} // namespace crypto
