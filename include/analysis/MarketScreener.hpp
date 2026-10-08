#pragma once

#include "core/Types.hpp"
#include <vector>
#include <string>

namespace crypto {

struct ScreenerFilter {
    double minChange24h{-100.0};
    double maxChange24h{100.0};
    double minVolumeUsd{0.0};
    double minRsi{0.0};
    double maxRsi{100.0};
    std::string trendFilter{"ALL"}; // "ALL", "BULLISH", "BEARISH"
    std::string macdFilter{"ALL"};  // "ALL", "BULLISH_CROSS", "BEARISH_CROSS"
};

struct ScreenerResultItem {
    std::string symbol;
    double price{0.0};
    double change24h{0.0};
    double volume24h{0.0};
    double rsi{50.0};
    std::string trend{"NEUTRAL"};
    std::string macdSignal{"NONE"};
    double score{0.0};
    std::string matchReason;
};

class MarketScreener {
public:
    static std::vector<ScreenerResultItem> runScreen(
        const std::vector<Ticker>& tickers,
        const ScreenerFilter& filter);
};

} // namespace crypto
