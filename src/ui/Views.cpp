#include "ui/Views.hpp"
#include "ui/Theme.hpp"
#include "ui/UIManager.hpp"
#include "ui/HackerConsole.hpp"
#include "market/MarketManager.hpp"
#include "analysis/TechnicalAnalysisEngine.hpp"
#include "analysis/MarketScreener.hpp"
#include "analysis/BacktestingEngine.hpp"
#include "analysis/AlertManager.hpp"
#include "portfolio/PortfolioManager.hpp"
#include "trading/PaperTradingEngine.hpp"
#include "ai/AIRouter.hpp"
#include "ai/AIContextBuilder.hpp"
#include "ai/AICommandEngine.hpp"
#include "news/NewsManager.hpp"
#include "storage/Database.hpp"
#include "core/Config.hpp"
#include "core/SystemMetrics.hpp"
#include "core/Logger.hpp"
#include <imgui.h>
#include <implot.h>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace crypto {

// Helper to format currency
static std::string formatMoney(double val) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << val;
    return "$" + ss.str();
}

// -------------------------------------------------------------
// 1. DASHBOARD VIEW
// -------------------------------------------------------------
// State for Chainblock interactive widgets
static bool s_showCandleMode = false;
static int s_analyticsTfIdx = 1; // 0: 1D, 1: 7D, 2: 1M, 3: 3M, 4: ALL
static int s_operationMode = 0;   // 0: Buy, 1: Sell, 2: Exchange
static char s_payAmountBuf[32] = "1321.21";
static int s_payCurrencyIdx = 0;   // 0: USD, 1: USDT, 2: EUR
static int s_getCurrencyIdx = 1;   // 0: BTC, 1: ETH, 2: SOL, 3: ADA
static bool s_hideBalance = false;

void DashboardView::render() {
    float availW = ImGui::GetContentRegionAvail().x;
    float colSpacing = 16.0f;
    float rightColW = std::max(340.0f, std::min(420.0f, availW * 0.36f));
    float leftColW = availW - rightColW - colSpacing;

    // =========================================================
    // LEFT COLUMN: Total Assets + 3 Mini Cards + Analytics Chart
    // =========================================================
    ImGui::BeginChild("LeftMainChainblockCol", ImVec2(leftColW, 0), false, ImGuiWindowFlags_NoScrollbar);
    {
        // 1. Total Assets Header Card
        ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::CardSurface());
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 14.0f);
        ImGui::BeginChild("TotalAssetsCard", ImVec2(0, 92.0f), true);

        ImGui::SetCursorPos(ImVec2(18, 12));
        ImGui::TextColored(Theme::TextMuted(), "Total assets value in $");
        
        ImGui::SetCursorPos(ImVec2(18, 32));
        auto pSummary = PortfolioManager::instance().getSummary();
        double totalVal = pSummary.totalValueUsd > 0.0 ? pSummary.totalValueUsd : 12312.23;
        double btcPrice = MarketManager::instance().getTicker("BTCUSDT").lastPrice;
        if (btcPrice <= 0.0) btcPrice = 64250.0;
        double btcEquiv = totalVal / btcPrice;

        ImGui::PushStyleColor(ImGuiCol_Text, Theme::CrystalWhite());
        ImGui::SetWindowFontScale(1.45f);
        if (s_hideBalance) {
            ImGui::TextUnformatted("$ ********");
        } else {
            ImGui::Text("$%.2f", totalVal);
        }
        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::SetCursorPosY(36.0f);
        if (ImGui::Button(s_hideBalance ? "[EYE]##show" : "[EYE]##hide")) {
            s_hideBalance = !s_hideBalance;
        }

        ImGui::SetCursorPos(ImVec2(18, 66));
        ImGui::TextColored(Theme::TextMuted(), "- %.4f BTC", btcEquiv);

        // (+) Add Funds Button
        ImGui::SetCursorPos(ImVec2(leftColW - 60.0f, 26.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(Theme::EmeraldGreen().x * 0.20f, Theme::EmeraldGreen().y * 0.20f, Theme::EmeraldGreen().z * 0.20f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::EmeraldGreen());
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 20.0f);
        if (ImGui::Button(" + ", ImVec2(40, 40))) {
            PaperTradingEngine::instance().reset(PaperTradingEngine::instance().getAvailableBalance() + 5000.0);
            UIManager::instance().showToast("FUNDS ADDED", "+$5,000.00 USD added to virtual wallet ", Theme::EmeraldGreen());
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);

        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

        ImGui::Spacing();

        // 2. 3 Mini Sparkline Cards (BTC, ETH, USDT)
        float miniCardW = (leftColW - (2.0f * 12.0f)) / 3.0f;
        const char* topCoins[] = {"BTCUSDT", "ETHUSDT", "SOLUSDT"};
        const char* coinSymbols[] = {"BTC", "ETH", "SOL"};
        const char* coinNames[] = {"BTC/USD", "ETH/USD", "SOL/USD"};

        for (int i = 0; i < 3; ++i) {
            if (i > 0) ImGui::SameLine(0, 12.0f);

            std::string sym = topCoins[i];
            Ticker t = MarketManager::instance().getTicker(sym);
            if (t.lastPrice <= 0.0) {
                if (i == 0) { t.lastPrice = 64250.0; t.priceChangePercent24h = 0.21; }
                else if (i == 1) { t.lastPrice = 3480.0; t.priceChangePercent24h = 1.23; }
                else { t.lastPrice = 145.20; t.priceChangePercent24h = 4.15; }
            }

            ImGui::PushID(i);
            ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::CardSurface());
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 14.0f);
            ImGui::BeginChild("MiniSparkCard", ImVec2(miniCardW, 110.0f), true);

            ImGui::SetCursorPos(ImVec2(14, 10));
            double holdingAmt = (i == 0) ? 0.2342 : (i == 1 ? 1.4820 : 15.240);
            double holdingUsd = holdingAmt * t.lastPrice;
            ImGui::TextColored(Theme::CrystalWhite(), "%.4f %s", holdingAmt, coinSymbols[i]);

            ImGui::SetCursorPos(ImVec2(14, 28));
            ImGui::TextColored(Theme::TextMuted(), "$%.0f", holdingUsd);

            // Fetch live candles for sparkline if available
            auto coinCandles = MarketManager::instance().getCandles(sym, "1h");
            float sparkValues[12];
            int sparkCount = 12;
            if (coinCandles.size() >= 12) {
                for (int k = 0; k < 12; ++k) {
                    sparkValues[k] = (float)coinCandles[coinCandles.size() - 12 + k].close;
                }
            } else {
                for (int k = 0; k < 12; ++k) {
                    float factor = 1.0f + ((float)(k - 6) * 0.005f * (t.priceChangePercent24h >= 0 ? 1.0f : -1.0f));
                    sparkValues[k] = (float)(t.lastPrice * factor);
                }
            }

            ImGui::SetCursorPos(ImVec2(14, 46));
            ImVec4 sparkCol = (t.priceChangePercent24h >= 0) ? Theme::EmeraldGreen() : Theme::CrimsonCoral();
            ImGui::PushStyleColor(ImGuiCol_PlotLines, sparkCol);
            ImGui::PlotLines("##spark", sparkValues, sparkCount, 0, nullptr, 3.4e38f, 3.4e38f, ImVec2(miniCardW - 28.0f, 26.0f));
            ImGui::PopStyleColor();

            // Bottom icon and % change
            ImGui::SetCursorPos(ImVec2(14, 80));
            ImVec4 dotCol = (i == 0) ? Theme::AmberGold() : (i == 1 ? Theme::SkyBlue() : Theme::EmeraldGreen());
            ImGui::TextColored(dotCol, "●");
            ImGui::SameLine();
            ImGui::TextColored(Theme::CrystalWhite(), "%s", coinNames[i]);

            ImGui::SameLine(miniCardW - 70.0f);
            ImVec4 chgCol = (t.priceChangePercent24h >= 0) ? Theme::EmeraldGreen() : Theme::CrimsonCoral();
            ImGui::TextColored(chgCol, "%s%.2f%%", t.priceChangePercent24h >= 0 ? "+" : "", t.priceChangePercent24h);

            if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0)) {
                MarketManager::instance().setActiveSymbol(sym);
            }

            ImGui::EndChild();
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
            ImGui::PopID();
        }

        ImGui::Spacing();

        // 3. Analytics Chart Card (Area Line / Candlestick + Volume Histogram)
        ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::CardSurface());
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 14.0f);
        float analyticsH = ImGui::GetContentRegionAvail().y - 4.0f;
        if (analyticsH < 300.0f) analyticsH = 300.0f;
        ImGui::BeginChild("AnalyticsCard", ImVec2(0, analyticsH), true);

        // Header: Active Symbol + Mode Switcher + Indicators + Timeframes
        std::string activeSym = MarketManager::instance().getActiveSymbol();
        Ticker activeT = MarketManager::instance().getTicker(activeSym);

        ImGui::SetCursorPos(ImVec2(16, 12));
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::CrystalWhite());
        ImGui::SetWindowFontScale(1.12f);
        ImGui::Text("Analytics — %s ($%.2f)", activeSym.c_str(), activeT.lastPrice > 0 ? activeT.lastPrice : 64250.0);
        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopStyleColor();

        // Price / Candle toggle pills
        ImGui::SameLine(std::max(220.0f, leftColW - 460.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
        if (!s_showCandleMode) {
            ImGui::PushStyleColor(ImGuiCol_Button, Theme::EmeraldGreen());
            ImGui::PushStyleColor(ImGuiCol_Text, Theme::DeepObsidian());
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(Theme::DarkSurface().x, Theme::DarkSurface().y, Theme::DarkSurface().z, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_Text, Theme::TextMuted());
        }
        if (ImGui::Button("Price", ImVec2(52, 24))) s_showCandleMode = false;
        ImGui::PopStyleColor(2);

        ImGui::SameLine();
        if (s_showCandleMode) {
            ImGui::PushStyleColor(ImGuiCol_Button, Theme::EmeraldGreen());
            ImGui::PushStyleColor(ImGuiCol_Text, Theme::DeepObsidian());
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(Theme::DarkSurface().x, Theme::DarkSurface().y, Theme::DarkSurface().z, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_Text, Theme::TextMuted());
        }
        if (ImGui::Button("Candle", ImVec2(58, 24))) s_showCandleMode = true;
        ImGui::PopStyleColor(2);

        // Indicator quick toggles
        static bool s_dashSMA = false;
        static bool s_dashBB = false;
        static bool s_dashVWAP = false;

        ImGui::SameLine();
        ImGui::Checkbox("SMA", &s_dashSMA);
        ImGui::SameLine();
        ImGui::Checkbox("BB", &s_dashBB);
        ImGui::SameLine();
        ImGui::Checkbox("VWAP", &s_dashVWAP);

        // Timeframe pills on the right (1D, 7D, 1M, 3M, ALL)
        ImGui::SameLine(leftColW - 270.0f);
        const char* tfLabels[] = {"1D", "7D", "1M", "3M", "ALL"};
        const char* tfIntervals[] = {"1h", "4h", "1D", "1D", "1W"};
        for (int k = 0; k < 5; ++k) {
            bool isTf = (s_analyticsTfIdx == k);
            if (isTf) {
                ImGui::PushStyleColor(ImGuiCol_Button, Theme::EmeraldGreen());
                ImGui::PushStyleColor(ImGuiCol_Text, Theme::DeepObsidian());
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
                ImGui::PushStyleColor(ImGuiCol_Text, Theme::TextMuted());
            }
            if (ImGui::Button(tfLabels[k], ImVec2(28, 24))) {
                s_analyticsTfIdx = k;
                MarketManager::instance().setActiveTimeframe(tfIntervals[k]);
            }
            ImGui::PopStyleColor(2);
            ImGui::SameLine();
        }

        // Pro Trading Chart view launcher button
        if (ImGui::Button("PRO >", ImVec2(48, 24))) {
            UIManager::instance().setActiveView(ViewType::CHART);
        }

        ImGui::PopStyleVar();

        ImGui::Spacing();
        ImGui::Separator();

        // Active Chart Drawing with ImPlot
        auto candles = MarketManager::instance().getActiveCandles();
        if (candles.empty()) {
            candles = MarketManager::instance().getCandles(activeSym, "1h");
        }
        if (candles.empty()) {
            candles = MarketManager::instance().getCandles("BTCUSDT", "1h");
        }

        if (!candles.empty() && ImPlot::BeginPlot("##AnalyticsPlot", ImVec2(-1, analyticsH - 70.0f), ImPlotFlags_NoTitle | ImPlotFlags_Crosshairs)) {
            size_t N = candles.size();
            std::vector<double> xs(N), closes(N), volumes(N), opens(N), highs(N), lows(N);
            for (size_t idx = 0; idx < N; ++idx) {
                xs[idx] = (double)idx;
                opens[idx] = candles[idx].open;
                highs[idx] = candles[idx].high;
                lows[idx] = candles[idx].low;
                closes[idx] = candles[idx].close;
                volumes[idx] = candles[idx].volume;
            }

            ImPlot::SetupAxes("Time", "Price (USD)", ImPlotAxisFlags_None, ImPlotAxisFlags_Opposite);
            ImPlot::SetupAxisLimits(ImAxis_X1, 0, (double)N, ImPlotCond_Always);

            if (!s_showCandleMode) {
                // Smooth Area Line Plot
                ImPlotSpec areaSpec;
                areaSpec.LineColor = Theme::EmeraldGreen();
                areaSpec.LineWeight = 2.4f;
                ImPlot::PlotLine("Price", xs.data(), closes.data(), (int)N, areaSpec);

                // Real-time Highlight Focus Node Marker on Last Tick
                if (N > 0) {
                    size_t pinIdx = N - 1;
                    double pinX[1] = {xs[pinIdx]};
                    double pinY[1] = {closes[pinIdx]};
                    ImPlotSpec pinSpec;
                    pinSpec.MarkerLineColor = Theme::EmeraldGreen();
                    pinSpec.MarkerFillColor = Theme::CrystalWhite();
                    pinSpec.MarkerSize = 8.0f;
                    ImPlot::PlotScatter("Live Tick", pinX, pinY, 1, pinSpec);
                }
            } else {
                // Candlestick Rendering
                for (size_t idx = 0; idx < N; ++idx) {
                    bool bullish = closes[idx] >= opens[idx];
                    ImVec4 col = bullish ? Theme::EmeraldGreen() : Theme::CrimsonCoral();

                    double wickX[2] = {xs[idx], xs[idx]};
                    double wickY[2] = {lows[idx], highs[idx]};
                    ImPlotSpec wickSpec;
                    wickSpec.LineColor = col;
                    wickSpec.LineWeight = 1.2f;
                    ImPlot::PlotLine("##w", wickX, wickY, 2, wickSpec);

                    double bodyX[2] = {xs[idx], xs[idx]};
                    double bodyY[2] = {std::min(opens[idx], closes[idx]), std::max(opens[idx], closes[idx])};
                    ImPlotSpec bodySpec;
                    bodySpec.LineColor = col;
                    bodySpec.LineWeight = 5.0f;
                    ImPlot::PlotLine("##b", bodyX, bodyY, 2, bodySpec);
                }
            }

            // Optional Real-time Indicators Overlaid
            if (s_dashSMA) {
                auto sma20 = TechnicalAnalysisEngine::calculateSMA(candles, 20);
                std::vector<double> smaX, smaY;
                for (size_t i = 0; i < N && i < sma20.values.size(); ++i) {
                    if (sma20.values[i] > 0) { smaX.push_back(xs[i]); smaY.push_back(sma20.values[i]); }
                }
                if (!smaX.empty()) {
                    ImPlotSpec smaSpec;
                    smaSpec.LineColor = Theme::AmberGold();
                    smaSpec.LineWeight = 1.8f;
                    ImPlot::PlotLine("SMA 20", smaX.data(), smaY.data(), (int)smaX.size(), smaSpec);
                }
            }

            if (s_dashBB) {
                auto bb = TechnicalAnalysisEngine::calculateBollingerBands(candles, 20, 2.0);
                std::vector<double> bbX, bbUpper, bbLower;
                for (size_t i = 0; i < N && i < bb.middle.size(); ++i) {
                    if (bb.middle[i] > 0) {
                        bbX.push_back(xs[i]);
                        bbUpper.push_back(bb.upper[i]);
                        bbLower.push_back(bb.lower[i]);
                    }
                }
                if (!bbX.empty()) {
                    ImPlotSpec bbSpec;
                    bbSpec.LineColor = Theme::SkyBlue();
                    bbSpec.LineWeight = 1.4f;
                    ImPlot::PlotLine("BB Upper", bbX.data(), bbUpper.data(), (int)bbX.size(), bbSpec);
                    ImPlot::PlotLine("BB Lower", bbX.data(), bbLower.data(), (int)bbX.size(), bbSpec);
                }
            }

            if (s_dashVWAP) {
                auto vwapRes = TechnicalAnalysisEngine::calculateVWAP(candles, 1.0);
                std::vector<double> vX, vY;
                for (size_t i = 0; i < N && i < vwapRes.vwap.size(); ++i) {
                    if (vwapRes.vwap[i] > 0) {
                        vX.push_back(xs[i]);
                        vY.push_back(vwapRes.vwap[i]);
                    }
                }
                if (!vX.empty()) {
                    ImPlotSpec vSpec;
                    vSpec.LineColor = Theme::ButtonGreen();
                    vSpec.LineWeight = 2.0f;
                    ImPlot::PlotLine("VWAP", vX.data(), vY.data(), (int)vX.size(), vSpec);
                }
            }

            // Volume Bars at bottom
            double maxVol = 1.0;
            for (double v : volumes) if (v > maxVol) maxVol = v;
            double minPrice = 1e9, maxPrice = 0.0;
            for (double p : closes) { if (p < minPrice) minPrice = p; if (p > maxPrice) maxPrice = p; }
            double priceRange = maxPrice - minPrice;

            for (size_t idx = 0; idx < N; ++idx) {
                double normVol = (volumes[idx] / maxVol) * (priceRange * 0.22);
                double barX[2] = {xs[idx], xs[idx]};
                double barY[2] = {minPrice, minPrice + normVol};
                ImPlotSpec volSpec;
                volSpec.LineColor = ImVec4(0.4f, 0.45f, 0.55f, 0.35f);
                volSpec.LineWeight = 3.0f;
                ImPlot::PlotLine("##vol", barX, barY, 2, volSpec);
            }

            ImPlot::EndPlot();
        }

        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
    }
    ImGui::EndChild();

    ImGui::SameLine(0, colSpacing);

    // =========================================================
    // RIGHT COLUMN: Operation / Swap + Recent Transactions
    // =========================================================
    ImGui::BeginChild("RightChainblockCol", ImVec2(rightColW, 0), false, ImGuiWindowFlags_NoScrollbar);
    {
        // 1. OPERATION / EXCHANGE SWAP CARD
        ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::CardSurface());
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 14.0f);
        ImGui::BeginChild("OperationCard", ImVec2(0, 310.0f), true);

        // Header: Operation Title + Mode Pills
        ImGui::SetCursorPos(ImVec2(16, 14));
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::CrystalWhite());
        ImGui::SetWindowFontScale(1.15f);
        ImGui::TextUnformatted("Operation");
        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopStyleColor();

        ImGui::SameLine(rightColW - 175.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
        const char* modes[] = {"Buy", "Sell", "Exchange"};
        for (int m = 0; m < 3; ++m) {
            bool isM = (s_operationMode == m);
            if (isM) {
                ImGui::PushStyleColor(ImGuiCol_Button, Theme::EmeraldGreen());
                ImGui::PushStyleColor(ImGuiCol_Text, Theme::DeepObsidian());
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.15f, 0.20f, 0.6f));
                ImGui::PushStyleColor(ImGuiCol_Text, Theme::TextMuted());
            }
            if (ImGui::Button(modes[m], ImVec2(50, 24))) s_operationMode = m;
            ImGui::PopStyleColor(2);
            ImGui::SameLine();
        }
        ImGui::PopStyleVar();

        ImGui::Spacing();
        ImGui::Spacing();

        // "You pay" input box
        ImGui::SetCursorPos(ImVec2(16, 52));
        ImGui::TextColored(Theme::TextMuted(), "You pay");

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(Theme::DarkSurface().x, Theme::DarkSurface().y, Theme::DarkSurface().z, 0.85f));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0f);
        ImGui::SetCursorPos(ImVec2(16, 72));
        ImGui::BeginChild("PayBox", ImVec2(rightColW - 32.0f, 44.0f), true);

        ImGui::SetCursorPos(ImVec2(8, 8));
        ImGui::SetNextItemWidth(75.0f);
        const char* payCurrencies[] = {"$ USD", "₮ USDT", "EUR EUR"};
        ImGui::Combo("##PayCurr", &s_payCurrencyIdx, payCurrencies, 3);

        ImGui::SameLine(100.0f);
        ImGui::SetNextItemWidth(rightColW - 200.0f);
        ImGui::InputText("##PayAmt", s_payAmountBuf, sizeof(s_payAmountBuf));

        ImGui::SameLine(rightColW - 85.0f);
        if (ImGui::SmallButton("MAX")) {
            snprintf(s_payAmountBuf, sizeof(s_payAmountBuf), "%.2f", PaperTradingEngine::instance().getAvailableBalance());
        }
        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

        // Switch Swap Icon ⇅
        ImGui::SetCursorPos(ImVec2((rightColW - 32.0f) * 0.5f, 122.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, Theme::CardSurface());
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::EmeraldGreen());
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 16.0f);
        if (ImGui::Button(" ⇅ ", ImVec2(32, 32))) {
            // Toggle currencies
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);

        // "You get" box
        ImGui::SetCursorPos(ImVec2(16, 158));
        ImGui::TextColored(Theme::TextMuted(), "You get");

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(Theme::DarkSurface().x, Theme::DarkSurface().y, Theme::DarkSurface().z, 0.85f));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0f);
        ImGui::SetCursorPos(ImVec2(16, 178));
        ImGui::BeginChild("GetBox", ImVec2(rightColW - 32.0f, 44.0f), true);

        ImGui::SetCursorPos(ImVec2(8, 8));
        ImGui::SetNextItemWidth(75.0f);
        const char* getCurrencies[] = {"₿ BTC", "Ξ ETH", "SOL SOL", "ADA ADA"};
        ImGui::Combo("##GetCurr", &s_getCurrencyIdx, getCurrencies, 4);

        double payVal = atof(s_payAmountBuf);
        double targetCoinPrice = 3480.0;
        if (s_getCurrencyIdx == 0) targetCoinPrice = MarketManager::instance().getTicker("BTCUSDT").lastPrice;
        else if (s_getCurrencyIdx == 1) targetCoinPrice = MarketManager::instance().getTicker("ETHUSDT").lastPrice;
        else if (s_getCurrencyIdx == 2) targetCoinPrice = MarketManager::instance().getTicker("SOLUSDT").lastPrice;
        else if (s_getCurrencyIdx == 3) targetCoinPrice = MarketManager::instance().getTicker("ADAUSDT").lastPrice;
        if (targetCoinPrice <= 0.0) targetCoinPrice = 3480.0;
        double coinGetAmt = (payVal > 0.0) ? (payVal / targetCoinPrice) : 0.0;

        ImGui::SameLine(100.0f);
        ImGui::SetCursorPosY(10.0f);
        ImGui::TextColored(Theme::CrystalWhite(), "%.4f", coinGetAmt);

        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

        // Subtitle: 1 ETH = $3,480.00
        ImGui::SetCursorPos(ImVec2(16, 228));
        const char* getSymNames[] = {"BTC", "ETH", "SOL", "ADA"};
        ImGui::TextColored(Theme::TextMuted(), "1 %s = $%.2f", getSymNames[s_getCurrencyIdx], targetCoinPrice);

        // Big Mint Green Rounded CTA Button
        ImGui::SetCursorPos(ImVec2(16, 252));
        ImGui::PushStyleColor(ImGuiCol_Button, Theme::EmeraldGreen());
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::DeepObsidian());
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
        char ctaBuf[64];
        snprintf(ctaBuf, sizeof(ctaBuf), "Buy %s (Instant Execution)", getSymNames[s_getCurrencyIdx]);
        if (ImGui::Button(ctaBuf, ImVec2(rightColW - 32.0f, 42.0f))) {
            std::string tradeSym = std::string(getSymNames[s_getCurrencyIdx]) + "USDT";
            std::string err;
            bool ok = PaperTradingEngine::instance().executeMarketOrder(tradeSym, OrderSide::BUY, coinGetAmt, targetCoinPrice, err);
            if (ok) {
                UIManager::instance().showToast("SWAP FILLED", "Successfully bought " + std::to_string(coinGetAmt).substr(0, 6) + " " + tradeSym, Theme::EmeraldGreen());
            } else {
                UIManager::instance().showToast("SWAP FAILED", err, Theme::CrimsonCoral());
            }
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);

        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

        ImGui::Spacing();

        // 2. RECENT TRANSACTIONS CARD
        ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::CardSurface());
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 14.0f);
        float txH = ImGui::GetContentRegionAvail().y - 4.0f;
        if (txH < 220.0f) txH = 220.0f;
        ImGui::BeginChild("TransactionsCard", ImVec2(0, txH), true);

        // Title + See All
        ImGui::SetCursorPos(ImVec2(16, 12));
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::CrystalWhite());
        ImGui::SetWindowFontScale(1.15f);
        ImGui::TextUnformatted("Transactions");
        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopStyleColor();

        ImGui::SameLine(rightColW - 90.0f);
        if (ImGui::SmallButton("See All >")) {
            UIManager::instance().setActiveView(ViewType::PORTFOLIO);
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Transactions List Feed
        struct TxItem {
            const char* name;
            const char* date;
            const char* amt;
            const char* usd;
            bool isBuy;
        };

        std::vector<TxItem> txList = {
            {"Bitcoin", "2 Oct 2026, 09:23 PM", "+0.0962 BTC", "$480.98", true},
            {"Ethereum", "1 Oct 2026, 09:23 PM", "-0.0082 ETH", "$183.43", false},
            {"Cardano", "29 Sept 2026, 12:23 PM", "+1,843 ADA", "$2,000.87", true},
            {"Cardano", "28 Sept 2026, 01:23 PM", "+1,843 ADA", "$2,000.87", true},
            {"Solana", "25 Sept 2026, 04:15 PM", "+15.24 SOL", "$2,324.10", true}
        };

        for (size_t i = 0; i < txList.size(); ++i) {
            const auto& tx = txList[i];
            ImGui::PushID((int)i);

            // Icon Arrow
            ImGui::SetCursorPosX(14.0f);
            if (tx.isBuy) {
                ImGui::TextColored(Theme::EmeraldGreen(), " ▲ ");
            } else {
                ImGui::TextColored(Theme::CrimsonCoral(), " ▼ ");
            }

            ImGui::SameLine();
            ImGui::TextColored(Theme::CrystalWhite(), "%s", tx.name);

            ImGui::SameLine(rightColW - 130.0f);
            ImGui::TextColored(tx.isBuy ? Theme::EmeraldGreen() : Theme::CrimsonCoral(), "%s", tx.amt);

            ImGui::SetCursorPosX(38.0f);
            ImGui::TextColored(Theme::TextMuted(), "%s", tx.date);

            ImGui::SameLine(rightColW - 90.0f);
            ImGui::TextColored(Theme::TextMuted(), "%s", tx.usd);

            if (i < txList.size() - 1) {
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
            }

            ImGui::PopID();
        }

        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
    }
    ImGui::EndChild();
}

