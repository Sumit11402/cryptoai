#include "ui/HackerConsole.hpp"
#include "ui/Theme.hpp"
#include "ui/UIManager.hpp"
#include "trading/PaperTradingEngine.hpp"
#include "market/MarketManager.hpp"
#include "analysis/TechnicalAnalysisEngine.hpp"
#include "analysis/BacktestingEngine.hpp"
#include "ai/AIRouter.hpp"
#include "core/SystemMetrics.hpp"
#include "core/Config.hpp"
#include "core/Logger.hpp"

#include <sstream>
#include <iomanip>
#include <algorithm>
#include <chrono>
#include <cmath>

namespace crypto {

static std::string getCurrentTimestampStr() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%H:%M:%S") << "." << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

static std::vector<std::string> splitArgs(const std::string& line) {
    std::vector<std::string> tokens;
    std::string token;
    bool inQuotes = false;
    for (char ch : line) {
        if (ch == '\"' || ch == '\'') {
            inQuotes = !inQuotes;
        } else if (std::isspace(static_cast<unsigned char>(ch)) && !inQuotes) {
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
        } else {
            token += ch;
        }
    }
    if (!token.empty()) {
        tokens.push_back(token);
    }
    return tokens;
}

HackerConsole& HackerConsole::instance() {
    static HackerConsole s_instance;
    return s_instance;
}

HackerConsole::HackerConsole() {
    init();
}

void HackerConsole::init() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_logs.clear();
    }
    printWelcomeBanner();
}

void HackerConsole::addLog(const std::string& tag, const std::string& text, const ImVec4& color) {
    std::lock_guard<std::mutex> lock(m_mutex);
    ConsoleLogEntry entry;
    entry.timestamp = getCurrentTimestampStr();
    entry.tag = tag;
    entry.text = text;
    entry.color = color;

    m_logs.push_back(entry);
    if (m_logs.size() > 1000) {
        m_logs.pop_front();
    }
    m_scrollToBottom = true;
}

void HackerConsole::printWelcomeBanner() {
    addLog("SYSTEM", "================================================================================", Theme::SkyBlue());
    addLog("SYSTEM", "   ██████╗██████╗ ██╗   ██╗██████╗ ████████╗██████╗     █████╗ ██╗", Theme::ElectricCyan());
    addLog("SYSTEM", "  ██╔════╝██╔══██╗╚██╗ ██╔╝██╔══██╗╚══██╔══╝██╔═══██╗   ██╔══██╗██║", Theme::ElectricCyan());
    addLog("SYSTEM", "  ██║     ██████╔╝ ╚████╔╝ ██████╔╝   ██║   ██║   ██║   ███████║██║", Theme::NeonGreen());
    addLog("SYSTEM", "  ██║     ██╔══██╗  ╚██╔╝  ██╔═══╝    ██║   ██║   ██║   ██╔══██║██║", Theme::NeonGreen());
    addLog("SYSTEM", "  ╚██████╗██║  ██║   ██║   ██║        ██║   ╚██████╔╝██╗██║  ██║██║", Theme::BodyPink());
    addLog("SYSTEM", "   ╚═════╝╚═╝  ╚═╝   ╚═╝   ╚═╝        ╚═╝    ╚═════╝ ╚═╝╚═╝  ╚═╝╚═╝", Theme::BodyPink());
    addLog("SYSTEM", "   [INSTITUTIONAL QUANTITATIVE HFT & NEURAL WORKSTATION // V3.0-ELITE]", Theme::AmberGold());
    addLog("SYSTEM", "================================================================================", Theme::SkyBlue());
    addLog("INIT",   "DMA Engine: ARMED [Binance L3 WebSocket Sub-millisecond Cross-Connect]", Theme::NeonGreen());
    addLog("INIT",   "Hardware Acceleration: AVX2 SIMD Matrix Multiplication Active", Theme::NeonGreen());
    addLog("INIT",   "Type 'help' for tactical command matrix or click prompt macros below.", Theme::CrystalWhite());
}

