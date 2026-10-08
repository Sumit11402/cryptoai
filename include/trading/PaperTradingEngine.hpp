#pragma once

#include "trading/ITradingProvider.hpp"
#include "core/Types.hpp"
#include <vector>
#include <unordered_map>
#include <string>
#include <mutex>

namespace crypto {

struct PaperAccountState {
    double initialBalance{10000.0};
    double cashBalance{10000.0};
    double totalPortfolioValue{10000.0};
    double totalRealizedPnL{0.0};
    double totalUnrealizedPnL{0.0};
    double totalFeesPaid{0.0};
    double usedMargin{0.0};
    double freeMargin{10000.0};
    double marginLevelPercent{999.0};
    int totalTradesCount{0};
    int winningTradesCount{0};
};

class PaperTradingEngine : public ITradingProvider {
public:
    static PaperTradingEngine& instance();

    void init(double initialBalance = 10000.0);
    void reset(double newBalance = 10000.0);

    // Orders & Positions
    bool executeMarketOrder(
        const std::string& symbol, 
        OrderSide side, 
        double quantity, 
        double currentMarketPrice, 
        std::string& error,
        double leverage = 1.0,
        double stopLoss = 0.0,
        double takeProfit = 0.0,
        double trailingStopPct = 0.0);

    bool placeLimitOrder(
        const std::string& symbol, 
        OrderSide side, 
        double quantity, 
        double limitPrice, 
        std::string& error,
        double leverage = 1.0,
        double stopLoss = 0.0,
        double takeProfit = 0.0);

    bool placeOrder(const PaperOrder& order, std::string& outOrderId, std::string& error) override;
    bool cancelOrder(const std::string& orderId) override;

    // Advanced Position Management
    bool closePosition(const std::string& symbol, double currentMarketPrice, std::string& error);
    bool reversePosition(const std::string& symbol, double currentMarketPrice, std::string& error);

    std::vector<PaperOrder> getOpenOrders() override;
    std::vector<PaperPosition> getPositions() override;
    std::vector<PaperOrder> getOrderHistory(int limit = 50);

    double getAvailableBalance() const override;
    PaperAccountState getAccountState();

    void checkLimitOrders(const std::unordered_map<std::string, double>& currentPrices);
    void updatePositionPrices(const std::unordered_map<std::string, double>& currentPrices);

    // Quantitative Risk & Position Sizing Tools
    double calculateRecommendedPositionSize(double riskPercent, double entryPrice, double stopLossPrice);
    PortfolioRiskAnalytics getRiskAnalytics(const std::unordered_map<std::string, double>& currentPrices);

private:
    PaperTradingEngine() = default;
    ~PaperTradingEngine() override = default;

    std::string generateOrderId();
    void recalculateAccountState();

    mutable std::mutex m_mutex;
    PaperAccountState m_state;
    std::vector<PaperOrder> m_openOrders;
    std::vector<PaperOrder> m_orderHistory;
    std::unordered_map<std::string, PaperPosition> m_positions;

    const double m_takerFeeRate{0.001}; // 0.1% fee
    const double m_makerFeeRate{0.0005}; // 0.05% fee
    const double m_simSlippage{0.0002}; // 0.02% simulated slippage
};

} // namespace crypto

