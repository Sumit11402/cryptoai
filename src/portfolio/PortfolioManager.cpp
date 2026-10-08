#include "portfolio/PortfolioManager.hpp"
#include "storage/Database.hpp"
#include "core/Logger.hpp"
#include <algorithm>
#include <chrono>

namespace crypto {

PortfolioManager& PortfolioManager::instance() {
    static PortfolioManager s_instance;
    return s_instance;
}

void PortfolioManager::init() {
    auto dbAssets = Database::instance().getPortfolioAssets();
    if (dbAssets.empty()) {
        // Provide initial default portfolio for demo/first-run
        addOrUpdateAsset("BTCUSDT", 0.35, 62000.0);
        addOrUpdateAsset("ETHUSDT", 3.5, 2450.0);
        addOrUpdateAsset("SOLUSDT", 25.0, 140.0);
    } else {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_assets = dbAssets;
        recalculateMetrics();
    }
    LOG_INFO("PortfolioManager initialized with " + std::to_string(m_assets.size()) + " assets.");
}

void PortfolioManager::addOrUpdateAsset(const std::string& symbol, double quantity, double avgBuyPrice) {
    if (quantity <= 0.0) {
        removeAsset(symbol);
        return;
    }

    int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    PortfolioAsset newAsset;
    newAsset.symbol = symbol;
    newAsset.quantity = quantity;
    newAsset.avgBuyPrice = avgBuyPrice;
    newAsset.currentPrice = avgBuyPrice; // Will update with live price
    newAsset.lastUpdated = now;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        bool found = false;
        for (auto& a : m_assets) {
            if (a.symbol == symbol) {
                a.quantity = quantity;
                a.avgBuyPrice = avgBuyPrice;
                a.lastUpdated = now;
                found = true;
                break;
            }
        }
        if (!found) {
            m_assets.push_back(newAsset);
        }
        recalculateMetrics();
    }

    Database::instance().savePortfolioAsset(newAsset);
    LOG_INFO("Portfolio asset updated: " + symbol + " qty=" + std::to_string(quantity) + " avg=$" + std::to_string(avgBuyPrice));
}

void PortfolioManager::removeAsset(const std::string& symbol) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_assets.erase(std::remove_if(m_assets.begin(), m_assets.end(), 
            [&symbol](const PortfolioAsset& a) { return a.symbol == symbol; }), m_assets.end());
        recalculateMetrics();
    }
    Database::instance().removePortfolioAsset(symbol);
    LOG_INFO("Portfolio asset removed: " + symbol);
}

void PortfolioManager::updatePricesWithTickers(const std::unordered_map<std::string, Ticker>& tickerMap) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& a : m_assets) {
        auto it = tickerMap.find(a.symbol);
        if (it != tickerMap.end() && it->second.lastPrice > 0.0) {
            a.currentPrice = it->second.lastPrice;
        }
    }
    recalculateMetrics();
}

void PortfolioManager::recalculateMetrics() {
    m_summary.totalValueUsd = 0.0;
    m_summary.totalCostBasisUsd = 0.0;
    m_summary.bestPerformerReturn = -1e9;
    m_summary.worstPerformerReturn = 1e9;
    m_summary.bestPerformer = "None";
    m_summary.worstPerformer = "None";

    for (auto& a : m_assets) {
        if (a.currentPrice <= 0.0) a.currentPrice = a.avgBuyPrice;

        a.totalValue = a.quantity * a.currentPrice;
        double cost = a.quantity * a.avgBuyPrice;
        a.unrealizedPnL = a.totalValue - cost;
        a.unrealizedPnLPercent = (cost > 0.0) ? (a.unrealizedPnL / cost * 100.0) : 0.0;

        m_summary.totalValueUsd += a.totalValue;
        m_summary.totalCostBasisUsd += cost;

        if (a.unrealizedPnLPercent > m_summary.bestPerformerReturn) {
            m_summary.bestPerformerReturn = a.unrealizedPnLPercent;
            m_summary.bestPerformer = a.symbol;
        }
        if (a.unrealizedPnLPercent < m_summary.worstPerformerReturn) {
            m_summary.worstPerformerReturn = a.unrealizedPnLPercent;
            m_summary.worstPerformer = a.symbol;
        }
    }

    m_summary.totalUnrealizedPnL = m_summary.totalValueUsd - m_summary.totalCostBasisUsd;
    m_summary.totalUnrealizedPnLPercent = (m_summary.totalCostBasisUsd > 0.0) 
        ? (m_summary.totalUnrealizedPnL / m_summary.totalCostBasisUsd * 100.0) : 0.0;

    // Allocation percentages
    for (auto& a : m_assets) {
        a.allocationPercent = (m_summary.totalValueUsd > 0.0) 
            ? (a.totalValue / m_summary.totalValueUsd * 100.0) : 0.0;
    }
}

std::vector<PortfolioAsset> PortfolioManager::getAssets() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_assets;
}

PortfolioSummary PortfolioManager::getSummary() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_summary;
}

} // namespace crypto
