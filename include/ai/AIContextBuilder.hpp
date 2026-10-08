#pragma once

#include "core/Types.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace crypto {

class AIContextBuilder {
public:
    static nlohmann::json buildMarketContext(const std::string& symbol,
                                             const std::string& timeframe,
                                             const Ticker& ticker,
                                             const std::vector<Candle>& candles,
                                             const TechnicalSummary& techSummary);

    static std::string getSystemPrompt();
    static std::string buildAnalysisPrompt(const std::string& symbol, const std::string& customQuery = "");
    static std::string buildRiskPrompt(const std::string& symbol, double portfolioValue);
    static std::string buildComparisonPrompt(const std::string& symbol1, const std::string& symbol2);
};

} // namespace crypto