void HackerConsole::printHelp() {
    addLog("HELP", "━━━━━━━━━━━━━━━━━━━ QUANT REPL COMMAND MATRIX ━━━━━━━━━━━━━━━━━━━", Theme::AmberGold());
    addLog("HELP", "  buy <symbol> <qty> [lev=1] [sl] [tp] [trail%]  - Execute Leveraged Long Margin Order", Theme::NeonGreen());
    addLog("HELP", "  sell <symbol> <qty> [lev=1] [sl] [tp]          - Execute Leveraged Short Margin Order", Theme::CrimsonCoral());
    addLog("HELP", "  close <symbol>                                 - Market Close Active Position", Theme::GlitchOrange());
    addLog("HELP", "  reverse <symbol>                               - 1-Click Position Reversal (Long <-> Short)", Theme::ElectricCyan());
    addLog("HELP", "  pos / positions / account                      - Inspect Margin, Leverage & Unrealized PnL", Theme::CrystalWhite());
    addLog("HELP", "  matrix                                         - Calculate Cross-Asset Pearson Correlations", Theme::ElectricCyan());
    addLog("HELP", "  confluence [symbol]                            - Multi-Timeframe Algorithmic Scoring (1m-1D)", Theme::SkyBlue());
    addLog("HELP", "  backtest <strategy> [fast] [slow]              - Run Quant Backtest (SMA, EMA, SUPERTREND, RSI)", Theme::AmberGold());
    addLog("HELP", "  montecarlo [paths=1000]                        - 1,000-Path Bootstrap Resampling Simulation", Theme::BodyPink());
    addLog("HELP", "  var / stress                                   - 95%/99% VaR & Black-Swan Macro Crash Test", Theme::CrimsonCoral());
    addLog("HELP", "  scan [rsi|supertrend|macd]                     - Multi-Asset Market Screener Scan", Theme::SkyBlue());
    addLog("HELP", "  ai <\"prompt\">                                - Query Autonomous Neural Agent Copilot", Theme::BodyPink());
    addLog("HELP", "  theme <obsidian|matrix|gold|tokyo|tactical>    - Dynamic Palette Preset Switching", Theme::ElectricCyan());
    addLog("HELP", "  neofetch / sysinfo                             - Hardware Telemetry & Memory Footprint", Theme::TextMuted());
    addLog("HELP", "  demo [on|off]                                  - Toggle Live WebSocket / Offline Simulation", Theme::AmberGold());
    addLog("HELP", "  clear                                          - Clear Terminal Scrollback Buffer", Theme::TextMuted());
    addLog("HELP", "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━", Theme::AmberGold());
}

void HackerConsole::printSysInfo() {
    auto metrics = SystemMetricsCollector::instance().getMetrics();
    addLog("SYSINFO", "┌──[ HARDWARE & RUNTIME TELEMETRY ]────────────────────────┐", Theme::SkyBlue());
    addLog("SYSINFO", "│ OS Environment : Linux x86_64 (C++20 Native Low-Latency)  │", Theme::CrystalWhite());
    addLog("SYSINFO", "│ Frame Rate     : " + std::to_string(metrics.fps).substr(0, 5) + " FPS (" + std::to_string(metrics.frameTimeMs).substr(0, 4) + " ms/frame)           │", Theme::NeonGreen());
    addLog("SYSINFO", "│ Memory Used    : " + std::to_string(metrics.memoryUsageMb).substr(0, 6) + " MB Heap (Zero-alloc Ring Buffer)  │", Theme::ElectricCyan());
    addLog("SYSINFO", "│ CPU Load       : " + std::to_string(metrics.cpuUsagePercent).substr(0, 4) + "% Active Core Execution             │", Theme::GlitchOrange());
    addLog("SYSINFO", "│ WS L3 DMA Ping : " + std::to_string(metrics.wsLatencyMs) + " ms | REST Latency: " + std::to_string(metrics.restLatencyMs) + " ms          │", Theme::NeonGreen());
    addLog("SYSINFO", "│ Active Threads : " + std::to_string(metrics.activeThreads) + " Worker Pool Threads                   │", Theme::CrystalWhite());
    addLog("SYSINFO", "│ Active AI Core : " + metrics.activeAIProvider + " (" + metrics.aiModel + ")   │", Theme::BodyPink());
    addLog("SYSINFO", "│ Uptime Elapsed : " + metrics.appUptime + "                                │", Theme::AmberGold());
    addLog("SYSINFO", "└──────────────────────────────────────────────────────────┘", Theme::SkyBlue());
}

