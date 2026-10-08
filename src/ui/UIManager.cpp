#include "ui/UIManager.hpp"
#include "market/MarketManager.hpp"
#include "trading/PaperTradingEngine.hpp"
#include "ui/HackerConsole.hpp"
#include "ai/AIRouter.hpp"
#include "core/Config.hpp"
#include "core/SystemMetrics.hpp"
#include "core/Logger.hpp"
#include "core/EventBus.hpp"
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <algorithm>

namespace crypto {

static char s_searchHeaderBuf[64] = "";
static int s_currencyHeaderIdx = 0;
static bool s_isDarkMode = true;

UIManager& UIManager::instance() {
    static UIManager s_instance;
    return s_instance;
}

bool UIManager::init(GLFWwindow* window) {
    m_window = window;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    Theme::applyTheme();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Load High-DPI TrueType Font with Full Symbol Ranges
    static const ImWchar custom_ranges[] = {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x0100, 0x024F, // Latin Extended-A & B
        0x2000, 0x206F, // General Punctuation
        0x20A0, 0x20CF, // Currency Symbols ($, €, £, ¥, ₿, ₮)
        0x2100, 0x214F, // Letterlike Symbols
        0x2190, 0x21FF, // Arrows (↑, ↓, ←, →, ↗, ↘, ↖, ↙, ⇄, ⇅)
        0x2200, 0x22FF, // Mathematical Operators (≈, ≠, ≤, ≥, ±, ∓, ∞)
        0x25A0, 0x25FF, // Geometric Shapes (■, □, ▲, △, ▼, ▽, ◆, ◇, ●, ○, ◈)
        0x2600, 0x26FF, // Misc Symbols (⚙, ⚡, ☕, ⚠, ✦, ✧)
        0,
    };

    const char* fontCandidates[] = {
        "/usr/share/fonts/noto/NotoSans-Regular.ttf",
        "/usr/share/fonts/TTF/Roboto-Regular.ttf",
        "/usr/share/fonts/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/TTF/JetBrainsMono-Regular.ttf"
    };

    bool fontLoaded = false;
    for (const char* path : fontCandidates) {
        FILE* f = fopen(path, "rb");
        if (f) {
            fclose(f);
            ImFontConfig cfg;
            cfg.OversampleH = 3;
            cfg.OversampleV = 2;
            cfg.PixelSnapH = true;
            io.Fonts->AddFontFromFileTTF(path, 15.0f, &cfg, custom_ranges);
            fontLoaded = true;
            LOG_INFO("Loaded high-definition UI font: " + std::string(path));
            break;
        }
    }
    if (!fontLoaded) {
        io.Fonts->AddFontDefault();
    }

    // Initialize Views
    m_views[ViewType::DASHBOARD] = std::make_unique<DashboardView>();
    m_views[ViewType::MARKETS] = std::make_unique<MarketsView>();
    m_views[ViewType::CHART] = std::make_unique<ChartView>();
    m_views[ViewType::AI_RESEARCH] = std::make_unique<AIResearchView>();
    m_views[ViewType::PORTFOLIO] = std::make_unique<PortfolioView>();
    m_views[ViewType::WATCHLIST] = std::make_unique<WatchlistView>();
    m_views[ViewType::SCREENER] = std::make_unique<ScreenerView>();
    m_views[ViewType::PAPER_TRADING] = std::make_unique<PaperTradingView>();
    m_views[ViewType::ALERTS] = std::make_unique<AlertsView>();
    m_views[ViewType::NEWS] = std::make_unique<NewsView>();
    m_views[ViewType::BACKTESTING] = std::make_unique<BacktestingView>();
    m_views[ViewType::HACKER_CONSOLE] = std::make_unique<HackerConsoleView>();
    m_views[ViewType::SYSTEM_MONITOR] = std::make_unique<SystemMonitorView>();
    m_views[ViewType::SETTINGS] = std::make_unique<SettingsView>();

    // Subscribe to events for UI toasts
    EventBus::instance().subscribe<AlertTriggeredEvent>([this](const AlertTriggeredEvent& e) {
        showToast("ALERT", e.alert.symbol + ": " + e.message, Theme::CrimsonCoral());
    });

    EventBus::instance().subscribe<PaperTradeExecutedEvent>([this](const PaperTradeExecutedEvent& e) {
        std::string side = (e.order.side == OrderSide::BUY) ? "BUY" : "SELL";
        showToast("TRADE EXECUTED", side + " " + std::to_string(e.order.quantity).substr(0, 6) + " " + e.order.symbol + " @ $" + std::to_string(e.order.executedPrice), Theme::EmeraldGreen());
    });

    LOG_INFO("UIManager initialized with Chainblock Reference UI.");
    return true;
}

void UIManager::shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    LOG_INFO("UIManager shutdown.");
}

