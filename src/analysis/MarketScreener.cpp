#include "analysis/MarketScreener.hpp"
#include <algorithm>

namespace crypto {

std::vector<ScreenerResultItem> MarketScreener::runScreen(
    const std::vector<Ticker>& tickers,
    const ScreenerFilter& filter) {

    std::vector<ScreenerResultItem> results;

    for (const auto& t : tickers) {
        if (t.lastPrice <= 0.0) continue;

        // 1. 24h Change filter
        if (t.priceChangePercent24h < filter.minChange24h || t.priceChangePercent24h > filter.maxChange24h) {
            continue;
        }

        // 2. Volume filter
        if (t.quoteVolume24h < filter.minVolumeUsd && t.volume24h < filter.minVolumeUsd) {
            continue;
        }

        // Simulated/derived RSI and trend from 24h stats for screener universe
        double approxRsi = 50.0 + (t.priceChangePercent24h * 2.5);
        approxRsi = std::clamp(approxRsi, 10.0, 90.0);

        if (approxRsi < filter.minRsi || approxRsi > filter.maxRsi) {
            continue;
        }

        std::string trend = (t.priceChangePercent24h > 1.5) ? "BULLISH" : (t.priceChangePercent24h < -1.5 ? "BEARISH" : "NEUTRAL");
        if (filter.trendFilter != "ALL" && trend != filter.trendFilter) {
            continue;
        }

        std::string macdSig = (t.priceChangePercent24h > 3.0) ? "BULLISH_CROSS" : (t.priceChangePercent24h < -3.0 ? "BEARISH_CROSS" : "NONE");
        if (filter.macdFilter != "ALL" && macdSig != filter.macdFilter) {
            continue;
        }

        ScreenerResultItem item;
        item.symbol = t.symbol;
        item.price = t.lastPrice;
        item.change24h = t.priceChangePercent24h;
        item.volume24h = (t.quoteVolume24h > 0) ? t.quoteVolume24h : t.volume24h;
        item.rsi = approxRsi;
        item.trend = trend;
        item.macdSignal = macdSig;

        // Calculate quantitative score (0-100)
        double score = 50.0;
        if (trend == "BULLISH") score += 20.0;
        if (trend == "BEARISH") score -= 20.0;
        if (approxRsi <= 35.0) score += 15.0; // Oversold discount
        if (macdSig == "BULLISH_CROSS") score += 15.0;
        item.score = std::clamp(score, 0.0, 100.0);

        std::string reason = std::string("Momentum ") + (t.priceChangePercent24h >= 0 ? "+" : "") + std::to_string(t.priceChangePercent24h).substr(0, 5) + "%";
        if (approxRsi <= 30.0) reason += " | RSI Oversold";
        if (approxRsi >= 70.0) reason += " | RSI Overbought";
        item.matchReason = reason;

        results.push_back(item);
    }

    // Sort by absolute 24h change descending
    std::sort(results.begin(), results.end(), [](const ScreenerResultItem& a, const ScreenerResultItem& b) {
        return std::abs(a.change24h) > std::abs(b.change24h);
    });

    return results;
}

} // namespace crypto