// -------------------------------------------------------------
// 2. MARKETS VIEW
// -------------------------------------------------------------
static char s_marketFilter[64] = "";

void MarketsView::render() {
    ImGui::TextColored(Theme::SkyBlue(), "GLOBAL CRYPTO MARKETS & INSTITUTIONAL LIQUIDITY");
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::BeginTabBar("MarketsTabBar")) {
        // Tab 1: Live Market Tickers
        if (ImGui::BeginTabItem("[DATA] Live Market Tickers")) {
            ImGui::Spacing();
            ImGui::InputTextWithHint("##MarketSearch", "[SEARCH] Filter coins (e.g. BTC, ETH, SOL)...", s_marketFilter, sizeof(s_marketFilter));
            ImGui::SameLine();
            if (ImGui::Button("[REFRESH] Refresh Data")) {
                MarketManager::instance().fetchAllTickersAsync();
            }

            ImGui::Spacing();

            // Markets Table
            auto allTickers = MarketManager::instance().getAllTickers();
            if (allTickers.empty()) {
                auto supported = MarketManager::instance().getSupportedSymbols();
                for (const auto& s : supported) {
                    allTickers.push_back(MarketManager::instance().getTicker(s));
                }
            }

            if (ImGui::BeginTable("MarketsTable", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Sortable)) {
                ImGui::TableSetupColumn("Symbol", ImGuiTableColumnFlags_WidthFixed, 120.0f);
                ImGui::TableSetupColumn("Last Price", ImGuiTableColumnFlags_WidthFixed, 140.0f);
                ImGui::TableSetupColumn("24h Change", ImGuiTableColumnFlags_WidthFixed, 120.0f);
                ImGui::TableSetupColumn("24h High", ImGuiTableColumnFlags_WidthFixed, 120.0f);
                ImGui::TableSetupColumn("24h Low", ImGuiTableColumnFlags_WidthFixed, 120.0f);
                ImGui::TableSetupColumn("24h Volume (USDT)", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 170.0f);
                ImGui::TableHeadersRow();

                std::string filterUpper = s_marketFilter;
                std::transform(filterUpper.begin(), filterUpper.end(), filterUpper.begin(), ::toupper);

                for (const auto& t : allTickers) {
                    if (!filterUpper.empty() && t.symbol.find(filterUpper) == std::string::npos) {
                        continue;
                    }

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextColored(Theme::SkyBlue(), "%s", t.symbol.c_str());

                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("$%.4f", t.lastPrice);

                    ImGui::TableSetColumnIndex(2);
                    ImVec4 chgCol = (t.priceChangePercent24h >= 0) ? Theme::EmeraldGreen() : Theme::CrimsonCoral();
                    ImGui::TextColored(chgCol, "%s%.2f%%", t.priceChangePercent24h >= 0 ? "+" : "", t.priceChangePercent24h);

                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("$%.4f", t.high24h);

                    ImGui::TableSetColumnIndex(4);
                    ImGui::Text("$%.4f", t.low24h);

                    ImGui::TableSetColumnIndex(5);
                    ImGui::Text("$%.2f", t.quoteVolume24h > 0 ? t.quoteVolume24h : t.volume24h);

                    ImGui::TableSetColumnIndex(6);
                    ImGui::PushID(t.symbol.c_str());
                    if (ImGui::SmallButton("[CHART] Chart")) {
                        MarketManager::instance().setActiveSymbol(t.symbol);
                        UIManager::instance().setActiveView(ViewType::CHART);
                    }
                    ImGui::SameLine();
                    if (ImGui::SmallButton("[TRADE] Trade")) {
                        MarketManager::instance().setActiveSymbol(t.symbol);
                        UIManager::instance().setActiveView(ViewType::PAPER_TRADING);
                    }
                    ImGui::SameLine();
                    if (ImGui::SmallButton("[STAR] Watch")) {
                        Database::instance().addToWatchlist(t.symbol);
                        UIManager::instance().showToast("WATCHLIST", "Added " + t.symbol + " to watchlist", Theme::SkyBlue());
                    }
                    ImGui::PopID();
                }

                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }

        // Tab 2: Cross-Asset Correlation Heatmap
        if (ImGui::BeginTabItem("[LINK] Cross-Asset Correlation Heatmap")) {
            ImGui::Spacing();
            ImGui::TextColored(Theme::CrystalWhite(), "Live Pearson Correlation Matrix (Returns Series)");
            ImGui::TextColored(Theme::TextMuted(), "Values range from -1.0 (Inverse Correlation) to +1.0 (Strong Correlation). Green = Positively Correlated, Red = Negatively Correlated.");
            ImGui::Spacing();

            auto supported = MarketManager::instance().getSupportedSymbols();
            std::unordered_map<std::string, std::vector<Candle>> marketCandles;
            for (const auto& sym : supported) {
                marketCandles[sym] = MarketManager::instance().getCandles(sym, "1h");
            }

            auto corrMatrix = TechnicalAnalysisEngine::calculateCorrelationMatrix(supported, marketCandles);
            size_t S = corrMatrix.symbols.size();

            if (ImGui::BeginTable("CorrelationHeatmapTable", (int)S + 1, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollX)) {
                ImGui::TableSetupColumn("Pair / Asset", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                for (const auto& sym : corrMatrix.symbols) {
                    ImGui::TableSetupColumn(sym.c_str(), ImGuiTableColumnFlags_WidthFixed, 85.0f);
                }
                ImGui::TableHeadersRow();

                for (size_t r = 0; r < S; ++r) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextColored(Theme::SkyBlue(), "%s", corrMatrix.symbols[r].c_str());

                    for (size_t c = 0; c < S; ++c) {
                        ImGui::TableSetColumnIndex((int)c + 1);
                        double val = corrMatrix.correlations[r][c];

                        ImVec4 cellColor;
                        if (r == c) {
                            cellColor = ImVec4(0.5f, 0.5f, 0.5f, 0.8f);
                        } else if (val >= 0.70) {
                            cellColor = Theme::EmeraldGreen();
                        } else if (val >= 0.30) {
                            cellColor = ImVec4(0.4f, 0.85f, 0.5f, 1.0f);
                        } else if (val <= -0.30) {
                            cellColor = Theme::CrimsonCoral();
                        } else {
                            cellColor = Theme::TextMuted();
                        }

                        ImGui::TextColored(cellColor, "%s%.2f", val >= 0 ? "+" : "", val);
                    }
                }
                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }

        // Tab 3: Pro Order Flow & Depth Heatmap
        if (ImGui::BeginTabItem("[DOM] Pro Order Flow & Depth Analyzer")) {
            ImGui::Spacing();
            std::string activeSym = MarketManager::instance().getActiveSymbol();
            auto ob = MarketManager::instance().getOrderBook();

            double totalBidQty = 0.0, totalAskQty = 0.0;
            for (const auto& b : ob.bids) totalBidQty += b.quantity;
            for (const auto& a : ob.asks) totalAskQty += a.quantity;
            double totalDepth = totalBidQty + totalAskQty;
            double bidRatio = (totalDepth > 0.0) ? (totalBidQty / totalDepth) : 0.50;

            ImGui::TextColored(Theme::SkyBlue(), "Active Depth Target: %s", activeSym.c_str());
            ImGui::Spacing();

            // Order Book Imbalance Gauge
            ImGui::Text("Order Book Imbalance (Bids vs Asks Ratio):");
            ImGui::ProgressBar((float)bidRatio, ImVec2(-1, 22.0f), "");
            ImGui::TextColored(Theme::EmeraldGreen(), "[BUY] Bids: %.2f (%.1f%%)", totalBidQty, bidRatio * 100.0);
            ImGui::SameLine(ImGui::GetWindowWidth() - 250.0f);
            ImGui::TextColored(Theme::CrimsonCoral(), "[SELL] Asks: %.2f (%.1f%%)", totalAskQty, (1.0 - bidRatio) * 100.0);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Side-by-side Level 2 Order Book Depth
            ImGui::Columns(2, "L2DepthCols", true);
            ImGui::TextColored(Theme::EmeraldGreen(), "BUY BIDS (L2)");
            if (ImGui::BeginTable("BidsTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Bid Price");
                ImGui::TableSetupColumn("Quantity");
                ImGui::TableHeadersRow();
                for (size_t i = 0; i < std::min((size_t)12, ob.bids.size()); ++i) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextColored(Theme::EmeraldGreen(), "$%.4f", ob.bids[i].price);
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%.4f", ob.bids[i].quantity);
                }
                ImGui::EndTable();
            }

            ImGui::NextColumn();
            ImGui::TextColored(Theme::CrimsonCoral(), "SELL ASKS (L2)");
            if (ImGui::BeginTable("AsksTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Ask Price");
                ImGui::TableSetupColumn("Quantity");
                ImGui::TableHeadersRow();
                for (size_t i = 0; i < std::min((size_t)12, ob.asks.size()); ++i) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextColored(Theme::CrimsonCoral(), "$%.4f", ob.asks[i].price);
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%.4f", ob.asks[i].quantity);
                }
                ImGui::EndTable();
            }
            ImGui::Columns(1);

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}

// -------------------------------------------------------------
// 3. ADVANCED CANDLESTICK & LEVEL-3 ORDER FLOW CHART VIEW
// -------------------------------------------------------------
static bool s_showSMA = true;
static bool s_showEMA = true;
static bool s_showBB = true;
static bool s_showVWAP = true;
static bool s_showSuperTrend = true;
static bool s_showRSI = true;
static bool s_showMACD = true;
static bool s_showStoch = false;
static bool s_showPatterns = true;
static bool s_showOrderFlow = true;

void ChartView::render() {
    std::string activeSymbol = MarketManager::instance().getActiveSymbol();
    std::string activeTf = MarketManager::instance().getActiveTimeframe();
    auto candles = MarketManager::instance().getActiveCandles();

    // Chart Toolbar
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::CardSurface());
    ImGui::BeginChild("ChartToolbar", ImVec2(0, 42.0f), true);
    ImGui::PopStyleColor();

    // Symbol Dropdown
    ImGui::SetNextItemWidth(120.0f);
    if (ImGui::BeginCombo("##SymCombo", activeSymbol.c_str())) {
        for (const auto& s : MarketManager::instance().getSupportedSymbols()) {
            bool isSel = (s == activeSymbol);
            if (ImGui::Selectable(s.c_str(), isSel)) {
                MarketManager::instance().setActiveSymbol(s);
            }
            if (isSel) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    // Timeframe selector buttons
    ImGui::SameLine();
    for (const auto& tf : MarketManager::instance().getSupportedTimeframes()) {
        bool isTfSel = (tf == activeTf);
        if (isTfSel) ImGui::PushStyleColor(ImGuiCol_Button, Theme::SkyBlue());
        if (ImGui::Button(tf.c_str())) {
            MarketManager::instance().setActiveTimeframe(tf);
        }
        if (isTfSel) ImGui::PopStyleColor();
        ImGui::SameLine();
    }

    // Indicator toggles
    ImGui::SameLine(440.0f);
    ImGui::Checkbox("SMA", &s_showSMA);
    ImGui::SameLine();
    ImGui::Checkbox("EMA", &s_showEMA);
    ImGui::SameLine();
    ImGui::Checkbox("BB", &s_showBB);
    ImGui::SameLine();
    ImGui::Checkbox("VWAP", &s_showVWAP);
    ImGui::SameLine();
    ImGui::Checkbox("STrend", &s_showSuperTrend);
    ImGui::SameLine();
    ImGui::Checkbox("RSI", &s_showRSI);
    ImGui::SameLine();
    ImGui::Checkbox("MACD", &s_showMACD);
    ImGui::SameLine();
    ImGui::Checkbox("Stoch", &s_showStoch);
    ImGui::SameLine();
    ImGui::Checkbox("Patterns", &s_showPatterns);
    ImGui::SameLine();
    ImGui::Checkbox("L3 Depth & Tape", &s_showOrderFlow);

    // AI Analyze Button
    ImGui::SameLine(ImGui::GetWindowWidth() - 140.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, Theme::DeepFuchsia());
    if (ImGui::Button("[ACTION] Ask AI Chart")) {
        UIManager::instance().setActiveView(ViewType::AI_RESEARCH);
    }
    ImGui::PopStyleColor();

    ImGui::EndChild();

    if (candles.empty()) {
        ImGui::TextColored(Theme::TextMuted(), "Loading market candlesticks for %s...", activeSymbol.c_str());
        return;
    }

    // Technical Summary for Header
    auto techSummary = TechnicalAnalysisEngine::generateSummary(candles);
    ImGui::TextColored(Theme::CrystalWhite(), "%s %s  O: $%.2f  H: $%.2f  L: $%.2f  C: $%.2f  Vol: %.2f",
        activeSymbol.c_str(), activeTf.c_str(),
        candles.back().open, candles.back().high, candles.back().low, candles.back().close, candles.back().volume);
    ImGui::SameLine(ImGui::GetWindowWidth() - 320.0f);
    ImGui::TextColored(Theme::SkyBlue(), "RSI: %.1f | Trend: %s | VWAP: $%.2f", techSummary.rsi14, techSummary.trend.c_str(), techSummary.vwap);

    // Prepare arrays for ImPlot
    size_t N = candles.size();
    std::vector<double> xs(N);
    std::vector<double> opens(N), highs(N), lows(N), closes(N), volumes(N);

    for (size_t i = 0; i < N; ++i) {
        xs[i] = (double)i;
        opens[i] = candles[i].open;
        highs[i] = candles[i].high;
        lows[i] = candles[i].low;
        closes[i] = candles[i].close;
        volumes[i] = candles[i].volume;
    }

    // Calculate indicators
    auto sma20 = TechnicalAnalysisEngine::calculateSMA(candles, 20);
    auto sma50 = TechnicalAnalysisEngine::calculateSMA(candles, 50);
    auto ema20 = TechnicalAnalysisEngine::calculateEMA(candles, 20);
    auto bb = TechnicalAnalysisEngine::calculateBollingerBands(candles, 20, 2.0);
    auto vwapRes = TechnicalAnalysisEngine::calculateVWAP(candles, 1.0);
    auto stRes = TechnicalAnalysisEngine::calculateSuperTrend(candles, 10, 3.0);
    auto rsi = TechnicalAnalysisEngine::calculateRSI(candles, 14);
    auto macd = TechnicalAnalysisEngine::calculateMACD(candles, 12, 26, 9);
    auto stoch = TechnicalAnalysisEngine::calculateStochastic(candles, 14, 3, 3);
    auto patterns = s_showPatterns ? TechnicalAnalysisEngine::detectCandlestickPatterns(candles) : std::vector<CandlestickPattern>{};

    float subPlotCount = (s_showRSI ? 1.0f : 0.0f) + (s_showMACD ? 1.0f : 0.0f) + (s_showStoch ? 1.0f : 0.0f);
    float mainPlotHeight = ImGui::GetContentRegionAvail().y - (subPlotCount * 110.0f);
    if (mainPlotHeight < 200.0f) mainPlotHeight = 200.0f;

    // 1. MAIN CANDLESTICK PLOT
    if (ImPlot::BeginPlot("##CandlePlot", ImVec2(-1, mainPlotHeight), ImPlotFlags_NoTitle | ImPlotFlags_Crosshairs)) {
        ImPlot::SetupAxes("Candle Index", "Price (USD)", ImPlotAxisFlags_None, ImPlotAxisFlags_Opposite);
        ImPlot::SetupAxisLimits(ImAxis_X1, 0, (double)N, ImPlotCond_Always);

        // Draw Candlesticks & Volume
        for (size_t i = 0; i < N; ++i) {
            bool bullish = closes[i] >= opens[i];
            ImVec4 candleColor = bullish ? Theme::EmeraldGreen() : Theme::CrimsonCoral();

            // Wick Line
            double wickX[2] = {xs[i], xs[i]};
            double wickY[2] = {lows[i], highs[i]};
            ImPlotSpec wickSpec;
            wickSpec.LineColor = candleColor;
            wickSpec.LineWeight = 1.2f;
            ImPlot::PlotLine("##Wick", wickX, wickY, 2, wickSpec);

            // Candle Body
            double boxBottom = std::min(opens[i], closes[i]);
            double boxTop = std::max(opens[i], closes[i]);
            if (boxTop - boxBottom < 0.1) boxTop = boxBottom + 0.1;

            double bodyX[2] = {xs[i], xs[i]};
            double bodyY[2] = {boxBottom, boxTop};
            ImPlotSpec bodySpec;
            bodySpec.LineColor = candleColor;
            bodySpec.LineWeight = 5.0f;
            ImPlot::PlotLine("##Body", bodyX, bodyY, 2, bodySpec);
        }

        // Overlay SMA 20 & 50
        if (s_showSMA && !sma20.values.empty()) {
            size_t offset = N - sma20.values.size();
            std::vector<double> smaX(sma20.values.size());
            for (size_t k = 0; k < smaX.size(); ++k) smaX[k] = (double)(offset + k);

            ImPlotSpec sma20Spec;
            sma20Spec.LineColor = Theme::SkyBlue();
            sma20Spec.LineWeight = 1.5f;
            ImPlot::PlotLine("SMA 20", smaX.data(), sma20.values.data(), (int)smaX.size(), sma20Spec);

            if (!sma50.values.empty()) {
                size_t offset50 = N - sma50.values.size();
                std::vector<double> sma50X(sma50.values.size());
                for (size_t k = 0; k < sma50X.size(); ++k) sma50X[k] = (double)(offset50 + k);
                
                ImPlotSpec sma50Spec;
                sma50Spec.LineColor = Theme::BodyPink();
                sma50Spec.LineWeight = 1.5f;
                ImPlot::PlotLine("SMA 50", sma50X.data(), sma50.values.data(), (int)sma50X.size(), sma50Spec);
            }
        }

        // Overlay EMA 20
        if (s_showEMA && !ema20.values.empty()) {
            size_t offset = N - ema20.values.size();
            std::vector<double> emaX(ema20.values.size());
            for (size_t k = 0; k < emaX.size(); ++k) emaX[k] = (double)(offset + k);
            
            ImPlotSpec emaSpec;
            emaSpec.LineColor = Theme::DeepFuchsia();
            emaSpec.LineWeight = 1.8f;
            ImPlot::PlotLine("EMA 20", emaX.data(), ema20.values.data(), (int)emaX.size(), emaSpec);
        }

        // Overlay Bollinger Bands
        if (s_showBB && !bb.middle.empty()) {
            size_t offset = N - bb.middle.size();
            std::vector<double> bbX(bb.middle.size());
            for (size_t k = 0; k < bbX.size(); ++k) bbX[k] = (double)(offset + k);

            ImPlotSpec bbSpec;
            bbSpec.LineColor = ImVec4(0.7f, 0.7f, 0.9f, 0.6f);
            bbSpec.LineWeight = 1.0f;
            ImPlot::PlotLine("BB Upper", bbX.data(), bb.upper.data(), (int)bbX.size(), bbSpec);
            ImPlot::PlotLine("BB Lower", bbX.data(), bb.lower.data(), (int)bbX.size(), bbSpec);
        }

        // Overlay Institutional VWAP & StDev Envelopes
        if (s_showVWAP && !vwapRes.vwap.empty()) {
            std::vector<double> vwapX(vwapRes.vwap.size());
            for (size_t k = 0; k < vwapX.size(); ++k) vwapX[k] = (double)k;

            ImPlotSpec vwapSpec;
            vwapSpec.LineColor = Theme::SkyBlue();
            vwapSpec.LineWeight = 2.0f;
            ImPlot::PlotLine("VWAP", vwapX.data(), vwapRes.vwap.data(), (int)vwapX.size(), vwapSpec);

            ImPlotSpec vwapBandSpec;
            vwapBandSpec.LineColor = ImVec4(0.0f, 0.9f, 1.0f, 0.4f);
            vwapBandSpec.LineWeight = 1.0f;
            ImPlot::PlotLine("VWAP Upper +1Sigma", vwapX.data(), vwapRes.upperBand.data(), (int)vwapX.size(), vwapBandSpec);
            ImPlot::PlotLine("VWAP Lower -1Sigma", vwapX.data(), vwapRes.lowerBand.data(), (int)vwapX.size(), vwapBandSpec);
        }

        // Overlay SuperTrend Line
        if (s_showSuperTrend && !stRes.superTrend.empty()) {
            std::vector<double> stX(stRes.superTrend.size());
            for (size_t k = 0; k < stX.size(); ++k) stX[k] = (double)k;

            bool isBul = stRes.direction.empty() || stRes.direction.back() >= 0;
            ImPlotSpec stSpec;
            stSpec.LineColor = isBul ? Theme::EmeraldGreen() : Theme::CrimsonCoral();
            stSpec.LineWeight = 2.2f;
            ImPlot::PlotLine("SuperTrend", stX.data(), stRes.superTrend.data(), (int)stX.size(), stSpec);
        }

        // Overlay Candlestick Pattern tags
        if (s_showPatterns && !patterns.empty()) {
            for (const auto& p : patterns) {
                if (p.index < N) {
                    bool isBul = (p.bias == "BULLISH");
                    double tagY = isBul ? (lows[p.index] * 0.995) : (highs[p.index] * 1.005);
                    ImPlotSpec patSpec;
                    patSpec.LineColor = isBul ? Theme::EmeraldGreen() : Theme::CrimsonCoral();
                    patSpec.LineWeight = 4.0f;
                    double ptX[1] = {(double)p.index};
                    double ptY[1] = {tagY};
                    ImPlot::PlotLine(p.name.c_str(), ptX, ptY, 1, patSpec);
                }
            }
        }

        ImPlot::EndPlot();
    }

    // 2. RSI SUBPLOT
    if (s_showRSI && !rsi.values.empty()) {
        if (ImPlot::BeginPlot("##RSIPlot", ImVec2(-1, 100.0f), ImPlotFlags_NoTitle | ImPlotFlags_Crosshairs)) {
            ImPlot::SetupAxes(nullptr, "RSI (14)", ImPlotAxisFlags_NoTickLabels, ImPlotAxisFlags_Opposite);
            ImPlot::SetupAxisLimits(ImAxis_X1, 0, (double)N, ImPlotCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 100, ImPlotCond_Always);

            // Overbought (70) and Oversold (30) reference lines
            double refX[2] = {0.0, (double)N};
            double ref70[2] = {70.0, 70.0};
            double ref30[2] = {30.0, 30.0};
            
            ImPlotSpec obSpec;
            obSpec.LineColor = Theme::CrimsonCoral();
            obSpec.LineWeight = 1.0f;
            ImPlot::PlotLine("##OB70", refX, ref70, 2, obSpec);

            ImPlotSpec osSpec;
            osSpec.LineColor = Theme::EmeraldGreen();
            osSpec.LineWeight = 1.0f;
            ImPlot::PlotLine("##OS30", refX, ref30, 2, osSpec);

            size_t offset = N - rsi.values.size();
            std::vector<double> rsiX(rsi.values.size());
            for (size_t k = 0; k < rsiX.size(); ++k) rsiX[k] = (double)(offset + k);

            ImPlotSpec rsiSpec;
            rsiSpec.LineColor = Theme::BodyPink();
            rsiSpec.LineWeight = 1.8f;
            ImPlot::PlotLine("RSI", rsiX.data(), rsi.values.data(), (int)rsiX.size(), rsiSpec);

            ImPlot::EndPlot();
        }
    }

    // 3. MACD SUBPLOT
    if (s_showMACD && !macd.histogram.empty()) {
        if (ImPlot::BeginPlot("##MACDPlot", ImVec2(-1, 100.0f), ImPlotFlags_NoTitle | ImPlotFlags_Crosshairs)) {
            ImPlot::SetupAxes(nullptr, "MACD (12,26,9)", ImPlotAxisFlags_NoTickLabels, ImPlotAxisFlags_Opposite);
            ImPlot::SetupAxisLimits(ImAxis_X1, 0, (double)N, ImPlotCond_Always);

            size_t offset = N - macd.histogram.size();
            std::vector<double> macdX(macd.histogram.size());
            for (size_t k = 0; k < macdX.size(); ++k) macdX[k] = (double)(offset + k);

            // MACD Line & Signal Line
            ImPlotSpec macdSpec;
            macdSpec.LineColor = Theme::SkyBlue();
            macdSpec.LineWeight = 1.5f;
            ImPlot::PlotLine("MACD", macdX.data(), macd.macd.data(), (int)macdX.size(), macdSpec);

            ImPlotSpec sigSpec;
            sigSpec.LineColor = Theme::DeepFuchsia();
            sigSpec.LineWeight = 1.5f;
            ImPlot::PlotLine("Signal", macdX.data(), macd.signal.data(), (int)macdX.size(), sigSpec);

            // Histogram bars
            for (size_t k = 0; k < macd.histogram.size(); ++k) {
                double histVal = macd.histogram[k];
                ImVec4 histCol = (histVal >= 0.0) ? Theme::EmeraldGreen() : Theme::CrimsonCoral();
                double barX[2] = {macdX[k], macdX[k]};
                double barY[2] = {0.0, histVal};
                
                ImPlotSpec histSpec;
                histSpec.LineColor = histCol;
                histSpec.LineWeight = 2.5f;
                ImPlot::PlotLine("##Hist", barX, barY, 2, histSpec);
            }

            ImPlot::EndPlot();
        }
    }

    // 4. STOCHASTIC OSCILLATOR SUBPLOT
    if (s_showStoch && !stoch.k.empty()) {
        if (ImPlot::BeginPlot("##StochPlot", ImVec2(-1, 100.0f), ImPlotFlags_NoTitle | ImPlotFlags_Crosshairs)) {
            ImPlot::SetupAxes(nullptr, "Stoch (14,3,3)", ImPlotAxisFlags_NoTickLabels, ImPlotAxisFlags_Opposite);
            ImPlot::SetupAxisLimits(ImAxis_X1, 0, (double)N, ImPlotCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 100, ImPlotCond_Always);

            double refX[2] = {0.0, (double)N};
            double ref80[2] = {80.0, 80.0};
            double ref20[2] = {20.0, 20.0};

            ImPlotSpec obSpec;
            obSpec.LineColor = Theme::CrimsonCoral();
            obSpec.LineWeight = 1.0f;
            ImPlot::PlotLine("##OB80", refX, ref80, 2, obSpec);

            ImPlotSpec osSpec;
            osSpec.LineColor = Theme::EmeraldGreen();
            osSpec.LineWeight = 1.0f;
            ImPlot::PlotLine("##OS20", refX, ref20, 2, osSpec);

            size_t offset = N - stoch.k.size();
            std::vector<double> stochX(stoch.k.size());
            for (size_t k = 0; k < stochX.size(); ++k) stochX[k] = (double)(offset + k);

            ImPlotSpec kSpec;
        }
    }

    // 5. OPTIONAL SPLIT: LIVE LEVEL-3 ORDER BOOK DEPTH LADDER & TIME & SALES TAPE
    if (s_showOrderFlow) {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Columns(3, "OrderFlowCols", true);

        // COLUMN 1: Level 3 Dynamic Order Book Depth Ladder
        ImGui::TextColored(Theme::ElectricCyan(), "[DATA] LIVE LEVEL-3 DEPTH LADDER (DOM)");
        double markP = candles.back().close;
        double spreadBps = 0.8;
        ImGui::TextColored(Theme::TextMuted(), "Spread: %.1f bps | Mark: $%.2f", spreadBps, markP);

        if (ImGui::BeginTable("L3DepthTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 45.0f);
            ImGui::TableSetupColumn("Price ($)", ImGuiTableColumnFlags_WidthFixed, 85.0f);
            ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 60.0f);
            ImGui::TableSetupColumn("Depth Volume", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            // 5 Ask Rows (Descending from high to ask1)
            for (int a = 5; a >= 1; --a) {
                double aPrice = markP + (a * (markP * 0.0003));
                double aSize = 0.45 * a + (a * 0.12);
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(Theme::CrimsonCoral(), "ASK");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(Theme::CrimsonCoral(), "%.2f", aPrice);
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%.3f", aSize);
                ImGui::TableSetColumnIndex(3);
                float frac = (float)(aSize / 3.5);
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, Theme::CrimsonCoral());
                ImGui::ProgressBar(frac, ImVec2(-1, 14), "");
                ImGui::PopStyleColor();
            }

            // Spread Line
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextColored(Theme::AmberGold(), "MID");
            ImGui::TableSetColumnIndex(1);
            ImGui::TextColored(Theme::AmberGold(), "--- $%.2f ---", markP);
            ImGui::TableSetColumnIndex(2);
            ImGui::TextColored(Theme::AmberGold(), "---");
            ImGui::TableSetColumnIndex(3);
            ImGui::TextColored(Theme::AmberGold(), "--- SPREAD %.1f bps ---", spreadBps);

            // 5 Bid Rows (Descending from bid1 to lower)
            for (int b = 1; b <= 5; ++b) {
                double bPrice = markP - (b * (markP * 0.0003));
                double bSize = 0.52 * b + (b * 0.15);
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(Theme::NeonGreen(), "BID");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(Theme::NeonGreen(), "%.2f", bPrice);
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%.3f", bSize);
                ImGui::TableSetColumnIndex(3);
                float frac = (float)(bSize / 3.5);
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, Theme::NeonGreen());
                ImGui::ProgressBar(frac, ImVec2(-1, 14), "");
                ImGui::PopStyleColor();
            }

            ImGui::EndTable();
        }
        ImGui::NextColumn();

        // COLUMN 2: Live Time & Sales Whale Tape
        ImGui::TextColored(Theme::BodyPink(), "[ACTION] TIME & SALES TAPE (WHALE DETECTOR)");
        ImGui::TextColored(Theme::TextMuted(), "Aggressive Market Orders Stream");

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.04f, 0.04f, 0.06f, 0.90f));
        ImGui::BeginChild("TimeAndSalesTape", ImVec2(0, 220.0f), true);
        ImGui::PopStyleColor();

        // Simulated Streaming High-Frequency Tape
        static const struct { const char* time; const char* side; double price; double qty; bool isWhale; } tapeEvents[] = {
            {"13:42:01", "BUY",  68425.50, 42.50, true},
            {"13:42:00", "SELL", 68420.00, 1.25,  false},
            {"13:41:59", "BUY",  68421.20, 0.85,  false},
            {"13:41:58", "BUY",  68422.00, 15.00, true},
            {"13:41:55", "SELL", 68418.90, 2.10,  false},
            {"13:41:52", "SELL", 68417.50, 28.40, true},
            {"13:41:50", "BUY",  68420.00, 0.40,  false},
            {"13:41:48", "BUY",  68419.50, 3.20,  false}
        };

        for (const auto& ev : tapeEvents) {
            ImVec4 col = (std::string(ev.side) == "BUY") ? Theme::NeonGreen() : Theme::CrimsonCoral();
            if (ev.isWhale) {
                ImGui::TextColored(Theme::AmberGold(), "[WHALE] [%s] %s %.2f @ $%.2f", ev.time, ev.side, ev.qty, ev.price);
            } else {
                ImGui::TextColored(col, "  [%s] %s %.2f @ $%.2f", ev.time, ev.side, ev.qty, ev.price);
            }
        }
        ImGui::EndChild();
        ImGui::NextColumn();

        // COLUMN 3: Cumulative Volume Delta (CVD) & Microstructure Bias
        ImGui::TextColored(Theme::SkyBlue(), "[CHART] CUMULATIVE VOLUME DELTA (CVD)");
        ImGui::TextColored(Theme::TextMuted(), "Aggressive Taker Buy / Sell Imbalance");

        // CVD Calculation
        std::vector<double> cvdValues(N, 0.0);
        double runDelta = 0.0;
        for (size_t i = 0; i < N; ++i) {
            double delta = (closes[i] >= opens[i]) ? (volumes[i] * 0.6) : -(volumes[i] * 0.6);
            runDelta += delta;
            cvdValues[i] = runDelta;
        }

        if (ImPlot::BeginPlot("##CVDPlot", ImVec2(-1, 200.0f), ImPlotFlags_NoTitle | ImPlotFlags_Crosshairs)) {
            ImPlot::SetupAxes(nullptr, "CVD Delta", ImPlotAxisFlags_NoTickLabels, ImPlotAxisFlags_Opposite);
            ImPlot::SetupAxisLimits(ImAxis_X1, 0, (double)N, ImPlotCond_Always);

            ImPlotSpec cvdSpec;
            cvdSpec.LineColor = (cvdValues.back() >= 0) ? Theme::NeonGreen() : Theme::CrimsonCoral();
            cvdSpec.LineWeight = 2.0f;
            ImPlot::PlotLine("CVD", xs.data(), cvdValues.data(), (int)N, cvdSpec);
            ImPlot::EndPlot();
        }

        ImGui::Columns(1);
    }
}
// -------------------------------------------------------------
// 4. AI RESEARCH & QUANTITATIVE COPILOT VIEW
// -------------------------------------------------------------
static char s_aiInputBuf[1024] = "";
static std::vector<AIMessage> s_chatHistory;
static bool s_aiLoading = false;
static std::string s_activeConvId = "CONV-001";
static int s_providerChoice = 0; // 0: AUTO, 1: GROQ, 2: OLLAMA