void UIManager::setActiveView(ViewType view) {
    m_currentView = view;
}

void UIManager::showToast(const std::string& title, const std::string& message, const ImVec4& color) {
    m_toasts.push_back({title, message, color, 4.0, 0.0});
}

void UIManager::render(double deltaTime) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGuiIO& io = ImGui::GetIO();
    if ((io.KeyCtrl || io.KeySuper) && ImGui::IsKeyPressed(ImGuiKey_K)) {
        m_showCommandPalette = !m_showCommandPalette;
        if (m_showCommandPalette) m_commandSearchBuf[0] = '\0';
    }

    if (ImGui::IsKeyPressed(ImGuiKey_GraveAccent) || ImGui::IsKeyPressed(ImGuiKey_F12)) {
        HackerConsole::instance().toggleDrawer();
    }

    ImGuiViewport* viewport = ImGui::GetMainViewport();

    // 1. Root Black Screen (#000000)
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags rootFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
                                 ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, Theme::PureBlack());
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Chainblock_Root", nullptr, rootFlags);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    // 2. Draw 3D Abstract Dark Flowing Ribbon / Silk in Background
    ImDrawList* bgDraw = ImGui::GetWindowDrawList();
    ImVec2 vMin = viewport->WorkPos;
    ImVec2 vMax = ImVec2(vMin.x + viewport->WorkSize.x, vMin.y + viewport->WorkSize.y);

    // Layered Dark Gray Silk Curves (#141414 -> #222222)
    for (int r = 0; r < 12; ++r) {
        float off = r * 28.0f;
        ImU32 col = IM_COL32(18 + r * 2, 18 + r * 2, 20 + r * 2, 140);
        bgDraw->AddBezierCubic(
            ImVec2(vMin.x - 80.0f, vMax.y - 180.0f + off),
            ImVec2(vMin.x + viewport->WorkSize.x * 0.30f, vMin.y + 40.0f + off),
            ImVec2(vMin.x + viewport->WorkSize.x * 0.70f, vMax.y + 120.0f - off),
            ImVec2(vMax.x + 80.0f, vMin.y + 120.0f + off),
            col,
            32.0f
        );
    }

    // 3. Full-Size Responsive Dashboard (Expands cleanly to full viewport)
    float padX = 14.0f;
    float padY = 12.0f;
    float dashW = viewport->WorkSize.x - (2.0f * padX);
    float dashH = viewport->WorkSize.y - (2.0f * padY);

    ImGui::SetCursorPos(ImVec2(padX, padY));

    // Outer Dashboard Floating Frame
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::MainDashboardBg());
    ImGui::PushStyleColor(ImGuiCol_Border, Theme::BorderGlow());
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 14.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);

    ImGui::BeginChild("FloatingDashboard", ImVec2(dashW, dashH), true, ImGuiWindowFlags_NoScrollbar);
    {
        // 18-20% Sidebar Width
        float sidebarW = std::max(180.0f, std::min(220.0f, dashW * 0.18f));
        float contentW = dashW - sidebarW - 1.0f;

        // Left Sidebar (#101010)
        ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::SidebarBg());
        ImGui::BeginChild("SidebarPane", ImVec2(sidebarW, dashH), false, ImGuiWindowFlags_NoScrollbar);
        renderSidebar();
        ImGui::EndChild();
        ImGui::PopStyleColor();

        ImGui::SameLine(0, 0);

        // Vertical Divider
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 p1 = ImGui::GetCursorScreenPos();
        dl->AddLine(ImVec2(p1.x, p1.y), ImVec2(p1.x, p1.y + dashH), IM_COL32(42, 42, 42, 255), 1.0f);

        // Right Main Content (#141414)
        ImGui::BeginChild("MainContentPane", ImVec2(contentW, dashH), false, ImGuiWindowFlags_NoScrollbar);
        {
            // Header Bar (55-60px)
            renderHeaderBar();

            // Main Workspace View
            float mainViewH = dashH - 60.0f;
            ImGui::SetCursorPos(ImVec2(18, 58));
            ImGui::BeginChild("ActiveWorkspaceView", ImVec2(contentW - 36.0f, mainViewH - 12.0f), false, ImGuiWindowFlags_NoScrollbar);
            auto it = m_views.find(m_currentView);
            if (it != m_views.end() && it->second) {
                it->second->render();
            }
            ImGui::EndChild();
        }
        ImGui::EndChild();
    }
    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);

    // Toast Notifications Overlay
    renderToasts(deltaTime);

    // Universal Command Palette Overlay
    if (m_showCommandPalette) {
        renderCommandPalette();
    }

    ImGui::End(); // Chainblock_Root

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void UIManager::renderHeaderBar() {
    float availW = ImGui::GetContentRegionAvail().x;
    ImGui::SetCursorPos(ImVec2(20, 16));

    // "Dashboard" Title Header
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::CrystalWhite());
    ImGui::SetWindowFontScale(1.25f);
    ImGui::TextUnformatted("Dashboard");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();

    // Right Controls: Search, USDT Selector, Profile
    float rightW = 380.0f;
    float startX = std::max(220.0f, availW - rightW);
    ImGui::SameLine(startX);

    // Search Field
    ImGui::PushStyleColor(ImGuiCol_FrameBg, Theme::CardSurface());
    ImGui::PushStyleColor(ImGuiCol_Border, Theme::BorderGlow());
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::SetNextItemWidth(140.0f);
    ImGui::InputTextWithHint("##HeaderSearch", "Search...", s_searchHeaderBuf, sizeof(s_searchHeaderBuf));
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);

    // USDT Currency Selector Pill
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, Theme::CardSurface());
    ImGui::PushStyleColor(ImGuiCol_Border, Theme::BorderGlow());
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    const char* currLabels[] = {"USDT ▼", "USD ▼", "EUR ▼"};
    if (ImGui::Button(currLabels[s_currencyHeaderIdx], ImVec2(90, 28))) {
        s_currencyHeaderIdx = (s_currencyHeaderIdx + 1) % 3;
    }
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);

    // User Profile Selector Pill
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, Theme::CardSurface());
    ImGui::PushStyleColor(ImGuiCol_Border, Theme::BorderGlow());
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    if (ImGui::Button("● Jeff Sh...", ImVec2(100, 28))) {
        m_currentView = ViewType::SETTINGS;
    }
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
}