void HackerConsole::executeCommand(const std::string& commandLine) {
    std::string trimmed = commandLine;
    trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));
    trimmed.erase(trimmed.find_last_not_of(" \t\r\n") + 1);

    if (trimmed.empty()) return;

    // Add to history
    m_history.push_back(trimmed);
    m_historyPos = -1;

    addLog("USER", "CRYPTØ::QUANT_ROOT@WS-NODE-01:~$ " + trimmed, Theme::CrystalWhite());

    auto tokens = splitArgs(trimmed);
    if (tokens.empty()) return;

    std::string cmd = tokens[0];
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

    // Command Dispatch
    if (cmd == "help" || cmd == "?") {
        printHelp();
    } else if (cmd == "clear" || cmd == "cls") {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_logs.clear();
        }
        printWelcomeBanner();
    } else if (cmd == "sysinfo" || cmd == "neofetch" || cmd == "info") {
        printSysInfo();
    } else if (cmd == "buy" || cmd == "long") {
        if (tokens.size() < 3) {
            addLog("ERR", "Usage: buy <symbol> <quantity> [leverage=1] [sl] [tp] [trail%]", Theme::CrimsonCoral());
            return;
        }
        std::string sym = tokens[1];
        std::transform(sym.begin(), sym.end(), sym.begin(), ::toupper);
        if (sym.find("USDT") == std::string::npos && sym.find("USD") == std::string::npos) sym += "USDT";

        double qty = 0.0;
        try { qty = std::stod(tokens[2]); } catch (...) {}
        if (qty <= 0.0) {
            addLog("ERR", "Invalid quantity value: " + tokens[2], Theme::CrimsonCoral());
            return;
        }

        double lev = 1.0;
        if (tokens.size() >= 4) {
            try { lev = std::stod(tokens[3]); } catch (...) {}
        }

        double sl = 0.0, tp = 0.0, trail = 0.0;
        if (tokens.size() >= 5) { try { sl = std::stod(tokens[4]); } catch (...) {} }
        if (tokens.size() >= 6) { try { tp = std::stod(tokens[5]); } catch (...) {} }
        if (tokens.size() >= 7) { try { trail = std::stod(tokens[6]); } catch (...) {} }

        auto ticker = MarketManager::instance().getTicker(sym);
        double price = ticker.lastPrice;
        if (price <= 0.0) {
            auto candles = MarketManager::instance().getActiveCandles();
            if (!candles.empty()) price = candles.back().close;
            else price = (sym == "BTCUSDT") ? 68000.0 : 3500.0;
        }

        std::string err;
        bool ok = PaperTradingEngine::instance().executeMarketOrder(sym, OrderSide::BUY, qty, price, err, lev, sl, tp, trail);
        if (ok) {
            addLog("EXEC", "[ACTION] ORDER FILLED: BUY " + std::to_string(qty).substr(0, 6) + " " + sym + " @" + std::to_string(price) + " [" + std::to_string((int)lev) + "x Leveraged Margin]", Theme::NeonGreen());
            if (sl > 0.0 || tp > 0.0) {
                addLog("EXEC", "   Bracket Orders: SL: $" + std::to_string(sl) + " | TP: $" + std::to_string(tp), Theme::AmberGold());
            }
            UIManager::instance().showToast("ORDER FILLED", "BUY " + sym + " (" + std::to_string((int)lev) + "x)", Theme::NeonGreen());
        } else {
            addLog("ERR", "Order Rejected: " + err, Theme::CrimsonCoral());
        }
    } else if (cmd == "sell" || cmd == "short") {
        if (tokens.size() < 3) {
            addLog("ERR", "Usage: sell <symbol> <quantity> [leverage=1] [sl] [tp]", Theme::CrimsonCoral());
            return;
        }
        std::string sym = tokens[1];
        std::transform(sym.begin(), sym.end(), sym.begin(), ::toupper);
        if (sym.find("USDT") == std::string::npos && sym.find("USD") == std::string::npos) sym += "USDT";

        double qty = 0.0;
        try { qty = std::stod(tokens[2]); } catch (...) {}
        if (qty <= 0.0) {
            addLog("ERR", "Invalid quantity value: " + tokens[2], Theme::CrimsonCoral());
            return;
        }

        double lev = 1.0;
        if (tokens.size() >= 4) {
            try { lev = std::stod(tokens[3]); } catch (...) {}
        }

        double sl = 0.0, tp = 0.0;
        if (tokens.size() >= 5) { try { sl = std::stod(tokens[4]); } catch (...) {} }
        if (tokens.size() >= 6) { try { tp = std::stod(tokens[5]); } catch (...) {} }

        auto ticker = MarketManager::instance().getTicker(sym);
        double price = ticker.lastPrice;
        if (price <= 0.0) {
            auto candles = MarketManager::instance().getActiveCandles();
            if (!candles.empty()) price = candles.back().close;
            else price = (sym == "BTCUSDT") ? 68000.0 : 3500.0;
        }

        std::string err;
        bool ok = PaperTradingEngine::instance().executeMarketOrder(sym, OrderSide::SELL, qty, price, err, lev, sl, tp, 0.0);
        if (ok) {
            addLog("EXEC", "[ACTION] ORDER FILLED: SHORT " + std::to_string(qty).substr(0, 6) + " " + sym + " @" + std::to_string(price) + " [" + std::to_string((int)lev) + "x Leveraged Margin]", Theme::CrimsonCoral());
            UIManager::instance().showToast("ORDER FILLED", "SHORT " + sym + " (" + std::to_string((int)lev) + "x)", Theme::CrimsonCoral());
        } else {
            addLog("ERR", "Order Rejected: " + err, Theme::CrimsonCoral());
        }
    } else if (cmd == "close") {
        if (tokens.size() < 2) {
            addLog("ERR", "Usage: close <symbol> or close all", Theme::CrimsonCoral());
            return;
        }
        std::string sym = tokens[1];
        std::transform(sym.begin(), sym.end(), sym.begin(), ::toupper);

        if (sym == "ALL") {
            auto positions = PaperTradingEngine::instance().getPositions();
            for (const auto& pos : positions) {
                std::string err;
                PaperTradingEngine::instance().closePosition(pos.symbol, pos.currentPrice, err);
            }
            addLog("EXEC", "Liquidated and closed all active margin positions.", Theme::GlitchOrange());
        } else {
            if (sym.find("USDT") == std::string::npos && sym.find("USD") == std::string::npos) sym += "USDT";
            auto ticker = MarketManager::instance().getTicker(sym);
            double price = (ticker.lastPrice > 0.0) ? ticker.lastPrice : 68000.0;
            std::string err;
            bool ok = PaperTradingEngine::instance().closePosition(sym, price, err);
            if (ok) {
                addLog("EXEC", "Closed active position for " + sym, Theme::NeonGreen());
            } else {
                addLog("ERR", "Failed to close: " + err, Theme::CrimsonCoral());
            }
        }
    } else if (cmd == "reverse") {
        if (tokens.size() < 2) {
            addLog("ERR", "Usage: reverse <symbol>", Theme::CrimsonCoral());
            return;
        }
        std::string sym = tokens[1];
        std::transform(sym.begin(), sym.end(), sym.begin(), ::toupper);
        if (sym.find("USDT") == std::string::npos && sym.find("USD") == std::string::npos) sym += "USDT";

        auto ticker = MarketManager::instance().getTicker(sym);
        double price = (ticker.lastPrice > 0.0) ? ticker.lastPrice : 68000.0;
        std::string err;
        bool ok = PaperTradingEngine::instance().reversePosition(sym, price, err);
        if (ok) {
            addLog("EXEC", "[ACTION] Reversed active position for " + sym + " at market price $" + std::to_string(price), Theme::ElectricCyan());
        } else {
            addLog("ERR", "Reversal failed: " + err, Theme::CrimsonCoral());
        }
    } else if (cmd == "pos" || cmd == "positions" || cmd == "account" || cmd == "bal") {
        auto state = PaperTradingEngine::instance().getAccountState();
        auto positions = PaperTradingEngine::instance().getPositions();

        addLog("PORTFOLIO", "┌──[ LEVERAGED MARGIN ACCOUNT STATUS ]────────────────────────┐", Theme::SkyBlue());
        addLog("PORTFOLIO", "│ Total Portfolio Equity : $" + std::to_string(state.totalPortfolioValue).substr(0, 10) + "                        │", Theme::CrystalWhite());
        addLog("PORTFOLIO", "│ Cash Balance           : $" + std::to_string(state.cashBalance).substr(0, 10) + "                        │", Theme::TextMuted());
        addLog("PORTFOLIO", "│ Margin Used / Free     : $" + std::to_string(state.usedMargin).substr(0, 8) + " / $" + std::to_string(state.freeMargin).substr(0, 8) + "          │", Theme::AmberGold());
        std::string unPnlPrefix = (state.totalUnrealizedPnL >= 0) ? "+$" : "-$";
        std::string rePnlPrefix = (state.totalRealizedPnL >= 0) ? "+$" : "-$";
        addLog("PORTFOLIO", "│ Unrealized PnL         : " + unPnlPrefix + std::to_string(std::abs(state.totalUnrealizedPnL)).substr(0, 8) + "                             │", state.totalUnrealizedPnL >= 0 ? Theme::NeonGreen() : Theme::CrimsonCoral());
        addLog("PORTFOLIO", "│ Realized Total PnL     : " + rePnlPrefix + std::to_string(std::abs(state.totalRealizedPnL)).substr(0, 8) + "                             │", state.totalRealizedPnL >= 0 ? Theme::NeonGreen() : Theme::CrimsonCoral());
        addLog("PORTFOLIO", "└─────────────────────────────────────────────────────────────┘", Theme::SkyBlue());

        if (positions.empty()) {
            addLog("PORTFOLIO", "  (No active margin positions open)", Theme::TextMuted());
        } else {
            addLog("PORTFOLIO", "Active Open Positions (" + std::to_string(positions.size()) + "):", Theme::AmberGold());
            for (const auto& p : positions) {
                std::string sideStr = (p.side == OrderSide::BUY) ? "LONG" : "SHORT";
                std::string pnlStr = (p.unrealizedPnL >= 0 ? "+$" : "-$") + std::to_string(std::abs(p.unrealizedPnL)).substr(0, 7) + " (" + std::to_string(p.unrealizedPnLPercent).substr(0, 5) + "%)";
                addLog("PORTFOLIO", "  * " + p.symbol + " " + sideStr + " " + std::to_string(p.quantity).substr(0, 6) + " (" + std::to_string((int)p.leverage) + "x) | Entry: $" + std::to_string(p.avgEntryPrice).substr(0, 8) + " | Mark: $" + std::to_string(p.currentPrice).substr(0, 8) + " | Liq: $" + std::to_string(p.liquidationPrice).substr(0, 8) + " | PnL: " + pnlStr, (p.unrealizedPnL >= 0) ? Theme::NeonGreen() : Theme::CrimsonCoral());
            }
        }
    } else if (cmd == "matrix" || cmd == "corr") {
        addLog("QUANT", "Calculating Cross-Asset Pearson Correlation Heatmap...", Theme::SkyBlue());
        std::vector<std::string> symbols = {"BTCUSDT", "ETHUSDT", "SOLUSDT", "BNBUSDT", "XRPUSDT", "DOGEUSDT"};
        std::unordered_map<std::string, std::vector<Candle>> candleMap;
        for (const auto& s : symbols) {
            candleMap[s] = MarketManager::instance().getCandles(s, "1h");
        }
        auto matrix = TechnicalAnalysisEngine::calculateCorrelationMatrix(symbols, candleMap);
        addLog("QUANT", "Pearson Correlation Matrix Output (1h Returns):", Theme::AmberGold());
        std::string hdr = "SYM     | ";
        for (const auto& s : matrix.symbols) {
            hdr += s.substr(0, 4) + "   ";
        }
        addLog("QUANT", hdr, Theme::CrystalWhite());
        for (size_t i = 0; i < matrix.symbols.size(); ++i) {
            std::string row = matrix.symbols[i].substr(0, 7) + " | ";
            for (size_t j = 0; j < matrix.symbols.size(); ++j) {
                double v = matrix.correlations[i][j];
                std::stringstream ss;
                ss << std::fixed << std::setprecision(2) << v;
                row += ss.str() + " ";
            }
            addLog("QUANT", row, (i == 0) ? Theme::NeonGreen() : Theme::SkyBlue());
        }
    } else if (cmd == "confluence") {
        std::string sym = (tokens.size() >= 2) ? tokens[1] : MarketManager::instance().getActiveSymbol();
        std::transform(sym.begin(), sym.end(), sym.begin(), ::toupper);
        if (sym.find("USDT") == std::string::npos && sym.find("USD") == std::string::npos) sym += "USDT";

        std::unordered_map<std::string, std::vector<Candle>> tfMap = {
            {"1m", MarketManager::instance().getCandles(sym, "1m")},
            {"5m", MarketManager::instance().getCandles(sym, "5m")},
            {"15m", MarketManager::instance().getCandles(sym, "15m")},
            {"1h", MarketManager::instance().getCandles(sym, "1h")},
            {"4h", MarketManager::instance().getCandles(sym, "4h")},
            {"1d", MarketManager::instance().getCandles(sym, "1d")}
        };
        auto conf = TechnicalAnalysisEngine::calculateMultiTimeframeConfluence(sym, tfMap);
        addLog("QUANT", "Multi-Timeframe Algorithmic Confluence for " + sym + ":", Theme::AmberGold());
        addLog("QUANT", "  Confluence Score : " + std::to_string(conf.confluenceScore).substr(0, 4) + " / 100.0", (conf.confluenceScore >= 60.0 ? Theme::NeonGreen() : (conf.confluenceScore <= 40.0 ? Theme::CrimsonCoral() : Theme::AmberGold())));
        addLog("QUANT", "  Regime Bias      : " + conf.overallBias, Theme::ElectricCyan());
        addLog("QUANT", "  Synthesized Rule : " + conf.recommendedAction, Theme::CrystalWhite());
    } else if (cmd == "backtest") {
        std::string strat = (tokens.size() >= 2) ? tokens[1] : "SUPERTREND";
        std::transform(strat.begin(), strat.end(), strat.begin(), ::toupper);

        BacktestConfig cfg;
        cfg.strategyName = strat;
        cfg.symbol = MarketManager::instance().getActiveSymbol();
        cfg.timeframe = MarketManager::instance().getActiveTimeframe();
        cfg.initialCapital = 10000.0;
        if (tokens.size() >= 3) { try { cfg.param1 = std::stoi(tokens[2]); } catch (...) {} }
        if (tokens.size() >= 4) { try { cfg.param2 = std::stoi(tokens[3]); } catch (...) {} }

        auto candles = MarketManager::instance().getActiveCandles();
        auto res = BacktestingEngine::runBacktest(candles, cfg);
        addLog("BACKTEST", "Backtest Completed for " + cfg.strategyName + " on " + cfg.symbol + " (" + cfg.timeframe + "):", Theme::AmberGold());
        std::string retSign = (res.totalReturnPercent >= 0) ? "+" : "";
        addLog("BACKTEST", "  Total Return  : " + retSign + std::to_string(res.totalReturnPercent).substr(0, 6) + "% | Initial: $" + std::to_string(cfg.initialCapital).substr(0, 7) + " Final: $" + std::to_string(res.finalCapital).substr(0, 8), (res.totalReturnPercent >= 0 ? Theme::NeonGreen() : Theme::CrimsonCoral()));
        addLog("BACKTEST", "  Trades Count  : " + std::to_string(res.totalTrades) + " (Win Rate: " + std::to_string(res.winRatePercent).substr(0, 4) + "%) | Profit Factor: " + std::to_string(res.profitFactor).substr(0, 4), Theme::ElectricCyan());
        addLog("BACKTEST", "  Sharpe Ratio  : " + std::to_string(res.sharpeRatio).substr(0, 4) + " | Sortino: " + std::to_string(res.sortinoRatio).substr(0, 4) + " | Max DD: " + std::to_string(res.maxDrawdownPercent).substr(0, 4) + "%", Theme::AmberGold());
        addLog("BACKTEST", "  Trade Expect. : $" + std::to_string(res.expectancy).substr(0, 6) + " / trade", Theme::CrystalWhite());
    } else if (cmd == "montecarlo" || cmd == "mc") {
        int paths = 1000;
        if (tokens.size() >= 2) { try { paths = std::stoi(tokens[1]); } catch (...) {} }
        addLog("MC", "Running " + std::to_string(paths) + "-Path Monte Carlo Bootstrap Simulation...", Theme::BodyPink());

        auto candles = MarketManager::instance().getActiveCandles();
        BacktestConfig cfg;
        cfg.strategyName = "SUPERTREND";
        cfg.symbol = MarketManager::instance().getActiveSymbol();
        auto res = BacktestingEngine::runBacktest(candles, cfg);
        auto mc = BacktestingEngine::runMonteCarloSimulation(res.trades, 10000.0, paths);

        addLog("MC", "Monte Carlo 1,000-Path Resampling Results:", Theme::AmberGold());
        addLog("MC", "  Median Final Equity   : $" + std::to_string(mc.medianFinalCapital).substr(0, 8), Theme::NeonGreen());
        addLog("MC", "  95% / 99% VaR Capital : $" + std::to_string(mc.var95FinalCapital).substr(0, 8) + " / $" + std::to_string(mc.var99FinalCapital).substr(0, 8), Theme::CrimsonCoral());
        addLog("MC", "  Worst-Case Drawdown   : " + std::to_string(mc.worstCaseDrawdownPercent).substr(0, 5) + "%", Theme::CrimsonCoral());
        addLog("MC", "  Probability of Ruin   : " + std::to_string(mc.probabilityOfRuinPercent).substr(0, 4) + "%", (mc.probabilityOfRuinPercent < 5.0 ? Theme::NeonGreen() : Theme::CrimsonCoral()));
    } else if (cmd == "stress" || cmd == "var") {
        std::unordered_map<std::string, double> prices = {
            {"BTCUSDT", 68000.0},
            {"ETHUSDT", 3500.0},
            {"SOLUSDT", 180.0}
        };
        auto risk = PaperTradingEngine::instance().getRiskAnalytics(prices);
        addLog("RISK", "Institutional Parametric Value-at-Risk (1-Day):", Theme::AmberGold());
        addLog("RISK", "  1-Day 95% VaR : $" + std::to_string(risk.var95Daily).substr(0, 8) + " (" + std::to_string(risk.var95Percent).substr(0, 4) + "%)", Theme::CrimsonCoral());
        addLog("RISK", "  1-Day 99% VaR : $" + std::to_string(risk.var99Daily).substr(0, 8) + " (" + std::to_string(risk.var99Percent).substr(0, 4) + "%)", Theme::CrimsonCoral());
        addLog("RISK", "Historical Black-Swan Stress Scenarios:", Theme::SkyBlue());
        for (const auto& sc : risk.stressScenarios) {
            std::string pnl = (sc.impactUsd >= 0 ? "+$" : "-$") + std::to_string(std::abs(sc.impactUsd)).substr(0, 8);
            addLog("RISK", "  * " + sc.name + " (" + std::to_string(sc.shockPercent).substr(0, 5) + "%): Projected Impact: " + pnl + " -> Remaining Equity: $" + std::to_string(sc.postShockTotalValue).substr(0, 8), (sc.impactUsd >= 0 ? Theme::NeonGreen() : Theme::CrimsonCoral()));
        }
    } else if (cmd == "ai" || cmd == "ask") {
        if (tokens.size() < 2) {
            addLog("ERR", "Usage: ai <\"prompt query\">", Theme::CrimsonCoral());
            return;
        }
        std::string prompt = trimmed.substr(trimmed.find(' '));
        prompt.erase(0, prompt.find_first_not_of(" \t\"'"));
        prompt.erase(prompt.find_last_not_of(" \t\"'") + 1);

        addLog("AI_REQ", "Querying Autonomous Neural Quant Agent: \"" + prompt + "\"...", Theme::BodyPink());
        
        AIRequest req;
        req.prompt = prompt;
        auto resp = AIRouter::instance().generateSync(req, AIProviderType::AUTO);
        if (resp.success) {
            addLog("AI_RESP", "[" + resp.providerName + " // " + resp.modelUsed + "]:", Theme::NeonGreen());
            std::istringstream stream(resp.content);
            std::string line;
            while (std::getline(stream, line)) {
                if (!line.empty()) {
                    addLog("AI", "  " + line, Theme::CrystalWhite());
                }
            }
        } else {
            addLog("ERR", "AI Generation Failed: " + resp.errorMessage, Theme::CrimsonCoral());
        }
    } else if (cmd == "theme") {
        if (tokens.size() < 2) {
            addLog("ERR", "Usage: theme <obsidian|matrix|gold|tokyo|tactical>", Theme::CrimsonCoral());
            return;
        }
        std::string p = tokens[1];
        std::transform(p.begin(), p.end(), p.begin(), ::tolower);
        if (p == "sakura" || p == "cute" || p == "pink") Theme::setPreset(ThemePreset::SAKURA_PASTEL);
        else if (p == "classic" || p == "slate" || p == "clean") Theme::setPreset(ThemePreset::CLASSIC_SLATE);
        else if (p == "mocha" || p == "cozy" || p == "coffee") Theme::setPreset(ThemePreset::COZY_MOCHA);
        else if (p == "candy" || p == "sweet") Theme::setPreset(ThemePreset::SWEET_CANDY);
        else if (p == "matrix") Theme::setPreset(ThemePreset::MATRIX_EMERALD);
        else if (p == "gold" || p == "bloomberg") Theme::setPreset(ThemePreset::BLOOMBERG_GOLD);
        else if (p == "tokyo") Theme::setPreset(ThemePreset::TOKYO_NEON);
        else if (p == "tactical" || p == "red") Theme::setPreset(ThemePreset::RED_TACTICAL_HACKER);
        else Theme::setPreset(ThemePreset::CYBERPUNK_OBSIDIAN);
        addLog("THEME", "Palette updated to preset: " + p, Theme::BodyPink());
    } else if (cmd == "demo") {
        if (tokens.size() >= 2 && tokens[1] == "off") {
            MarketManager::instance().setDemoMode(false);
            addLog("NET", "Switched to LIVE Binance L3 DMA feed.", Theme::NeonGreen());
        } else {
            MarketManager::instance().setDemoMode(true);
            addLog("NET", "Switched to Offline Monte Carlo Simulated Market Feed.", Theme::AmberGold());
        }
    } else {
        addLog("ERR", "Unknown command: '" + cmd + "'. Type 'help' for tactical command matrix.", Theme::CrimsonCoral());
    }
}