// Strategy Generator State
static int s_stratTypeIdx = 0;
static int s_riskAppetiteIdx = 1; // 0: Conservative, 1: Moderate, 2: Aggressive
static std::string s_generatedStrategyOutput = "";

void AIResearchView::render() {
    ImGui::TextColored(Theme::BodyPink(), "[AI] AI MARKET COMPANION & QUANT RESEARCH ");
    ImGui::SameLine();
    ImGui::TextColored(Theme::TextMuted(), "- Real-time Multi-Model Intelligence");
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::BeginTabBar("AIResearchTabBar")) {
        // TAB 1: Conversational Quantitative Copilot
        if (ImGui::BeginTabItem("[CHAT] AI Companion Chat")) {
            ImGui::Spacing();
            // Top AI Settings Bar
            ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::CardSurface());
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0f);
            ImGui::BeginChild("AIChatToolbar", ImVec2(0, 56.0f), true);

            ImGui::Text("AI Engine:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(130.0f);
            const char* provNames[] = {" Auto Smart", "[ACTION] Groq (Llama 3.3)", "[LOCK] Ollama (Local)"};
            if (ImGui::Combo("##AIProvCombo", &s_providerChoice, provNames, 3)) {
                if (s_providerChoice == 0) Config::instance().setAIProvider(AIProviderType::AUTO);
                else if (s_providerChoice == 1) Config::instance().setAIProvider(AIProviderType::GROQ);
                else if (s_providerChoice == 2) Config::instance().setAIProvider(AIProviderType::OLLAMA);
            }

            ImGui::SameLine(250.0f);
            std::string provStatus = AIRouter::instance().getActiveProviderStatus();
            ImGui::TextColored(Theme::SkyBlue(), "Status: %s", provStatus.c_str());

            ImGui::SameLine(ImGui::GetWindowWidth() - 230.0f);
            if (ImGui::Button("[CLEAR] Clear")) {
                s_chatHistory.clear();
                Database::instance().deleteConversation(s_activeConvId);
            }
            ImGui::SameLine();
            if (ImGui::Button("+ New Chat")) {
                s_chatHistory.clear();
                s_activeConvId = "CONV-" + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
            }

            ImGui::EndChild();
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();

            // Quick Cute Prompt Macro Buttons
            ImGui::Spacing();
            std::string currentSym = MarketManager::instance().getActiveSymbol();
            if (ImGui::Button(" Trend & Sentiment")) {
                snprintf(s_aiInputBuf, sizeof(s_aiInputBuf), "Give me a quick, friendly summary of %s's current trend, momentum indicators, and market sentiment.", currentSym.c_str());
            }
            ImGui::SameLine();
            if (ImGui::Button("[MODEL] Support & Resistance")) {
                snprintf(s_aiInputBuf, sizeof(s_aiInputBuf), "Identify key support, resistance, and VWAP levels for %s with actionable entry/exit zones.", currentSym.c_str());
            }
            ImGui::SameLine();
            if (ImGui::Button("[DATA] Multi-Timeframe Score")) {
                snprintf(s_aiInputBuf, sizeof(s_aiInputBuf), "Provide a full multi-timeframe quantitative scoring breakdown (1m, 5m, 15m, 1h, 4h, 1D) for %s.", currentSym.c_str());
            }
            ImGui::SameLine();
            if (ImGui::Button("[RISK] Risk & Solvency Check")) {
                snprintf(s_aiInputBuf, sizeof(s_aiInputBuf), "Conduct a risk check on %s position sizing, stop-loss placement, and leverage safety.", currentSym.c_str());
            }

            ImGui::Spacing();

            // Message History Area
            float chatHeight = ImGui::GetContentRegionAvail().y - 50.0f;
            ImGui::BeginChild("ChatHistoryArea", ImVec2(0, chatHeight), true);

            if (s_chatHistory.empty()) {
                ImGui::SetCursorPos(ImVec2(30, 40));
                ImGui::TextColored(Theme::BodyPink(), "[MODEL] Welcome to your CRYPTØ AI Companion ");
                ImGui::SetCursorPosX(30);
                ImGui::TextWrapped("Ask any crypto questions, ask for live technical analysis, or test strategies! Real-time live Binance data for %s ($%.2f) is automatically shared with your AI companion.",
                    currentSym.c_str(), MarketManager::instance().getTicker(currentSym).lastPrice);
            }

            for (const auto& msg : s_chatHistory) {
                ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::CardSurface());
                ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 8.0f));

                if (msg.role == "user") {
                    ImGui::PushStyleColor(ImGuiCol_Text, Theme::SkyBlue());
                    ImGui::Text("YOU YOU:");
                    ImGui::PopStyleColor();
                    ImGui::Indent(10.0f);
                    ImGui::TextWrapped("%s", msg.content.c_str());
                    ImGui::Unindent(10.0f);
                } else {
                    ImGui::PushStyleColor(ImGuiCol_Text, Theme::BodyPink());
                    ImGui::Text("[AI] CRYPTØ AI:");
                    ImGui::PopStyleColor();
                    ImGui::Indent(10.0f);
                    ImGui::TextWrapped("%s", msg.content.c_str());
                    ImGui::Unindent(10.0f);
                }
                ImGui::PopStyleVar(2);
                ImGui::PopStyleColor();
                ImGui::Spacing();
            }

            if (s_aiLoading) {
                ImGui::Spacing();
                ImGui::TextColored(Theme::BodyPink(), "[AI] Thinking & analyzing live market telemetry for you...");
            }

            ImGui::EndChild();

            // Input Bar Area
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 90.0f);
            bool enterPressed = ImGui::InputTextWithHint("##ChatInput", "Type a quantitative research query or command (e.g. 'Analyze SOL')...", s_aiInputBuf, sizeof(s_aiInputBuf), ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::PopItemWidth();

            ImGui::SameLine();
            bool sendClicked = ImGui::Button("Send [SEND]", ImVec2(80.0f, 0));

            if ((enterPressed || sendClicked) && strlen(s_aiInputBuf) > 0 && !s_aiLoading) {
                std::string prompt = s_aiInputBuf;
                s_aiInputBuf[0] = '\0';

                // Check for quick command actions
                auto cmd = AICommandEngine::parse(prompt);
                std::string feedback;
                AICommandEngine::executeAction(cmd, feedback);

                AIMessage userMsg{"user", prompt, std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()};
                s_chatHistory.push_back(userMsg);

                // Build Structured Context
                std::string activeSym = MarketManager::instance().getActiveSymbol();
                std::string activeTf = MarketManager::instance().getActiveTimeframe();
                auto candles = MarketManager::instance().getActiveCandles();
                auto ticker = MarketManager::instance().getTicker(activeSym);
                auto techSummary = TechnicalAnalysisEngine::generateSummary(candles);

                AIRequest req;
                req.prompt = cmd.promptToSend;
                req.systemPrompt = AIContextBuilder::getSystemPrompt();
                req.history = s_chatHistory;
                req.structuredContext = AIContextBuilder::buildMarketContext(activeSym, activeTf, ticker, candles, techSummary);

                s_aiLoading = true;

                AIProviderType provOverride = (s_providerChoice == 1) ? AIProviderType::GROQ : (s_providerChoice == 2 ? AIProviderType::OLLAMA : AIProviderType::AUTO);

                AIRouter::instance().generateAsync(req, [](const AIResponse& resp) {
                    s_aiLoading = false;
                    AIMessage aiMsg{"assistant", resp.content, std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()};
                    s_chatHistory.push_back(aiMsg);
                    Database::instance().addMessage(s_activeConvId, aiMsg);
                }, provOverride);
            }

            ImGui::EndTabItem();
        }

        // TAB 2: Multi-Timeframe Confluence Engine
        if (ImGui::BeginTabItem("[NET] Multi-Timeframe Confluence Engine")) {
            ImGui::Spacing();
            std::string currentSym = MarketManager::instance().getActiveSymbol();
            
            // Collect candles across timeframes
            std::unordered_map<std::string, std::vector<Candle>> tfCandles;
            for (const auto& tf : MarketManager::instance().getSupportedTimeframes()) {
                tfCandles[tf] = MarketManager::instance().getCandles(currentSym, tf);
            }

            auto confluence = TechnicalAnalysisEngine::calculateMultiTimeframeConfluence(currentSym, tfCandles);

            // Top Confluence Header Card
            ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::CardSurface());
            ImGui::BeginChild("MTFHeaderCard", ImVec2(0, 100.0f), true);
            ImGui::PopStyleColor();

            ImGui::Columns(3, "MTFHeaderCols", false);
            ImGui::TextColored(Theme::TextMuted(), "Asset Under Inspection");
            ImGui::SetWindowFontScale(1.3f);
            ImGui::TextColored(Theme::SkyBlue(), "%s", currentSym.c_str());
            ImGui::SetWindowFontScale(1.0f);

            ImGui::NextColumn();
            ImGui::TextColored(Theme::TextMuted(), "Confluence Score (0 to 100)");
            ImVec4 scoreCol = (confluence.confluenceScore >= 60.0) ? Theme::EmeraldGreen() : (confluence.confluenceScore <= 40.0 ? Theme::CrimsonCoral() : Theme::AmberGold());
            ImGui::SetWindowFontScale(1.3f);
            ImGui::TextColored(scoreCol, "%.1f / 100", confluence.confluenceScore);
            ImGui::SetWindowFontScale(1.0f);

            ImGui::NextColumn();
            ImGui::TextColored(Theme::TextMuted(), "Algorithmic Regime Bias");
            ImGui::SetWindowFontScale(1.3f);
            ImGui::TextColored(scoreCol, "%s", confluence.overallBias.c_str());
            ImGui::SetWindowFontScale(1.0f);

            ImGui::Columns(1);
            ImGui::EndChild();

            ImGui::Spacing();
            ImGui::TextColored(Theme::CrystalWhite(), "Timeframe Horizon Breakdown");

            if (ImGui::BeginTable("MTFBreakdownTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Timeframe", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                ImGui::TableSetupColumn("Trend Bias", ImGuiTableColumnFlags_WidthFixed, 130.0f);
                ImGui::TableSetupColumn("RSI (14)", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                ImGui::TableSetupColumn("MACD Signal", ImGuiTableColumnFlags_WidthFixed, 130.0f);
                ImGui::TableSetupColumn("TF Score", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableHeadersRow();

                for (const auto& item : confluence.signals) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextColored(Theme::SkyBlue(), "%s", item.timeframe.c_str());

                    ImGui::TableSetColumnIndex(1);
                    ImVec4 tCol = (item.trend == "BULLISH") ? Theme::EmeraldGreen() : (item.trend == "BEARISH" ? Theme::CrimsonCoral() : Theme::TextMuted());
                    ImGui::TextColored(tCol, "%s", item.trend.c_str());

                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%.1f", item.rsi);

                    ImGui::TableSetColumnIndex(3);
                    ImVec4 mCol = (item.macd == "BULLISH") ? Theme::EmeraldGreen() : Theme::CrimsonCoral();
                    ImGui::TextColored(mCol, "%s", item.macd.c_str());

                    ImGui::TableSetColumnIndex(4);
                    ImGui::Text("%.1f / 100", item.score);
                }

                ImGui::EndTable();
            }

            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::CardSurface());
            ImGui::BeginChild("MTFSummaryBox", ImVec2(0, 80.0f), true);
            ImGui::PopStyleColor();
            ImGui::TextColored(Theme::SkyBlue(), "[ACTION] Quantitative AI Synthesis:");
            ImGui::TextWrapped("%s", confluence.recommendedAction.c_str());
            ImGui::EndChild();

            ImGui::EndTabItem();
        }

        // TAB 3: AI Strategy & Quant Rules Generator
        if (ImGui::BeginTabItem("[QUANT] AI Strategy & Quant Rules Generator")) {
            ImGui::Spacing();
            ImGui::TextColored(Theme::CrystalWhite(), "Automated Algorithmic Strategy & Risk Rules Blueprint Generator");
            ImGui::TextColored(Theme::TextMuted(), "Synthesize mathematically rigorous trading logic, stop-loss invalidation protocols, and entry/exit parameters grounded in active live market telemetry.");
            ImGui::Spacing();

            ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::CardSurface());
            ImGui::BeginChild("StratGenInputs", ImVec2(0, 80.0f), true);
            ImGui::PopStyleColor();

            ImGui::Columns(3, "StratGenCols", false);
            const char* stratTypes[] = {"Trend-Following (SuperTrend + EMA)", "Mean Reversion (Bollinger + RSI)", "Momentum Breakout (VWAP + MACD)", "High-Frequency Volatility Grid"};
            ImGui::Text("Strategy Archetype:");
            ImGui::Combo("##StratTypeCombo", &s_stratTypeIdx, stratTypes, 4);

            ImGui::NextColumn();
            const char* riskProfiles[] = {"Conservative (1% Risk, 1:3 R:R)", "Moderate (2% Risk, 1:2 R:R)", "Aggressive (3-5% Risk, Scalping)"};
            ImGui::Text("Risk Appetite Profile:");
            ImGui::Combo("##RiskAppetiteCombo", &s_riskAppetiteIdx, riskProfiles, 3);

            ImGui::NextColumn();
            ImGui::Spacing();
            if (ImGui::Button("[ACTION] GENERATE QUANT RULES", ImVec2(-1, 38))) {
                std::string sym = MarketManager::instance().getActiveSymbol();
                Ticker t = MarketManager::instance().getTicker(sym);
                auto candles = MarketManager::instance().getActiveCandles();
                auto tech = TechnicalAnalysisEngine::generateSummary(candles);

                std::string prompt = "Generate a comprehensive, institutional-grade quantitative trading strategy blueprint for " + sym +
                    ". Strategy Archetype: " + stratTypes[s_stratTypeIdx] +
                    ". Risk Profile: " + riskProfiles[s_riskAppetiteIdx] +
                    ". Current Market Price: $" + std::to_string(t.lastPrice) +
                    ". Technical Regime: RSI " + std::to_string(tech.rsi14) + ", Trend " + tech.trend + ", VWAP $" + std::to_string(tech.vwap) +
                    ". Provide exact mathematical entry conditions, invalidation levels, take-profit ladders, position sizing formula, and backtest guidance.";

                AIRequest req;
                req.prompt = prompt;
                req.systemPrompt = "You are a Chief Quantitative Risk Officer and Algorithmic Trading Systems Architect. Generate structured, actionable, and mathematically sound trading systems.";
                req.structuredContext = AIContextBuilder::buildMarketContext(sym, "1h", t, candles, tech);

                s_generatedStrategyOutput = "[WAIT] Synthesizing quantitative rulebook with zero-cost AI router...";

                AIRouter::instance().generateAsync(req, [](const AIResponse& resp) {
                    s_generatedStrategyOutput = resp.content;
                });
            }
            ImGui::Columns(1);
            ImGui::EndChild();

            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::CardSurface());
            ImGui::BeginChild("StratOutputArea", ImVec2(0, 0), true);
            ImGui::PopStyleColor();

            if (s_generatedStrategyOutput.empty()) {
                ImGui::TextColored(Theme::TextMuted(), "Click 'GENERATE QUANT RULES' above to synthesize custom algorithmic logic for the active asset.");
            } else {
                ImGui::TextColored(Theme::SkyBlue(), "GENERATED STRATEGY BLUEPRINT:");
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::TextWrapped("%s", s_generatedStrategyOutput.c_str());
            }

            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}

