#include "ai/AIContextBuilder.hpp"
#include <iomanip>
#include <sstream>

namespace crypto {

nlohmann::json AIContextBuilder::buildMarketContext(const std::string& symbol,
                                                   const std::string& timeframe,
                                                   const Ticker& ticker,
                                                   const std::vector<Candle>& candles,
                                                   const TechnicalSummary& techSummary) {
    nlohmann::json j;
    j["symbol"] = symbol;
    j["timeframe"] = timeframe;
    j["price"] = ticker.lastPrice > 0 ? ticker.lastPrice : techSummary.currentPrice;
    j["change24h"] = ticker.priceChangePercent24h;
    j["high24h"] = ticker.high24h;
    j["low24h"] = ticker.low24h;
    j["volume24h"] = ticker.volume24h;

    // Technical Indicators
    j["rsi"] = techSummary.rsi14;
    j["rsiState"] = techSummary.rsiState;
    j["ema20"] = techSummary.ema20;
    j["ema50"] = techSummary.ema50;
    j["ema200"] = techSummary.ema200;
    j["sma20"] = techSummary.sma20;
    j["sma50"] = techSummary.sma50;
    j["macdLine"] = techSummary.macdLine;
    j["macdSignal"] = techSummary.macdSignal;
    j["macdHist"] = techSummary.macdHist;
    j["macdCross"] = techSummary.macdCross;
    j["trend"] = techSummary.trend;
    j["atr14"] = techSummary.atr14;

    nlohmann::json bb;
    bb["upper"] = techSummary.bbUpper;
    bb["middle"] = techSummary.bbMiddle;
    bb["lower"] = techSummary.bbLower;
    j["bollinger"] = bb;

    // Recent 5 candles summary
    if (!candles.empty()) {
        nlohmann::json recentCandles = nlohmann::json::array();
        size_t count = std::min((size_t)5, candles.size());
        for (size_t i = candles.size() - count; i < candles.size(); ++i) {
            const auto& c = candles[i];
            recentCandles.push_back({
                {"t", c.timestamp},
                {"o", c.open},
                {"h", c.high},
                {"l", c.low},
                {"c", c.close},
                {"v", c.volume}
            });
        }
        j["recentCandles"] = recentCandles;
    }

    return j;
}

std::string AIContextBuilder::getSystemPrompt() {
    return "You are CRYPTØ AI, a production-grade cryptocurrency quantitative research assistant and market intelligence copilot.\n\n"
           "CRITICAL PRINCIPLES:\n"
           "1. Ground all analysis strictly on the supplied real-time market data and calculated technical indicators.\n"
           "2. Clearly separate observed factual data from quantitative interpretation and potential scenarios.\n"
           "3. Never guarantee profits, never claim certainty regarding future market directions, and avoid hyperbolic language.\n"
           "4. Explicitly discuss downside risk, volatility, stop-loss invalidation levels, and liquidity considerations.\n"
           "5. Format responses cleanly using GitHub-flavored Markdown, bullet points, and bold metrics where appropriate.\n"
           "6. Structure analysis into: (1) Observed Market Structure, (2) Technical Momentum & Indicators, (3) Bull/Bear Scenarios & Key Levels, and (4) Risk Assessment.";
}

std::string AIContextBuilder::buildAnalysisPrompt(const std::string& symbol, const std::string& customQuery) {
    if (!customQuery.empty()) {
        return customQuery;
    }
    return "Provide a comprehensive technical analysis for " + symbol + 
           ". Evaluate the current trend structure, RSI momentum, moving average alignments, MACD status, and key support/resistance zones.";
}

std::string AIContextBuilder::buildRiskPrompt(const std::string& symbol, double portfolioValue) {
    std::stringstream ss;
    ss << "Analyze the risk profile of entering a position in " << symbol 
       << " given a portfolio equity of $" << std::fixed << std::setprecision(2) << portfolioValue 
       << ". Recommend appropriate position sizing rules (1-2% max risk), invalidation levels using ATR, and volatility warnings.";
    return ss.str();
}

std::string AIContextBuilder::buildComparisonPrompt(const std::string& symbol1, const std::string& symbol2) {
    return "Compare the technical and market structure of " + symbol1 + " versus " + symbol2 + 
           ". Which asset exhibits stronger relative momentum, cleaner trend structure, and more favorable risk-reward conditions right now?";
}

} // namespace crypto
