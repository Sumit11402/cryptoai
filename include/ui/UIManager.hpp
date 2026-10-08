#pragma once

#include "ui/Views.hpp"
#include "ui/Theme.hpp"
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>

struct GLFWwindow;

namespace crypto {

struct ToastNotification {
    std::string title;
    std::string message;
    ImVec4 color;
    double durationSec{4.0};
    double elapsedSec{0.0};
};

class UIManager {
public:
    static UIManager& instance();

    bool init(GLFWwindow* window);
    void shutdown();

    void render(double deltaTime);
    void setActiveView(ViewType view);
    ViewType getActiveView() const { return m_currentView; }

    void showToast(const std::string& title, const std::string& message, const ImVec4& color = Theme::SkyBlue());
    void openCommandPalette() { m_showCommandPalette = true; }

private:
    UIManager() = default;
    ~UIManager() = default;

    void renderHeaderBar();
    void renderSidebar();
    void renderStatusBar();
    void renderToasts(double deltaTime);
    void renderCommandPalette();

    GLFWwindow* m_window{nullptr};
    ViewType m_currentView{ViewType::DASHBOARD};

    std::unordered_map<ViewType, std::unique_ptr<IView>> m_views;
    std::vector<ToastNotification> m_toasts;

    char m_symbolSearchBuf[64]{""};
    char m_commandSearchBuf[128]{""};
    bool m_showCommandPalette{false};
};

} // namespace crypto