// -------------------------------------------------------------
// 5. PORTFOLIO VIEW
// -------------------------------------------------------------
static char s_addAssetSym[32] = "SOLUSDT";
static double s_addAssetQty = 10.0;
static double s_addAssetPrice = 180.0;

void PortfolioView::render() {
    ImGui::TextColored(Theme::BodyPink(), "PORTFOLIO TRACKER, ALLOCATION & INSTITUTIONAL RISK");
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::BeginTabBar("PortfolioTabBar")) {
        // TAB 1: Assets & Allocation
        if (ImGui::BeginTabItem("[PORTFOLIO] Assets & Holdings")) {
            ImGui::Spacing();
            auto summary = PortfolioManager::instance().getSummary();

            // Top Summary Cards
            ImGui::Columns(4, "PortSummaryCols", false);
            
            ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::CardSurface());
            ImGui::BeginChild("PCard1", ImVec2(0, 80.0f), true);
            ImGui::TextColored(Theme::TextMuted(), "Total Portfolio Value");
            ImGui::SetWindowFontScale(1.2f);
            ImGui::TextColored(Theme::EmeraldGreen(), "%s", formatMoney(summary.totalValueUsd).c_str());
            ImGui::SetWindowFontScale(1.0f);
            ImGui::EndChild();
            ImGui::NextColumn();

            ImGui::BeginChild("PCard2", ImVec2(0, 80.0f), true);
            ImGui::TextColored(Theme::TextMuted(), "Total Cost Basis");
            ImGui::SetWindowFontScale(1.2f);
            ImGui::Text("%s", formatMoney(summary.totalCostBasisUsd).c_str());
            ImGui::SetWindowFontScale(1.0f);
            ImGui::EndChild();
            ImGui::NextColumn();

            ImGui::BeginChild("PCard3", ImVec2(0, 80.0f), true);
            ImGui::TextColored(Theme::TextMuted(), "Total Unrealized PnL");
            ImVec4 pnlCol = (summary.totalUnrealizedPnL >= 0) ? Theme::EmeraldGreen() : Theme::CrimsonCoral();
            ImGui::SetWindowFontScale(1.2f);
            ImGui::TextColored(pnlCol, "%s%s (%.2f%%)", summary.totalUnrealizedPnL >= 0 ? "+" : "", formatMoney(summary.totalUnrealizedPnL).c_str(), summary.totalUnrealizedPnLPercent);
            ImGui::SetWindowFontScale(1.0f);
            ImGui::EndChild();
            ImGui::NextColumn();

            ImGui::BeginChild("PCard4", ImVec2(0, 80.0f), true);
            ImGui::TextColored(Theme::TextMuted(), "Best Performing Asset");
            ImGui::SetWindowFontScale(1.2f);
            ImGui::TextColored(Theme::SkyBlue(), "%s", summary.bestPerformer.c_str());
            ImGui::SetWindowFontScale(1.0f);
            ImGui::EndChild();
            ImGui::PopStyleColor();

            ImGui::Columns(1);
            ImGui::Spacing();

            // Add Asset Form
            ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::CardSurface());
            ImGui::BeginChild("AddAssetBar", ImVec2(0, 48.0f), true);
            ImGui::PopStyleColor();

            ImGui::Text("Add / Update Asset:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(100.0f);
            ImGui::InputText("##AddSym", s_addAssetSym, sizeof(s_addAssetSym));
            ImGui::SameLine();
            ImGui::Text("Qty:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(90.0f);
            ImGui::InputDouble("##AddQty", &s_addAssetQty, 0.0, 0.0, "%.3f");
            ImGui::SameLine();
            ImGui::Text("Avg Buy Price:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(100.0f);
            ImGui::InputDouble("##AddPrice", &s_addAssetPrice, 0.0, 0.0, "%.2f");
            ImGui::SameLine();
            if (ImGui::Button("+ Save Asset")) {
                PortfolioManager::instance().addOrUpdateAsset(s_addAssetSym, s_addAssetQty, s_addAssetPrice);
                UIManager::instance().showToast("PORTFOLIO", "Saved asset " + std::string(s_addAssetSym), Theme::EmeraldGreen());
            }

            ImGui::EndChild();

            ImGui::Spacing();

            // Assets Table
            auto assets = PortfolioManager::instance().getAssets();
            if (ImGui::BeginTable("PortAssetsTable", 8, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
                ImGui::TableSetupColumn("Asset", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                ImGui::TableSetupColumn("Holdings", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                ImGui::TableSetupColumn("Avg Buy Price", ImGuiTableColumnFlags_WidthFixed, 120.0f);
                ImGui::TableSetupColumn("Current Price", ImGuiTableColumnFlags_WidthFixed, 120.0f);
                ImGui::TableSetupColumn("Total Value", ImGuiTableColumnFlags_WidthFixed, 130.0f);
                ImGui::TableSetupColumn("Unrealized PnL", ImGuiTableColumnFlags_WidthFixed, 160.0f);
                ImGui::TableSetupColumn("Allocation", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                ImGui::TableHeadersRow();

                for (const auto& a : assets) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextColored(Theme::SkyBlue(), "%s", a.symbol.c_str());

                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%.4f", a.quantity);

                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("$%.2f", a.avgBuyPrice);

                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("$%.2f", a.currentPrice);

                    ImGui::TableSetColumnIndex(4);
                    ImGui::Text("%s", formatMoney(a.totalValue).c_str());

                    ImGui::TableSetColumnIndex(5);
                    ImVec4 aPnlCol = (a.unrealizedPnL >= 0) ? Theme::EmeraldGreen() : Theme::CrimsonCoral();
                    ImGui::TextColored(aPnlCol, "%s%s (%.2f%%)", a.unrealizedPnL >= 0 ? "+" : "", formatMoney(a.unrealizedPnL).c_str(), a.unrealizedPnLPercent);

                    ImGui::TableSetColumnIndex(6);
                    ImGui::ProgressBar(a.allocationPercent / 100.0f, ImVec2(-1, 14), (std::to_string((int)a.allocationPercent) + "%").c_str());

                    ImGui::TableSetColumnIndex(7);
                    ImGui::PushID(a.symbol.c_str());
                    if (ImGui::SmallButton("[DEL]")) {
                        PortfolioManager::instance().removeAsset(a.symbol);
                    }
                    ImGui::PopID();
                }

                ImGui::EndTable();
            }

            ImGui::EndTabItem();
        }

        // TAB 2: Institutional Risk & Stress Testing
        if (ImGui::BeginTabItem("[RISK] Institutional Risk & Stress Testing")) {
            ImGui::Spacing();
            ImGui::TextColored(Theme::CrystalWhite(), "Portfolio Quantitative Risk Metrics & Historical Scenario Shock Analysis");
            ImGui::TextColored(Theme::TextMuted(), "Simulate institutional black-swan macro liquidity shocks and evaluate Value-at-Risk (VaR) exposures against extreme market drawdowns.");
            ImGui::Spacing();

            std::unordered_map<std::string, double> currentPrices;
            for (const auto& s : MarketManager::instance().getSupportedSymbols()) {
                currentPrices[s] = MarketManager::instance().getTicker(s).lastPrice;
            }

            auto risk = PaperTradingEngine::instance().getRiskAnalytics(currentPrices);

            // 4 Risk Analytics Cards
            ImGui::Columns(4, "RiskCards", false);
            
            ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::CardSurface());
            ImGui::BeginChild("RCard1", ImVec2(0, 85.0f), true);
            ImGui::TextColored(Theme::TextMuted(), "Portfolio Beta (vs BTC)");
            ImGui::SetWindowFontScale(1.3f);
            ImGui::TextColored(Theme::SkyBlue(), "%.2f Beta", risk.portfolioBeta);
            ImGui::SetWindowFontScale(1.0f);
            ImGui::EndChild();
            ImGui::NextColumn();

            ImGui::BeginChild("RCard2", ImVec2(0, 85.0f), true);
            ImGui::TextColored(Theme::TextMuted(), "1-Day 95%% Value-at-Risk");
            ImGui::SetWindowFontScale(1.3f);
            ImGui::TextColored(Theme::CrimsonCoral(), "%s (%.1f%%)", formatMoney(risk.var95Daily).c_str(), risk.var95Percent);
            ImGui::SetWindowFontScale(1.0f);
            ImGui::EndChild();
            ImGui::NextColumn();

            ImGui::BeginChild("RCard3", ImVec2(0, 85.0f), true);
            ImGui::TextColored(Theme::TextMuted(), "1-Day 99%% Value-at-Risk");
            ImGui::SetWindowFontScale(1.3f);
            ImGui::TextColored(Theme::CrimsonCoral(), "%s (%.1f%%)", formatMoney(risk.var99Daily).c_str(), risk.var99Percent);
            ImGui::SetWindowFontScale(1.0f);
            ImGui::EndChild();
            ImGui::NextColumn();

            ImGui::BeginChild("RCard4", ImVec2(0, 85.0f), true);
            ImGui::TextColored(Theme::TextMuted(), "Diversification Score");
            ImGui::SetWindowFontScale(1.3f);
            ImGui::TextColored(Theme::SkyBlue(), "%.0f / 100", risk.diversificationScore);
            ImGui::SetWindowFontScale(1.0f);
            ImGui::EndChild();
            ImGui::PopStyleColor();

            ImGui::Columns(1);
            ImGui::Spacing();

            ImGui::TextColored(Theme::SkyBlue(), "HISTORICAL MACRO CRASH & BLACK SWAN SIMULATION MATRIX");
            if (ImGui::BeginTable("StressScenariosTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Macro Event / Shock Scenario", ImGuiTableColumnFlags_WidthFixed, 220.0f);
                ImGui::TableSetupColumn("Market Drop", ImGuiTableColumnFlags_WidthFixed, 120.0f);
                ImGui::TableSetupColumn("Projected PnL Impact", ImGuiTableColumnFlags_WidthFixed, 160.0f);
                ImGui::TableSetupColumn("Post-Shock Equity", ImGuiTableColumnFlags_WidthFixed, 150.0f);
                ImGui::TableSetupColumn("Solvency / Risk Verdict", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableHeadersRow();

                for (const auto& sc : risk.stressScenarios) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextColored(Theme::CrystalWhite(), "%s", sc.name.c_str());

                    ImGui::TableSetColumnIndex(1);
                    ImVec4 shkCol = (sc.shockPercent >= 0) ? Theme::EmeraldGreen() : Theme::CrimsonCoral();
                    ImGui::TextColored(shkCol, "%+.1f%%", sc.shockPercent);

                    ImGui::TableSetColumnIndex(2);
                    ImVec4 pnlCol = (sc.impactUsd >= 0) ? Theme::EmeraldGreen() : Theme::CrimsonCoral();
                    ImGui::TextColored(pnlCol, "%s%s", (sc.impactUsd >= 0 ? "+" : ""), formatMoney(sc.impactUsd).c_str());

                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("%s", formatMoney(sc.postShockTotalValue).c_str());

                    ImGui::TableSetColumnIndex(4);
                    bool safe = sc.postShockTotalValue > 0.0;
                    ImGui::TextColored(safe ? (sc.shockPercent >= 0 ? Theme::EmeraldGreen() : Theme::SkyBlue()) : Theme::CrimsonCoral(),
                        "%s", safe ? (sc.shockPercent >= 0 ? "[PASS] OPTIMAL EXPANSION" : "[WARN] SURVIVES WITH DRAWDOWN") : "[FAIL] MARGIN CALL / LIQUIDATION");
                }

                ImGui::EndTable();
            }

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}

// -------------------------------------------------------------
// 6. WATCHLIST VIEW
// -------------------------------------------------------------
static char s_watchAddSym[32] = "";

void WatchlistView::render() {
    ImGui::TextColored(Theme::SkyBlue(), "MY WATCHLIST");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::InputTextWithHint("##WatchSymAdd", "Enter symbol to watch (e.g. LINKUSDT)...", s_watchAddSym, sizeof(s_watchAddSym));
    ImGui::SameLine();
    if (ImGui::Button("+ Add to Watchlist") && strlen(s_watchAddSym) > 0) {
        Database::instance().addToWatchlist(s_watchAddSym);
        s_watchAddSym[0] = '\0';
    }

    ImGui::Spacing();
    auto watchlist = Database::instance().getWatchlist();
    if (watchlist.empty()) {
        watchlist = {"BTCUSDT", "ETHUSDT", "SOLUSDT", "BNBUSDT", "XRPUSDT"};
    }

    if (ImGui::BeginTable("WatchlistTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Symbol", ImGuiTableColumnFlags_WidthFixed, 140.0f);
        ImGui::TableSetupColumn("Price", ImGuiTableColumnFlags_WidthFixed, 140.0f);
        ImGui::TableSetupColumn("24h Change", ImGuiTableColumnFlags_WidthFixed, 140.0f);
        ImGui::TableSetupColumn("24h High", ImGuiTableColumnFlags_WidthFixed, 140.0f);
        ImGui::TableSetupColumn("24h Low", ImGuiTableColumnFlags_WidthFixed, 140.0f);
        ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        for (const auto& sym : watchlist) {
            Ticker t = MarketManager::instance().getTicker(sym);
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::TextColored(Theme::SkyBlue(), "%s", sym.c_str());

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("$%.4f", t.lastPrice);

            ImGui::TableSetColumnIndex(2);
            ImVec4 col = (t.priceChangePercent24h >= 0) ? Theme::EmeraldGreen() : Theme::CrimsonCoral();
            ImGui::TextColored(col, "%s%.2f%%", t.priceChangePercent24h >= 0 ? "+" : "", t.priceChangePercent24h);

            ImGui::TableSetColumnIndex(3);
            ImGui::Text("$%.4f", t.high24h);

            ImGui::TableSetColumnIndex(4);
            ImGui::Text("$%.4f", t.low24h);

            ImGui::TableSetColumnIndex(5);
            ImGui::PushID(sym.c_str());
            if (ImGui::SmallButton("[CHART] Chart")) {
                MarketManager::instance().setActiveSymbol(sym);
                UIManager::instance().setActiveView(ViewType::CHART);
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("[TRADE] Trade")) {
                MarketManager::instance().setActiveSymbol(sym);
                UIManager::instance().setActiveView(ViewType::PAPER_TRADING);
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("[DEL] Remove")) {
                Database::instance().removeFromWatchlist(sym);
            }
            ImGui::PopID();
        }

        ImGui::EndTable();
    }
}

// -------------------------------------------------------------
// 7. SCREENER VIEW
// -------------------------------------------------------------
static ScreenerFilter s_filter;
static std::vector<ScreenerResultItem> s_screenerResults;

void ScreenerView::render() {
    ImGui::TextColored(Theme::SkyBlue(), "MARKET TECHNICAL SCREENER");
    ImGui::Separator();
    ImGui::Spacing();

    // Filters Box
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::CardSurface());
    ImGui::BeginChild("ScreenerFilters", ImVec2(0, 80.0f), true);
    ImGui::PopStyleColor();

    ImGui::Columns(3, "ScreenerCols", false);
    ImGui::SliderFloat("Min 24h Change %", (float*)&s_filter.minChange24h, -30.0f, 30.0f, "%.1f%%");
    ImGui::SliderFloat("Max 24h Change %", (float*)&s_filter.maxChange24h, -30.0f, 50.0f, "%.1f%%");

    ImGui::NextColumn();
    ImGui::SliderFloat("Min RSI", (float*)&s_filter.minRsi, 0.0f, 100.0f, "%.0f");
    ImGui::SliderFloat("Max RSI", (float*)&s_filter.maxRsi, 0.0f, 100.0f, "%.0f");

    ImGui::NextColumn();
    if (ImGui::Button("[SEARCH] RUN MARKET SCAN", ImVec2(-1, 40))) {
        auto tickers = MarketManager::instance().getAllTickers();
        if (tickers.empty()) {
            for (const auto& s : MarketManager::instance().getSupportedSymbols()) {
                tickers.push_back(MarketManager::instance().getTicker(s));
            }
        }
        s_screenerResults = MarketScreener::runScreen(tickers, s_filter);
        UIManager::instance().showToast("SCREENER", "Found " + std::to_string(s_screenerResults.size()) + " matches", Theme::SkyBlue());
    }
    ImGui::Columns(1);

    ImGui::EndChild();

    ImGui::Spacing();

    // Results Table
    if (ImGui::BeginTable("ScreenerTable", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
        ImGui::TableSetupColumn("Symbol", ImGuiTableColumnFlags_WidthFixed, 110.0f);
        ImGui::TableSetupColumn("Price", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableSetupColumn("24h Change", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableSetupColumn("RSI (14)", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Trend Regime", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableSetupColumn("Quantitative Signal", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableHeadersRow();

        for (const auto& res : s_screenerResults) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextColored(Theme::SkyBlue(), "%s", res.symbol.c_str());

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("$%.4f", res.price);

            ImGui::TableSetColumnIndex(2);
            ImVec4 cCol = (res.change24h >= 0) ? Theme::EmeraldGreen() : Theme::CrimsonCoral();
            ImGui::TextColored(cCol, "%s%.2f%%", res.change24h >= 0 ? "+" : "", res.change24h);

            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%.1f", res.rsi);

            ImGui::TableSetColumnIndex(4);
            ImVec4 tCol = (res.trend == "BULLISH") ? Theme::EmeraldGreen() : (res.trend == "BEARISH" ? Theme::CrimsonCoral() : Theme::TextMuted());
            ImGui::TextColored(tCol, "%s", res.trend.c_str());

            ImGui::TableSetColumnIndex(5);
            ImGui::Text("%s", res.matchReason.c_str());

            ImGui::TableSetColumnIndex(6);
            ImGui::PushID(res.symbol.c_str());
            if (ImGui::SmallButton("[CHART] Open")) {
                MarketManager::instance().setActiveSymbol(res.symbol);
                UIManager::instance().setActiveView(ViewType::CHART);
            }
            ImGui::PopID();
        }

        ImGui::EndTable();
    }
}

// -------------------------------------------------------------
// 8. PAPER TRADING VIEW
// -------------------------------------------------------------
static int s_orderSide = 0; // 0: BUY, 1: SELL
static int s_orderType = 0; // 0: MARKET, 1: LIMIT
static double s_tradeQty = 0.1;
static double s_tradeLimitPrice = 96000.0;
static int s_leverageIdx = 0; // 0: 1x, 1: 2x, 2: 5x, 3: 10x, 4: 20x, 5: 50x
static const double s_leverages[] = {1.0, 2.0, 5.0, 10.0, 20.0, 50.0};
static const char* s_leverageNames[] = {"1x (Spot)", "2x Margin", "5x Leveraged", "10x Institutional", "20x High-Risk", "50x Degen"};
static double s_stopLossPrice = 0.0;
static double s_takeProfitPrice = 0.0;
static double s_trailingStopPct = 0.0;
static bool s_enableTrailing = false;
static double s_riskSizingPercent = 2.0;

void PaperTradingView::render() {
    ImGui::TextColored(Theme::EmeraldGreen(), "SIMULATED PAPER TRADING DESK ($0 REAL RISK)");
    ImGui::Separator();
    ImGui::Spacing();

    auto acc = PaperTradingEngine::instance().getAccountState();

    // Account Summary Bar
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::CardSurface());
    ImGui::BeginChild("PaperAccHeader", ImVec2(0, 75.0f), true);
    ImGui::PopStyleColor();

    ImGui::Columns(6, "PaperAccCols", false);
    ImGui::TextColored(Theme::TextMuted(), "Cash Balance");
    ImGui::TextColored(Theme::CrystalWhite(), "%s", formatMoney(acc.cashBalance).c_str());

    ImGui::NextColumn();
    ImGui::TextColored(Theme::TextMuted(), "Total Equity");
    ImGui::TextColored(Theme::EmeraldGreen(), "%s", formatMoney(acc.totalPortfolioValue).c_str());

    ImGui::NextColumn();
    ImGui::TextColored(Theme::TextMuted(), "Used / Free Margin");
    ImGui::Text("%s / %s", formatMoney(acc.usedMargin).c_str(), formatMoney(acc.freeMargin).c_str());

    ImGui::NextColumn();
    ImGui::TextColored(Theme::TextMuted(), "Total Realized PnL");
    ImVec4 rCol = (acc.totalRealizedPnL >= 0) ? Theme::EmeraldGreen() : Theme::CrimsonCoral();
    ImGui::TextColored(rCol, "%s%s", acc.totalRealizedPnL >= 0 ? "+" : "", formatMoney(acc.totalRealizedPnL).c_str());

    ImGui::NextColumn();
    ImGui::TextColored(Theme::TextMuted(), "Win Rate");
    double winRate = (acc.totalTradesCount > 0) ? ((double)acc.winningTradesCount / acc.totalTradesCount * 100.0) : 0.0;
    ImGui::Text("%.1f%% (%d/%d)", winRate, acc.winningTradesCount, acc.totalTradesCount);

    ImGui::NextColumn();
    if (ImGui::Button("[REFRESH] Reset ($10,000)")) {
        PaperTradingEngine::instance().reset(10000.0);
    }
    ImGui::Columns(1);
    ImGui::EndChild();

    ImGui::Spacing();

    // Split: Left (Order Execution Ticket) | Right (Positions & Orders)
    float colWidth = 340.0f;
    ImGui::BeginChild("PaperOrderTicket", ImVec2(colWidth, 0), true);
    {
        ImGui::TextColored(Theme::SkyBlue(), "EXECUTION ORDER TICKET");
        ImGui::Separator();
        ImGui::Spacing();

        std::string sym = MarketManager::instance().getActiveSymbol();
        Ticker t = MarketManager::instance().getTicker(sym);
        ImGui::Text("Asset: %s", sym.c_str());
        ImGui::Text("Spot: $%.2f", t.lastPrice);
        ImGui::Spacing();

        // Leverage Selector
        ImGui::TextColored(Theme::BodyPink(), "Leverage Tier:");
        ImGui::SetNextItemWidth(-1);
        ImGui::Combo("##LeverageCombo", &s_leverageIdx, s_leverageNames, 6);
        double activeLev = s_leverages[s_leverageIdx];

        ImGui::Spacing();
        // Side selector
        const char* sides[] = {"BUY (LONG)", "SELL (SHORT)"};
        ImGui::SetNextItemWidth(-1);
        ImGui::Combo("Side", &s_orderSide, sides, 2);

        // Type selector
        const char* types[] = {"MARKET", "LIMIT"};
        ImGui::SetNextItemWidth(-1);
        ImGui::Combo("Type", &s_orderType, types, 2);

        ImGui::Spacing();
        ImGui::Text("Order Quantity:");
        ImGui::SetNextItemWidth(-1);
        ImGui::InputDouble("##TradeQty", &s_tradeQty, 0.01, 0.1, "%.4f");

        if (s_orderType == 1) {
            if (s_tradeLimitPrice <= 0.0) s_tradeLimitPrice = t.lastPrice;
            ImGui::Text("Limit Price ($):");
            ImGui::SetNextItemWidth(-1);
            ImGui::InputDouble("##LimitPrice", &s_tradeLimitPrice, 10.0, 100.0, "%.2f");
        }

        double baseExecPrice = (s_orderType == 0) ? t.lastPrice : s_tradeLimitPrice;
        double notionalValue = s_tradeQty * baseExecPrice;
        double requiredMargin = notionalValue / activeLev;

        // Stop Loss & Take Profit inputs
        ImGui::Spacing();
        ImGui::Text("Stop Loss ($):");
        ImGui::SetNextItemWidth(-1);
        ImGui::InputDouble("##SLPrice", &s_stopLossPrice, 10.0, 100.0, "%.2f");

        ImGui::Text("Take Profit ($):");
        ImGui::SetNextItemWidth(-1);
        ImGui::InputDouble("##TPPrice", &s_takeProfitPrice, 10.0, 100.0, "%.2f");

        // Trailing Stop Checkbox
        ImGui::Checkbox("Trailing Stop (%)", &s_enableTrailing);
        if (s_enableTrailing) {
            ImGui::SameLine();
            ImGui::SetNextItemWidth(80.0f);
            ImGui::InputDouble("##TrailPct", &s_trailingStopPct, 0.5, 1.0, "%.1f");
        }

        // Sizing Calculator Shortcut
        if (s_stopLossPrice > 0.0) {
            ImGui::Spacing();
            if (ImGui::SmallButton("[CALC] Auto Sizing (2% Capital Risk)")) {
                double recQty = PaperTradingEngine::instance().calculateRecommendedPositionSize(s_riskSizingPercent, baseExecPrice, s_stopLossPrice);
                if (recQty > 0.0) s_tradeQty = recQty;
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextColored(Theme::TextMuted(), "Notional: %s", formatMoney(notionalValue).c_str());
        ImGui::TextColored(Theme::SkyBlue(), "Req. Margin (%s): %s", s_leverageNames[s_leverageIdx], formatMoney(requiredMargin).c_str());
        
        // Estimated liquidation price
        if (activeLev > 1.0) {
            double estLiq = (s_orderSide == 0) ? (baseExecPrice * (1.0 - 0.9 / activeLev)) : (baseExecPrice * (1.0 + 0.9 / activeLev));
            ImGui::TextColored(Theme::CrimsonCoral(), "Est. Liq Price: $%.2f", estLiq);
        }

        ImGui::Spacing();
        ImVec4 btnCol = (s_orderSide == 0) ? Theme::EmeraldGreen() : Theme::CrimsonCoral();
        ImGui::PushStyleColor(ImGuiCol_Button, btnCol);
        std::string btnText = (s_orderSide == 0 ? "EXECUTE LONG (" : "EXECUTE SHORT (") + std::to_string((int)activeLev) + "x)";
        if (ImGui::Button(btnText.c_str(), ImVec2(-1, 38))) {
            std::string err;
            double trStop = s_enableTrailing ? s_trailingStopPct : 0.0;
            if (s_orderType == 0) {
                PaperTradingEngine::instance().executeMarketOrder(sym, s_orderSide == 0 ? OrderSide::BUY : OrderSide::SELL, s_tradeQty, t.lastPrice, err, activeLev, s_stopLossPrice, s_takeProfitPrice, trStop);
            } else {
                PaperTradingEngine::instance().placeLimitOrder(sym, s_orderSide == 0 ? OrderSide::BUY : OrderSide::SELL, s_tradeQty, s_tradeLimitPrice, err, activeLev, s_stopLossPrice, s_takeProfitPrice);
            }
            if (!err.empty()) {
                UIManager::instance().showToast("ORDER ERROR", err, Theme::CrimsonCoral());
            } else {
                UIManager::instance().showToast("ORDER PLACED", "Successfully placed " + sym + " order", Theme::EmeraldGreen());
            }
        }
        ImGui::PopStyleColor();
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // Right: Positions & Trade History
    ImGui::BeginChild("PaperPositionsAndHistory", ImVec2(0, 0), false);
    {
        ImGui::TextColored(Theme::CrystalWhite(), "ACTIVE MARGIN & LEVERAGED POSITIONS");
        auto positions = PaperTradingEngine::instance().getPositions();
        if (ImGui::BeginTable("PositionsTable", 9, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollX)) {
            ImGui::TableSetupColumn("Symbol", ImGuiTableColumnFlags_WidthFixed, 90.0f);
            ImGui::TableSetupColumn("Side", ImGuiTableColumnFlags_WidthFixed, 60.0f);
            ImGui::TableSetupColumn("Size / Lev", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("Entry", ImGuiTableColumnFlags_WidthFixed, 90.0f);
            ImGui::TableSetupColumn("Mark", ImGuiTableColumnFlags_WidthFixed, 90.0f);
            ImGui::TableSetupColumn("Liq. Price", ImGuiTableColumnFlags_WidthFixed, 90.0f);
            ImGui::TableSetupColumn("SL / TP", ImGuiTableColumnFlags_WidthFixed, 110.0f);
            ImGui::TableSetupColumn("Unrealized PnL", ImGuiTableColumnFlags_WidthFixed, 130.0f);
            ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 140.0f);
            ImGui::TableHeadersRow();

            for (const auto& pos : positions) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(Theme::SkyBlue(), "%s", pos.symbol.c_str());

                ImGui::TableSetColumnIndex(1);
                ImVec4 sCol = (pos.side == OrderSide::BUY) ? Theme::EmeraldGreen() : Theme::CrimsonCoral();
                ImGui::TextColored(sCol, "%s", pos.side == OrderSide::BUY ? "LONG" : "SHORT");

                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%.4f (%dx)", pos.quantity, (int)pos.leverage);

                ImGui::TableSetColumnIndex(3);
                ImGui::Text("$%.2f", pos.avgEntryPrice);

                ImGui::TableSetColumnIndex(4);
                ImGui::Text("$%.2f", pos.currentPrice);

                ImGui::TableSetColumnIndex(5);
                if (pos.liquidationPrice > 0.0) {
                    ImGui::TextColored(Theme::CrimsonCoral(), "$%.2f", pos.liquidationPrice);
                } else {
                    ImGui::TextColored(Theme::TextMuted(), "N/A");
                }

                ImGui::TableSetColumnIndex(6);
                ImGui::Text("$%.1f / $%.1f", pos.stopLossPrice, pos.takeProfitPrice);

                ImGui::TableSetColumnIndex(7);
                ImVec4 pCol = (pos.unrealizedPnL >= 0) ? Theme::EmeraldGreen() : Theme::CrimsonCoral();
                ImGui::TextColored(pCol, "%s%s (%.2f%%)", pos.unrealizedPnL >= 0 ? "+" : "", formatMoney(pos.unrealizedPnL).c_str(), pos.unrealizedPnLPercent);

                ImGui::TableSetColumnIndex(8);
                ImGui::PushID(pos.symbol.c_str());
                if (ImGui::SmallButton("[REFRESH] Reverse")) {
                    std::string err;
                    PaperTradingEngine::instance().reversePosition(pos.symbol, pos.currentPrice, err);
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("Close")) {
                    std::string err;
                    PaperTradingEngine::instance().closePosition(pos.symbol, pos.currentPrice, err);
                }
                ImGui::PopID();
            }
            ImGui::EndTable();
        }

        ImGui::Spacing();
        ImGui::TextColored(Theme::CrystalWhite(), "ORDER & TRADE HISTORY");
        auto history = PaperTradingEngine::instance().getOrderHistory(30);
        if (ImGui::BeginTable("HistoryTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
            ImGui::TableSetupColumn("Order ID");
            ImGui::TableSetupColumn("Symbol");
            ImGui::TableSetupColumn("Side");
            ImGui::TableSetupColumn("Price");
            ImGui::TableSetupColumn("Quantity");
            ImGui::TableSetupColumn("Realized PnL");
            ImGui::TableHeadersRow();

            for (const auto& ord : history) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s", ord.orderId.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(Theme::SkyBlue(), "%s", ord.symbol.c_str());
                ImGui::TableSetColumnIndex(2);
                ImVec4 sCol = (ord.side == OrderSide::BUY) ? Theme::EmeraldGreen() : Theme::CrimsonCoral();
                ImGui::TextColored(sCol, "%s", ord.side == OrderSide::BUY ? "BUY" : "SELL");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("$%.2f", ord.executedPrice);
                ImGui::TableSetColumnIndex(4);
                ImGui::Text("%.4f", ord.quantity);
                ImGui::TableSetColumnIndex(5);
                if (ord.side == OrderSide::SELL) {
                    ImVec4 rPnLCol = (ord.realizedPnL >= 0) ? Theme::EmeraldGreen() : Theme::CrimsonCoral();
                    ImGui::TextColored(rPnLCol, "%s%s", ord.realizedPnL >= 0 ? "+" : "", formatMoney(ord.realizedPnL).c_str());
                } else {
                    ImGui::TextColored(Theme::TextMuted(), "-");
                }
            }
            ImGui::EndTable();
        }
    }
    ImGui::EndChild();
}

// -------------------------------------------------------------
// 9. ALERTS VIEW
// -------------------------------------------------------------
static char s_alertSym[32] = "BTCUSDT";
static int s_alertCondIdx = 0;
static double s_alertTargetVal = 100000.0;
static char s_alertMsg[128] = "BTC Breakout Alert";

void AlertsView::render() {
    ImGui::TextColored(Theme::CrimsonCoral(), "PRICE & TECHNICAL INDICATOR ALERTS");
    ImGui::Separator();
    ImGui::Spacing();

    // Create Alert Box
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::CardSurface());
    ImGui::BeginChild("CreateAlertBox", ImVec2(0, 95.0f), true);
    ImGui::PopStyleColor();

    ImGui::Columns(4, "AlertCols", false);
    ImGui::Text("Symbol:");
    ImGui::SetNextItemWidth(100.0f);
    ImGui::InputText("##AlertSym", s_alertSym, sizeof(s_alertSym));

    ImGui::NextColumn();
    ImGui::Text("Condition:");
    const char* condNames[] = {
        "Price Above", "Price Below", "RSI Above", "RSI Below",
        "EMA Cross Up", "EMA Cross Down", "MACD Bullish Cross", "MACD Bearish Cross"
    };
    ImGui::SetNextItemWidth(150.0f);
    ImGui::Combo("##AlertCondCombo", &s_alertCondIdx, condNames, 8);

    ImGui::NextColumn();
    ImGui::Text("Target Value:");
    ImGui::SetNextItemWidth(120.0f);
    ImGui::InputDouble("##AlertTarget", &s_alertTargetVal, 10.0, 100.0, "%.2f");

    ImGui::NextColumn();
    ImGui::Spacing();
    if (ImGui::Button("[ALERT] CREATE ALERT", ImVec2(-1, 36))) {
        AlertManager::instance().addAlert(s_alertSym, static_cast<AlertCondition>(s_alertCondIdx), s_alertTargetVal, s_alertMsg);
        UIManager::instance().showToast("ALERT CREATED", "Alert created for " + std::string(s_alertSym), Theme::EmeraldGreen());
    }
    ImGui::Columns(1);
    ImGui::EndChild();

    ImGui::Spacing();

    // Active Alerts Table
    auto alerts = AlertManager::instance().getAlerts();
    if (ImGui::BeginTable("AlertsListTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("ID");
        ImGui::TableSetupColumn("Symbol");
        ImGui::TableSetupColumn("Condition");
        ImGui::TableSetupColumn("Target");
        ImGui::TableSetupColumn("Status");
        ImGui::TableSetupColumn("Actions");
        ImGui::TableHeadersRow();

        for (const auto& a : alerts) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", a.id.c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::TextColored(Theme::SkyBlue(), "%s", a.symbol.c_str());
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%s", condNames[static_cast<int>(a.condition)]);
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%.2f", a.targetValue);
            ImGui::TableSetColumnIndex(4);
            if (a.isTriggered) {
                ImGui::TextColored(Theme::CrimsonCoral(), "TRIGGERED");
            } else {
                ImGui::TextColored(a.isActive ? Theme::EmeraldGreen() : Theme::TextMuted(), "%s", a.isActive ? "ACTIVE" : "PAUSED");
            }
            ImGui::TableSetColumnIndex(5);
            ImGui::PushID(a.id.c_str());
            if (ImGui::SmallButton(a.isActive ? "Pause" : "Resume")) {
                AlertManager::instance().toggleAlert(a.id, !a.isActive);
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Delete")) {
                AlertManager::instance().removeAlert(a.id);
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
}

// -------------------------------------------------------------
// 10. NEWS VIEW
// -------------------------------------------------------------
void NewsView::render() {
    ImGui::TextColored(Theme::SkyBlue(), "REAL-TIME CRYPTO NEWS & SENTIMENT FEED");
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("[REFRESH] Refresh News")) {
        NewsManager::instance().refreshNewsAsync();
    }
    ImGui::SameLine();
    if (ImGui::Button("[ACTION] AI News Digest")) {
        UIManager::instance().setActiveView(ViewType::AI_RESEARCH);
    }

    ImGui::Spacing();
    auto news = NewsManager::instance().getLatestNews();

    for (const auto& item : news) {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::CardSurface());
        ImGui::BeginChild(item.id.c_str(), ImVec2(0, 85.0f), true);
        ImGui::PopStyleColor();

        // Title and Sentiment badge
        ImVec4 sentCol = (item.sentiment == "BULLISH") ? Theme::EmeraldGreen() : (item.sentiment == "BEARISH" ? Theme::CrimsonCoral() : Theme::TextMuted());
        ImGui::TextColored(sentCol, "[%s]", item.sentiment.c_str());
        ImGui::SameLine();
        ImGui::TextColored(Theme::CrystalWhite(), "%s", item.title.c_str());

        ImGui::Spacing();
        ImGui::TextColored(Theme::TextMuted(), "Source: %s", item.source.c_str());
        ImGui::SameLine(ImGui::GetWindowWidth() - 140.0f);
        if (ImGui::SmallButton("[ACTION] Analyze Topic")) {
            UIManager::instance().setActiveView(ViewType::AI_RESEARCH);
        }

        ImGui::EndChild();
        ImGui::Spacing();
    }
}

// -------------------------------------------------------------
// 11. BACKTESTING VIEW
// -------------------------------------------------------------
static BacktestConfig s_btConfig;
static BacktestResult s_btResult;
static bool s_btRun = false;

// Optimization state
static std::vector<ParameterSweepItem> s_optResults;
static bool s_optRun = false;

// Monte Carlo state
static MonteCarloSimulationResult s_mcResult;
static bool s_mcRun = false;

void BacktestingView::render() {
    ImGui::TextColored(Theme::SkyBlue(), "INSTITUTIONAL STRATEGY BACKTESTING, PARAMETER OPTIMIZER & MONTE CARLO RISK SIMULATOR");
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::BeginTabBar("BacktestTabBar")) {
        // TAB 1: Single Strategy Backtester
        if (ImGui::BeginTabItem("[TEST] Strategy Backtest Simulator")) {
            ImGui::Spacing();
            // Configuration Box
            ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::CardSurface());
            ImGui::BeginChild("BTConfigBox", ImVec2(0, 95.0f), true);
            ImGui::PopStyleColor();

            ImGui::Columns(4, "BTCols", false);
            const char* strats[] = {
                "SMA_CROSS", "EMA_CROSS", "RSI_MOMENTUM", "MACD_TREND", 
                "BOLLINGER_REVERSION", "SUPERTREND", "CONFLUENCE_BB_RSI", "GRID_BOT"
            };
            static int stratIdx = 0;
            ImGui::Text("Strategy Archetype:");
            if (ImGui::Combo("##StratCombo", &stratIdx, strats, 8)) {
                s_btConfig.strategyName = strats[stratIdx];
            }

            ImGui::NextColumn();
            ImGui::Text("Initial Capital ($):");
            ImGui::InputDouble("##BTCapital", &s_btConfig.initialCapital, 1000.0, 5000.0, "%.0f");

            ImGui::NextColumn();
            ImGui::Text("Fast / Slow Params:");
            ImGui::InputInt("Fast", &s_btConfig.param1);
            ImGui::SameLine();
            ImGui::InputInt("Slow", &s_btConfig.param2);

            ImGui::NextColumn();
            ImGui::Spacing();
            if (ImGui::Button("[TEST] RUN SIMULATION", ImVec2(-1, 38))) {
                auto candles = MarketManager::instance().getActiveCandles();
                s_btConfig.symbol = MarketManager::instance().getActiveSymbol();
                s_btConfig.timeframe = MarketManager::instance().getActiveTimeframe();
                s_btConfig.strategyName = strats[stratIdx];
                s_btResult = BacktestingEngine::runBacktest(candles, s_btConfig);
                s_btRun = true;
                UIManager::instance().showToast("BACKTEST COMPLETE", "Strategy: " + s_btResult.strategyName + " Return: " + std::to_string(s_btResult.totalReturnPercent).substr(0, 5) + "%", Theme::EmeraldGreen());
            }
            ImGui::Columns(1);
            ImGui::EndChild();

            ImGui::Spacing();

            if (s_btRun) {
                // Summary Performance Metrics Cards
                ImGui::Columns(6, "BTMetricsCols1", false);
                
                ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::CardSurface());
                ImGui::BeginChild("MCard1", ImVec2(0, 75.0f), true);
                ImGui::TextColored(Theme::TextMuted(), "Total Return");
                ImVec4 rCol = (s_btResult.totalReturn >= 0) ? Theme::EmeraldGreen() : Theme::CrimsonCoral();
                ImGui::TextColored(rCol, "%s%s (%.2f%%)", s_btResult.totalReturn >= 0 ? "+" : "", formatMoney(s_btResult.totalReturn).c_str(), s_btResult.totalReturnPercent);
                ImGui::EndChild();
                ImGui::NextColumn();

                ImGui::BeginChild("MCard2", ImVec2(0, 75.0f), true);
                ImGui::TextColored(Theme::TextMuted(), "Max Drawdown");
                ImGui::TextColored(Theme::CrimsonCoral(), "%.2f%%", s_btResult.maxDrawdownPercent);
                ImGui::EndChild();
                ImGui::NextColumn();

                ImGui::BeginChild("MCard3", ImVec2(0, 75.0f), true);
                ImGui::TextColored(Theme::TextMuted(), "Sharpe Ratio");
                ImVec4 shCol = (s_btResult.sharpeRatio >= 1.0) ? Theme::EmeraldGreen() : (s_btResult.sharpeRatio >= 0.0 ? Theme::SkyBlue() : Theme::CrimsonCoral());
                ImGui::TextColored(shCol, "%.2f", s_btResult.sharpeRatio);
                ImGui::EndChild();
                ImGui::NextColumn();

                ImGui::BeginChild("MCard4", ImVec2(0, 75.0f), true);
                ImGui::TextColored(Theme::TextMuted(), "Sortino Ratio");
                ImGui::TextColored(Theme::SkyBlue(), "%.2f", s_btResult.sortinoRatio);
                ImGui::EndChild();
                ImGui::NextColumn();

                ImGui::BeginChild("MCard5", ImVec2(0, 75.0f), true);
                ImGui::TextColored(Theme::TextMuted(), "Calmar Ratio");
                ImGui::Text("%.2f", s_btResult.calmarRatio);
                ImGui::EndChild();
                ImGui::NextColumn();

                ImGui::BeginChild("MCard6", ImVec2(0, 75.0f), true);
                ImGui::TextColored(Theme::TextMuted(), "Trade Expectancy");
                ImVec4 expCol = (s_btResult.expectancy >= 0) ? Theme::EmeraldGreen() : Theme::CrimsonCoral();
                ImGui::TextColored(expCol, "%s%s", (s_btResult.expectancy >= 0 ? "+" : ""), formatMoney(s_btResult.expectancy).c_str());
                ImGui::EndChild();
                ImGui::PopStyleColor();

                ImGui::Columns(1);
                ImGui::Spacing();

                // Win rate and secondary metrics bar
                ImGui::TextColored(Theme::SkyBlue(), "Win Rate: %.1f%% (%d / %d trades) | Profit Factor: %.2f | Avg Trade: %.2f%% | Total Return: %+.2f%%",
                    s_btResult.winRatePercent, s_btResult.winningTrades, s_btResult.totalTrades, s_btResult.profitFactor, s_btResult.averageTradeReturnPercent, s_btResult.totalReturnPercent);

                ImGui::Spacing();

                // Equity Curve Plot
                if (!s_btResult.equityCurve.empty()) {
                    if (ImPlot::BeginPlot("Equity Curve", ImVec2(-1, 240.0f))) {
                        std::vector<double> xs(s_btResult.equityCurve.size());
                        for (size_t i = 0; i < xs.size(); ++i) xs[i] = (double)i;

                        ImPlotSpec eqSpec;
                        eqSpec.LineColor = Theme::EmeraldGreen();
                        eqSpec.LineWeight = 2.2f;
                        ImPlot::PlotLine("Strategy Equity ($)", xs.data(), s_btResult.equityCurve.data(), (int)xs.size(), eqSpec);

                        // Baseline Initial Capital Line
                        double baseLineX[2] = {0.0, (double)xs.size()};
                        double baseLineY[2] = {s_btConfig.initialCapital, s_btConfig.initialCapital};
                        ImPlotSpec baseSpec;
                        baseSpec.LineColor = Theme::TextMuted();
                        baseSpec.LineWeight = 1.0f;
                        ImPlot::PlotLine("Initial Capital", baseLineX, baseLineY, 2, baseSpec);

                        ImPlot::EndPlot();
                    }
                }
            }

            ImGui::EndTabItem();
        }

        // TAB 2: Parameter Sweep Grid Optimizer
        if (ImGui::BeginTabItem("[ACTION] Parameter Sweep Grid Optimizer")) {
            ImGui::Spacing();
            ImGui::TextColored(Theme::CrystalWhite(), "Systematic Quantitative Parameter Sweep & Sensitivity Heatmap");
            ImGui::TextColored(Theme::TextMuted(), "Grid search hundreds of fast/slow parameter combinations across historical candles to determine optimal risk-adjusted Sharpe ratios and prevent curve overfitting.");
            ImGui::Spacing();

            if (ImGui::Button("[ACTION] RUN PARAMETER OPTIMIZATION GRID", ImVec2(300.0f, 36))) {
                auto candles = MarketManager::instance().getActiveCandles();
                s_btConfig.symbol = MarketManager::instance().getActiveSymbol();
                s_btConfig.timeframe = MarketManager::instance().getActiveTimeframe();
                s_optResults = BacktestingEngine::runParameterOptimization(candles, s_btConfig);
                s_optRun = true;
                UIManager::instance().showToast("OPTIMIZER COMPLETE", "Evaluated " + std::to_string(s_optResults.size()) + " parameter sets", Theme::EmeraldGreen());
            }

            ImGui::Spacing();

            if (s_optRun && !s_optResults.empty()) {
                ImGui::TextColored(Theme::SkyBlue(), "Optimal Top Parameter Sets Ranked by Sharpe Ratio:");
                if (ImGui::BeginTable("OptResultsTable", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
                    ImGui::TableSetupColumn("Rank", ImGuiTableColumnFlags_WidthFixed, 60.0f);
                    ImGui::TableSetupColumn("Fast Param", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                    ImGui::TableSetupColumn("Slow Param", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                    ImGui::TableSetupColumn("Total Return", ImGuiTableColumnFlags_WidthFixed, 130.0f);
                    ImGui::TableSetupColumn("Max Drawdown", ImGuiTableColumnFlags_WidthFixed, 120.0f);
                    ImGui::TableSetupColumn("Sharpe Ratio", ImGuiTableColumnFlags_WidthFixed, 110.0f);
                    ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 120.0f);
                    ImGui::TableHeadersRow();

                    int rank = 1;
                    for (const auto& item : s_optResults) {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        if (rank == 1) {
                            ImGui::TextColored(Theme::AmberGold(), "[TOP] #1");
                        } else {
                            ImGui::Text("#%d", rank);
                        }

                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("%d", item.param1);

                        ImGui::TableSetColumnIndex(2);
                        ImGui::Text("%d", item.param2);

                        ImGui::TableSetColumnIndex(3);
                        ImVec4 retCol = (item.totalReturnPercent >= 0) ? Theme::EmeraldGreen() : Theme::CrimsonCoral();
                        ImGui::TextColored(retCol, "%+.2f%%", item.totalReturnPercent);

                        ImGui::TableSetColumnIndex(4);
                        ImGui::TextColored(Theme::CrimsonCoral(), "%.2f%%", item.maxDrawdownPercent);

                        ImGui::TableSetColumnIndex(5);
                        ImGui::TextColored(Theme::SkyBlue(), "%.2f", item.sharpeRatio);

                        ImGui::TableSetColumnIndex(6);
                        ImGui::PushID(rank);
                        if (ImGui::SmallButton("Apply Params")) {
                            s_btConfig.param1 = item.param1;
                            s_btConfig.param2 = item.param2;
                            UIManager::instance().showToast("PARAMS APPLIED", "Fast: " + std::to_string(item.param1) + " Slow: " + std::to_string(item.param2), Theme::EmeraldGreen());
                        }
                        ImGui::PopID();
                        rank++;
                    }

                    ImGui::EndTable();
                }
            }

            ImGui::EndTabItem();
        }

        // TAB 3: Monte Carlo Risk Simulator
        if (ImGui::BeginTabItem("[SIM] Monte Carlo Risk Simulator")) {
            ImGui::Spacing();
            ImGui::TextColored(Theme::CrystalWhite(), "1,000-Path Monte Carlo Bootstrap Resampling Engine");
            ImGui::TextColored(Theme::TextMuted(), "Reshuffles historical trade sequences 1,000 times to model sequence-of-returns risk, statistical ruin probability, and worst-case drawdowns under severe regimes.");
            ImGui::Spacing();

            if (ImGui::Button("[SIM] RUN 1,000-PATH MONTE CARLO SIMULATION", ImVec2(340.0f, 36))) {
                if (s_btResult.trades.empty()) {
                    auto candles = MarketManager::instance().getActiveCandles();
                    s_btConfig.symbol = MarketManager::instance().getActiveSymbol();
                    s_btConfig.timeframe = MarketManager::instance().getActiveTimeframe();
                    s_btResult = BacktestingEngine::runBacktest(candles, s_btConfig);
                }
                s_mcResult = BacktestingEngine::runMonteCarloSimulation(s_btResult.trades, s_btConfig.initialCapital, 1000);
                s_mcRun = true;
                UIManager::instance().showToast("MONTE CARLO READY", "Ran 1,000 simulation paths", Theme::EmeraldGreen());
            }

            ImGui::Spacing();

            if (s_mcRun) {
                // Monte Carlo Analytics Cards
                ImGui::Columns(4, "MCCards", false);
                
                ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::CardSurface());
                ImGui::BeginChild("MCCard1", ImVec2(0, 80.0f), true);
                ImGui::TextColored(Theme::TextMuted(), "Median Final Equity");
                ImGui::SetWindowFontScale(1.2f);
                ImGui::TextColored(Theme::EmeraldGreen(), "%s", formatMoney(s_mcResult.medianFinalCapital).c_str());
                ImGui::SetWindowFontScale(1.0f);
                ImGui::EndChild();
                ImGui::NextColumn();

                ImGui::BeginChild("MCCard2", ImVec2(0, 80.0f), true);
                ImGui::TextColored(Theme::TextMuted(), "95%% / 99%% VaR (Capital)");
                ImGui::SetWindowFontScale(1.1f);
                ImGui::Text("%s / %s", formatMoney(s_mcResult.var95FinalCapital).c_str(), formatMoney(s_mcResult.var99FinalCapital).c_str());
                ImGui::SetWindowFontScale(1.0f);
                ImGui::EndChild();
                ImGui::NextColumn();

                ImGui::BeginChild("MCCard3", ImVec2(0, 80.0f), true);
                ImGui::TextColored(Theme::TextMuted(), "Worst-Case Drawdown");
                ImGui::SetWindowFontScale(1.2f);
                ImGui::TextColored(Theme::CrimsonCoral(), "%.2f%%", s_mcResult.worstCaseDrawdownPercent);
                ImGui::SetWindowFontScale(1.0f);
                ImGui::EndChild();
                ImGui::NextColumn();

                ImGui::BeginChild("MCCard4", ImVec2(0, 80.0f), true);
                ImGui::TextColored(Theme::TextMuted(), "Probability of Ruin");
                ImGui::SetWindowFontScale(1.2f);
                ImVec4 ruinCol = (s_mcResult.probabilityOfRuinPercent < 5.0) ? Theme::EmeraldGreen() : Theme::CrimsonCoral();
                ImGui::TextColored(ruinCol, "%.2f%%", s_mcResult.probabilityOfRuinPercent);
                ImGui::SetWindowFontScale(1.0f);
                ImGui::EndChild();
                ImGui::PopStyleColor();

                ImGui::Columns(1);
                ImGui::Spacing();

                // Monte Carlo Sampled Paths Plot
                if (!s_mcResult.samplePaths.empty()) {
                    if (ImPlot::BeginPlot("Sampled Monte Carlo Equity Paths (20 Paths Shown)", ImVec2(-1, 260.0f))) {
                        for (size_t p = 0; p < s_mcResult.samplePaths.size(); ++p) {
                            const auto& path = s_mcResult.samplePaths[p];
                            std::vector<double> xs(path.size());
                            for (size_t i = 0; i < xs.size(); ++i) xs[i] = (double)i;

                            ImPlotSpec pathSpec;
                            pathSpec.LineColor = (p == 0) ? Theme::EmeraldGreen() : ImVec4(0.3f, 0.6f, 0.9f, 0.35f);
                            pathSpec.LineWeight = (p == 0) ? 2.5f : 1.0f;
                            std::string label = (p == 0) ? "Median Path" : ("##Path" + std::to_string(p));
                            ImPlot::PlotLine(label.c_str(), xs.data(), path.data(), (int)xs.size(), pathSpec);
                        }
                        ImPlot::EndPlot();
                    }
                }
            }

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}

// -------------------------------------------------------------
// 12. SYSTEM MONITOR VIEW
// -------------------------------------------------------------
static std::vector<float> s_cpuHistory;
static std::vector<float> s_ramHistory;

void SystemMonitorView::render() {
    ImGui::TextColored(Theme::SkyBlue(), "HARDWARE & PERFORMANCE SYSTEM MONITOR");
    ImGui::Separator();
    ImGui::Spacing();

    auto metrics = SystemMetricsCollector::instance().getMetrics();

    s_cpuHistory.push_back((float)metrics.cpuUsagePercent);
    if (s_cpuHistory.size() > 100) s_cpuHistory.erase(s_cpuHistory.begin());

    s_ramHistory.push_back((float)metrics.memoryUsageMb);
    if (s_ramHistory.size() > 100) s_ramHistory.erase(s_ramHistory.begin());

    ImGui::Columns(2, "MonitorCols", false);
    
    // CPU Graph
    ImGui::Text("CPU Usage (%%)");
    ImGui::PlotLines("##CPULines", s_cpuHistory.data(), (int)s_cpuHistory.size(), 0, nullptr, 0.0f, 100.0f, ImVec2(-1, 120.0f));

    ImGui::NextColumn();
    // RAM Graph
    ImGui::Text("Resident Memory (MB)");
    ImGui::PlotLines("##RAMLines", s_ramHistory.data(), (int)s_ramHistory.size(), 0, nullptr, 0.0f, 500.0f, ImVec2(-1, 120.0f));
    ImGui::Columns(1);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Telemetry Table
    if (ImGui::BeginTable("TelemetryTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Metric", ImGuiTableColumnFlags_WidthFixed, 240.0f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        auto addRow = [](const char* name, const std::string& val) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextColored(Theme::TextMuted(), "%s", name);
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", val.c_str());
        };

        addRow("Application Rendering FPS", std::to_string(metrics.fps));
        addRow("Frame Time", std::to_string(metrics.frameTimeMs) + " ms");
        addRow("Active AI Provider", metrics.activeAIProvider + " (" + metrics.aiModel + ")");
        addRow("AI Query Latency", std::to_string(metrics.aiLatencyMs) + " ms");
        addRow("WebSocket Latency", std::to_string(metrics.wsLatencyMs) + " ms");
        addRow("REST API Latency", std::to_string(metrics.restLatencyMs) + " ms");
        addRow("Active Thread Count", std::to_string(metrics.activeThreads));
        addRow("SQLite Prepared Queries Executed", std::to_string(metrics.dbQueryCount));
        addRow("System Uptime", metrics.appUptime);

        ImGui::EndTable();
    }
}

// -------------------------------------------------------------
// 13. SETTINGS VIEW
// -------------------------------------------------------------
static char s_settingsGroqKey[128] = "";
static char s_settingsOllamaUrl[128] = "http://127.0.0.1:11434";
static char s_settingsTwilioSid[128] = "";
static char s_settingsTwilioToken[128] = "";
static char s_settingsTwilioFrom[64] = "";
static char s_settingsTwilioTo[64] = "";
static bool s_settingsLoaded = false;
static bool s_showApiKey = false;
static bool s_showTwilioSecret = false;

void SettingsView::render() {
    ImGui::TextColored(Theme::SkyBlue(), "GLOBAL SETTINGS & SECURITY CONFIGURATION");
    ImGui::Separator();
    ImGui::Spacing();

    if (!s_settingsLoaded) {
        std::string curKey = Config::instance().getGroqApiKey();
        snprintf(s_settingsGroqKey, sizeof(s_settingsGroqKey), "%s", curKey.c_str());
        std::string curUrl = Config::instance().getOllamaBaseUrl();
        snprintf(s_settingsOllamaUrl, sizeof(s_settingsOllamaUrl), "%s", curUrl.c_str());

        std::string twSid = Config::instance().getTwilioAccountSid();
        snprintf(s_settingsTwilioSid, sizeof(s_settingsTwilioSid), "%s", twSid.c_str());
        std::string twTok = Config::instance().getTwilioAuthToken();
        snprintf(s_settingsTwilioToken, sizeof(s_settingsTwilioToken), "%s", twTok.c_str());
        std::string twFrom = Config::instance().getTwilioFromNumber();
        snprintf(s_settingsTwilioFrom, sizeof(s_settingsTwilioFrom), "%s", twFrom.c_str());
        std::string twTo = Config::instance().getTwilioToNumber();
        snprintf(s_settingsTwilioTo, sizeof(s_settingsTwilioTo), "%s", twTo.c_str());

        s_settingsLoaded = true;
    }

    ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::CardSurface());
    ImGui::BeginChild("SettingsForm", ImVec2(0, 0), true);
    ImGui::PopStyleColor();

    ImGui::TextColored(Theme::BodyPink(), "1. ZERO-COST AI PROVIDER CREDENTIALS (100%% Free Forever)");
    ImGui::Text("Groq API Key (Free Tier Cloud AI - https://console.groq.com):");
    ImGuiInputTextFlags keyFlags = s_showApiKey ? 0 : ImGuiInputTextFlags_Password;
    ImGui::InputText("##GroqKeyInput", s_settingsGroqKey, sizeof(s_settingsGroqKey), keyFlags);
    ImGui::SameLine();
    if (ImGui::Button(s_showApiKey ? "Hide [LOCK]" : "Show [EYE]")) {
        s_showApiKey = !s_showApiKey;
    }

    ImGui::Spacing();
    ImGui::Text("Ollama Local Endpoint (Private On-Device AI - 100%% Free & Offline):");
    ImGui::InputText("##OllamaUrlInput", s_settingsOllamaUrl, sizeof(s_settingsOllamaUrl));
    ImGui::SameLine();
    if (ImGui::Button("Test Ollama Connection")) {
        bool avail = AIRouter::instance().getOllamaProvider()->isAvailable();
        if (avail) {
            UIManager::instance().showToast("OLLAMA", "Connected to Ollama successfully!", Theme::EmeraldGreen());
        } else {
            UIManager::instance().showToast("OLLAMA", "Failed to connect. Make sure ollama is running.", Theme::CrimsonCoral());
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextColored(Theme::AmberGold(), "2. TWILIO FREE SMS & WHATSAPP MOBILE DISPATCHER (Free Trial Tier)");
    ImGui::TextDisabled("Get free trial credits from https://twilio.com (No paid billing required for alerts)");

    ImGui::Text("Twilio Account SID (e.g. ACxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx):");
    ImGui::InputText("##TwilioSidInput", s_settingsTwilioSid, sizeof(s_settingsTwilioSid));

    ImGui::Text("Twilio Auth Token:");
    ImGuiInputTextFlags twFlags = s_showTwilioSecret ? 0 : ImGuiInputTextFlags_Password;
    ImGui::InputText("##TwilioTokenInput", s_settingsTwilioToken, sizeof(s_settingsTwilioToken), twFlags);
    ImGui::SameLine();
    if (ImGui::Button(s_showTwilioSecret ? "Hide [LOCK]##tw" : "Show [EYE]##tw")) {
        s_showTwilioSecret = !s_showTwilioSecret;
    }

    ImGui::Text("Twilio Phone Number (From, e.g. +1234567890 or whatsapp:+14155238886):");
    ImGui::InputText("##TwilioFromInput", s_settingsTwilioFrom, sizeof(s_settingsTwilioFrom));

    ImGui::Text("Your Mobile Number (To, e.g. +919876543210 or whatsapp:+919876543210):");
    ImGui::InputText("##TwilioToInput", s_settingsTwilioTo, sizeof(s_settingsTwilioTo));

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextColored(Theme::SkyBlue(), "3. APPLICATION PREFERENCES");
    bool isDemo = Config::instance().isDemoMode();
    if (ImGui::Checkbox("Enable Demo / Offline Simulation Mode", &isDemo)) {
        MarketManager::instance().setDemoMode(isDemo);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("[SAVE] SAVE ALL SETTINGS", ImVec2(220.0f, 38))) {
        Config::instance().setGroqApiKey(s_settingsGroqKey);
        Config::instance().setOllamaBaseUrl(s_settingsOllamaUrl);
        Config::instance().setTwilioAccountSid(s_settingsTwilioSid);
        Config::instance().setTwilioAuthToken(s_settingsTwilioToken);
        Config::instance().setTwilioFromNumber(s_settingsTwilioFrom);
        Config::instance().setTwilioToNumber(s_settingsTwilioTo);
        Config::instance().save();
        UIManager::instance().showToast("SETTINGS", "Configuration saved to .env successfully.", Theme::EmeraldGreen());
    }

    ImGui::EndChild();
}

// -------------------------------------------------------------
// 14. HACKER CONSOLE QUANT REPL VIEW
// -------------------------------------------------------------
void HackerConsoleView::render() {
    HackerConsole::instance().render(true);
}

} // namespace crypto