void UIManager::renderSidebar() {
    float sidebarW = ImGui::GetContentRegionAvail().x;
    ImGui::SetCursorPos(ImVec2(18, 16));

    // Brand Wordmark: chainblack / chainblock
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::CrystalWhite());
    ImGui::SetWindowFontScale(1.18f);
    ImGui::TextUnformatted("chainblack");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::Spacing();

    // Helper for navigation item matching reference
    auto renderNavItem = [this, sidebarW](ViewType type, const char* label, const char* icon = nullptr, const char* badge = nullptr) {
        bool isSel = (m_currentView == type);

        if (isSel) {
            ImGui::PushStyleColor(ImGuiCol_Button, Theme::CardHover());
            ImGui::PushStyleColor(ImGuiCol_Text, Theme::CrystalWhite());
            ImGui::PushStyleColor(ImGuiCol_Border, Theme::EmeraldGreen());
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_Text, Theme::TextMuted());
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        }

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.06f, 0.5f));

        std::string fullLabel;
        if (icon) fullLabel += std::string(icon) + "  ";
        fullLabel += label;

        if (ImGui::Button(fullLabel.c_str(), ImVec2(sidebarW - 32.0f, 32))) {
            m_currentView = type;
        }

        if (badge) {
            ImGui::SameLine(sidebarW - 60.0f);
            ImGui::PushStyleColor(ImGuiCol_Button, Theme::CrimsonCoral());
            ImGui::PushStyleColor(ImGuiCol_Text, Theme::CrystalWhite());
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 1));
            ImGui::Button(badge);
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(2);
        }

        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(3);
    };

    // SECTION: OVERVIEW
    ImGui::SetCursorPosX(18);
    ImGui::TextColored(Theme::TextDim(), "OVERVIEW");
    ImGui::Spacing();

    renderNavItem(ViewType::DASHBOARD, "Dashboard", "◆");
    renderNavItem(ViewType::MARKETS, "Market", "▲");
    renderNavItem(ViewType::PORTFOLIO, "Portfolio", "■");
    renderNavItem(ViewType::PAPER_TRADING, "Transactions", "≡");
    renderNavItem(ViewType::NEWS, "News", "§");

    ImGui::Spacing();
    ImGui::Spacing();

    // SECTION: ACCOUNT
    ImGui::SetCursorPosX(18);
    ImGui::TextColored(Theme::TextDim(), "ACCOUNT");
    ImGui::Spacing();

    renderNavItem(ViewType::ALERTS, "Notifications", "●", "12");
    renderNavItem(ViewType::AI_RESEARCH, "Community", "◈");
    renderNavItem(ViewType::SETTINGS, "Settings", "⚙");

    // Bottom Navigation (Support, Feedback, Theme Switcher)
    float bottomY = ImGui::GetWindowHeight() - 120.0f;
    if (bottomY > 380.0f) {
        ImGui::SetCursorPosY(bottomY);
    }

    renderNavItem(ViewType::HACKER_CONSOLE, "Get Support", "?");
    renderNavItem(ViewType::SYSTEM_MONITOR, "Add Feedback", "+");

    ImGui::Spacing();

    // Theme Switcher Toggle Pill: [ Light | Dark ]
    ImGui::SetCursorPosX(16);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::CardSurface());
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
    ImGui::BeginChild("ThemeTogglePill", ImVec2(sidebarW - 32.0f, 32.0f), true, ImGuiWindowFlags_NoScrollbar);

    ImGui::SetCursorPos(ImVec2(4, 3));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

    if (!s_isDarkMode) ImGui::PushStyleColor(ImGuiCol_Button, Theme::CardHover());
    else ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    if (ImGui::Button("Light", ImVec2((sidebarW - 48.0f) * 0.5f, 24))) s_isDarkMode = false;
    ImGui::PopStyleColor();

    ImGui::SameLine();
    if (s_isDarkMode) ImGui::PushStyleColor(ImGuiCol_Button, Theme::CardHover());
    else ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    if (ImGui::Button("Dark", ImVec2((sidebarW - 48.0f) * 0.5f, 24))) s_isDarkMode = true;
    ImGui::PopStyleColor();

    ImGui::PopStyleVar();
    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

