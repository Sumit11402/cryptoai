#pragma once

namespace crypto {

enum class ViewType {
    DASHBOARD = 0,
    MARKETS,
    CHART,
    AI_RESEARCH,
    PORTFOLIO,
    WATCHLIST,
    SCREENER,
    PAPER_TRADING,
    ALERTS,
    NEWS,
    BACKTESTING,
    HACKER_CONSOLE,
    SYSTEM_MONITOR,
    SETTINGS
};

class IView {
public:
    virtual ~IView() = default;
    virtual void render() = 0;
};

// View Declarations
class DashboardView : public IView { public: void render() override; };
class MarketsView : public IView { public: void render() override; };
class ChartView : public IView { public: void render() override; };
class AIResearchView : public IView { public: void render() override; };
class PortfolioView : public IView { public: void render() override; };
class WatchlistView : public IView { public: void render() override; };
class ScreenerView : public IView { public: void render() override; };
class PaperTradingView : public IView { public: void render() override; };
class AlertsView : public IView { public: void render() override; };
class NewsView : public IView { public: void render() override; };
class BacktestingView : public IView { public: void render() override; };
class HackerConsoleView : public IView { public: void render() override; };
class SystemMonitorView : public IView { public: void render() override; };
class SettingsView : public IView { public: void render() override; };

} // namespace crypto