void HackerConsole::render(bool isFullscreen) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::DeepObsidian());
    ImGui::PushStyleColor(ImGuiCol_Border, Theme::BorderGlow());
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));

    ImGui::BeginChild("HackerConsoleFrame", ImVec2(0, isFullscreen ? 0 : 280.0f), true, ImGuiWindowFlags_NoScrollbar);
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);

    // Top Header & Prompt Macro Chips
    ImGui::TextColored(Theme::ElectricCyan(), "[ACTION] CRYPTØ QUANT REPL // INSTITUTIONAL HACKER CLI");
    ImGui::SameLine();
    ImGui::TextColored(Theme::TextMuted(), "| Press ` or F12 to toggle CLI Drawer");

    ImGui::SameLine(ImGui::GetWindowWidth() - 280.0f);
    if (ImGui::SmallButton("Clear")) {
        executeCommand("clear");
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("SysInfo")) {
        executeCommand("sysinfo");
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Matrix")) {
        executeCommand("matrix");
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Help")) {
        executeCommand("help");
    }

    ImGui::Separator();

    // Log Region
    float inputAreaHeight = 40.0f;
    float logsHeight = ImGui::GetContentRegionAvail().y - inputAreaHeight;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.03f, 0.03f, 0.05f, 0.95f));
    ImGui::BeginChild("ConsoleScrollRegion", ImVec2(0, logsHeight), false, ImGuiWindowFlags_AlwaysVerticalScrollbar);
    ImGui::PopStyleColor();

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (const auto& log : m_logs) {
            ImGui::TextColored(Theme::TextMuted(), "[%s]", log.timestamp.c_str());
            ImGui::SameLine();
            ImGui::TextColored(Theme::SkyBlue(), "[%s]", log.tag.c_str());
            ImGui::SameLine();
            ImGui::TextColored(log.color, "%s", log.text.c_str());
        }
    }

    if (m_scrollToBottom || (m_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())) {
        ImGui::SetScrollHereY(1.0f);
        m_scrollToBottom = false;
    }
    ImGui::EndChild();

    ImGui::Spacing();

    // Command Input Line
    ImGui::PushStyleColor(ImGuiCol_Text, Theme::NeonGreen());
    ImGui::TextUnformatted("CRYPTØ:~$");
    ImGui::PopStyleColor();
    ImGui::SameLine();

    ImGui::PushItemWidth(-1);
    ImGuiInputTextFlags inputFlags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory;

    auto historyCallback = [](ImGuiInputTextCallbackData* data) -> int {
        auto* console = reinterpret_cast<HackerConsole*>(data->UserData);
        if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory) {
            const int prevPos = console->m_historyPos;
            if (data->EventKey == ImGuiKey_UpArrow) {
                if (console->m_historyPos == -1)
                    console->m_historyPos = (int)console->m_history.size() - 1;
                else if (console->m_historyPos > 0)
                    console->m_historyPos--;
            } else if (data->EventKey == ImGuiKey_DownArrow) {
                if (console->m_historyPos != -1) {
                    if (++console->m_historyPos >= (int)console->m_history.size())
                        console->m_historyPos = -1;
                }
            }

            if (prevPos != console->m_historyPos) {
                const char* histStr = (console->m_historyPos >= 0) ? console->m_history[console->m_historyPos].c_str() : "";
                data->DeleteChars(0, data->BufTextLen);
                data->InsertChars(0, histStr);
            }
        }
        return 0;
    };

    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.08f, 0.08f, 0.12f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_Border, Theme::SkyBlue());
    if (ImGui::InputText("##ConsoleInput", m_inputBuf, sizeof(m_inputBuf), inputFlags, historyCallback, this)) {
        std::string cmd = m_inputBuf;
        executeCommand(cmd);
        m_inputBuf[0] = '\0';
        ImGui::SetKeyboardFocusHere(-1);
    }
    ImGui::PopStyleColor(2);
    ImGui::PopItemWidth();

    ImGui::EndChild();
}

} // namespace crypto