void UIManager::renderStatusBar() {
    // Unused in minimal floating layout
}

void UIManager::renderToasts(double deltaTime) {
    if (m_toasts.empty()) return;

    float yOffset = 40.0f;
    for (auto it = m_toasts.begin(); it != m_toasts.end();) {
        it->elapsedSec += deltaTime;
        if (it->elapsedSec >= it->durationSec) {
            it = m_toasts.erase(it);
            continue;
        }

        ImGuiViewport* vp = ImGui::GetMainViewport();
        float toastW = 320.0f;
        float toastH = 65.0f;
        float xPos = vp->WorkSize.x - toastW - 24.0f;

        ImGui::SetNextWindowPos(ImVec2(xPos, yOffset));
        ImGui::SetNextWindowSize(ImVec2(toastW, toastH));

        ImGui::PushStyleColor(ImGuiCol_WindowBg, Theme::CardSurface());
        ImGui::PushStyleColor(ImGuiCol_Border, it->color);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);

        std::string winName = "Toast_" + std::to_string((uintptr_t)&(*it));
        ImGui::Begin(winName.c_str(), nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
        
        ImGui::PushStyleColor(ImGuiCol_Text, it->color);
        ImGui::TextUnformatted(it->title.c_str());
        ImGui::PopStyleColor();

        ImGui::TextWrapped("%s", it->message.c_str());

        ImGui::End();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);

        yOffset += toastH + 8.0f;
        ++it;
    }
}

