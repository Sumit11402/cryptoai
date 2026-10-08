#pragma once

#include "core/Types.hpp"
#include <vector>
#include <string>
#include <mutex>
#include <unordered_map>

namespace crypto {

struct PortfolioSummary {
    double totalValueUsd{0.0};
    double totalCostBasisUsd{0.0};
    double totalUnrealizedPnL{0.0};
    double totalUnrealizedPnLPercent{0.0};
    double totalRealizedPnL{0.0};
    double change24hPercent{0.0};
    std::string bestPerformer;
    double bestPerformerReturn{0.0};
    std::string worstPerformer;
    double worstPerformerReturn{0.0};
};

class PortfolioManager {
public:
    static PortfolioManager& instance();

    void init();
    void addOrUpdateAsset(const std::string& symbol, double quantity, double avgBuyPrice);
    void removeAsset(const std::string& symbol);

    std::vector<PortfolioAsset> getAssets();
    PortfolioSummary getSummary();

    void updatePricesWithTickers(const std::unordered_map<std::string, Ticker>& tickerMap);

private:
    PortfolioManager() = default;
    ~PortfolioManager() = default;

    void recalculateMetrics();

    mutable std::mutex m_mutex;
    std::vector<PortfolioAsset> m_assets;
    PortfolioSummary m_summary;
};

} // namespace crypto
