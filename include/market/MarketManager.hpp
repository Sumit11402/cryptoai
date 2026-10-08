#pragma once

#include "market/IMarketDataProvider.hpp"
#include "market/BinanceProvider.hpp"
#include "market/DemoMarketProvider.hpp"
#include "core/Types.hpp"
#include <memory>
#include <mutex>
#include <unordered_map>
#include <string>
#include <vector>

namespace crypto {

class MarketManager {
public:
    static MarketManager& instance();

    void init();
    void shutdown();

    void setDemoMode(bool demo);
    bool isDemoMode() const;

    void setActiveSymbol(const std::string& symbol);
    std::string getActiveSymbol() const;

    void setActiveTimeframe(const std::string& interval);
    std::string getActiveTimeframe() const;

    // Data Accessors
    std::vector<Candle> getActiveCandles() const;
    std::vector<Candle> getCandles(const std::string& symbol, const std::string& interval) const;
    Ticker getTicker(const std::string& symbol) const;
    std::vector<Ticker> getAllTickers() const;
    OrderBook getOrderBook() const;
    std::vector<Trade> getRecentTrades() const;

    // Asynchronous Fetchers
    void refreshDataAsync();
    void fetchHistoricalKlinesAsync(const std::string& symbol, const std::string& interval);
    void fetchAllTickersAsync();

    // Available symbols and intervals
    std::vector<std::string> getSupportedSymbols() const;
    std::vector<std::string> getSupportedTimeframes() const;

    IMarketDataProvider* getActiveProvider() const;

private:
    MarketManager();
    ~MarketManager() = default;

    void setupProviderCallbacks(IMarketDataProvider* provider);

    std::unique_ptr<BinanceProvider> m_binanceProvider;
    std::unique_ptr<DemoMarketProvider> m_demoProvider;
    IMarketDataProvider* m_currentProvider{nullptr};

    std::string m_activeSymbol{"BTCUSDT"};
    std::string m_activeTimeframe{"1h"};

    mutable std::mutex m_mutex;
    std::vector<Candle> m_activeCandles;
    std::unordered_map<std::string, Ticker> m_tickerMap;
    std::vector<Ticker> m_allTickers;
    OrderBook m_currentOrderBook;
    std::vector<Trade> m_recentTrades;

    std::vector<std::string> m_supportedSymbols;
    std::vector<std::string> m_supportedTimeframes;
};

} // namespace crypto