void UIManager::renderCommandPalette() {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    float modalW = std::min(640.0f, vp->WorkSize.x - 40.0f);
    float modalH = 400.0f;
    float posX = (vp->WorkSize.x - modalW) * 0.5f;
    float posY = 80.0f;

    ImGui::SetNextWindowPos(ImVec2(posX, posY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(modalW, modalH), ImGuiCond_Always);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, Theme::CardSurface());
    ImGui::PushStyleColor(ImGuiCol_Border, Theme::BorderLight());
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 16.0f));

    if (ImGui::Begin("##CommandPaletteModal", &m_showCommandPalette, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings)) {
        ImGui::TextColored(Theme::EmeraldGreen(), "COMMAND PALETTE & SEARCH");
        ImGui::SameLine(modalW - 80.0f);
        if (ImGui::Button("Close")) m_showCommandPalette = false;

        ImGui::Separator();
        ImGui::Spacing();

        ImGui::SetNextItemWidth(-1);
        if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
        ImGui::InputTextWithHint("##CommandInput", "Type command or symbol (BTC, ETH, SOL)...", m_commandSearchBuf, IM_ARRAYSIZE(m_commandSearchBuf));

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        std::string query = m_commandSearchBuf;
        std::transform(query.begin(), query.end(), query.begin(), ::tolower);

        struct PaletteAction {
            std::string label;
            std::function<void()> action;
        };

        std::vector<PaletteAction> actions = {
            {"Open Dashboard Overview", [this]() { m_currentView = ViewType::DASHBOARD; m_showCommandPalette = false; }},
            {"Open Live Markets Table", [this]() { m_currentView = ViewType::MARKETS; m_showCommandPalette = false; }},
            {"Open Candlestick Chart", [this]() { m_currentView = ViewType::CHART; m_showCommandPalette = false; }},
            {"Open AI Research Copilot", [this]() { m_currentView = ViewType::AI_RESEARCH; m_showCommandPalette = false; }},
            {"Open Portfolio Ledger", [this]() { m_currentView = ViewType::PORTFOLIO; m_showCommandPalette = false; }},
            {"Open Paper Trading Operations", [this]() { m_currentView = ViewType::PAPER_TRADING; m_showCommandPalette = false; }},
            {"Open System Settings", [this]() { m_currentView = ViewType::SETTINGS; m_showCommandPalette = false; }},
            {"Switch to BTCUSDT", [this]() { MarketManager::instance().setActiveSymbol("BTCUSDT"); m_showCommandPalette = false; }},
            {"Switch to ETHUSDT", [this]() { MarketManager::instance().setActiveSymbol("ETHUSDT"); m_showCommandPalette = false; }},
            {"Switch to SOLUSDT", [this]() { MarketManager::instance().setActiveSymbol("SOLUSDT"); m_showCommandPalette = false; }}
        };

        ImGui::BeginChild("CmdList", ImVec2(0, 0), false);
        for (size_t i = 0; i < actions.size(); ++i) {
            const auto& a = actions[i];
            std::string l = a.label;
            std::transform(l.begin(), l.end(), l.begin(), ::tolower);
            if (!query.empty() && l.find(query) == std::string::npos) continue;

            if (ImGui::Button(a.label.c_str(), ImVec2(-1, 32))) {
                a.action();
            }
        }
        ImGui::EndChild();

        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) m_showCommandPalette = false;
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
}

} // namespace crypto
