#pragma once

#include "market/IMarketDataProvider.hpp"
#include <mutex>
#include <unordered_map>
#include <random>
#include <thread>
#include <atomic>

namespace crypto {

class DemoMarketProvider : public IMarketDataProvider {
public:
    DemoMarketProvider();
    ~DemoMarketProvider() override;

    bool connect() override;
    void disconnect() override;
    bool isConnected() const override { return m_running.load(); }

    std::vector<Candle> getHistoricalKlines(const std::string& symbol, const std::string& interval, int limit = 100) override;
    Ticker getTicker(const std::string& symbol) override;
    std::vector<Ticker> getAllTickers() override;
    OrderBook getOrderBook(const std::string& symbol, int limit = 20) override;
    std::vector<Trade> getRecentTrades(const std::string& symbol, int limit = 30) override;

    void subscribeSymbols(const std::vector<std::string>& symbols) override;
    void setOnTickerUpdate(std::function<void(const Ticker&)> callback) override;
    void setOnCandleUpdate(std::function<void(const std::string& symbol, const std::string& interval, const Candle&)> callback) override;
    void setOnOrderBookUpdate(std::function<void(const OrderBook&)> callback) override;

    double microDistVal(int idx);
    std::string getProviderName() const override { return "DEMO DATA (Offline Simulation)"; }

private:
    void simulationLoop();
    void generateInitialData();

    std::atomic<bool> m_running{false};
    std::thread m_simThread;

    std::unordered_map<std::string, double> m_basePrices;
    std::unordered_map<std::string, Ticker> m_tickers;
    std::unordered_map<std::string, std::vector<Candle>> m_candleCache;

    std::function<void(const Ticker&)> m_onTickerUpdate;
    std::function<void(const std::string&, const std::string&, const Candle&)> m_onCandleUpdate;
    std::function<void(const OrderBook&)> m_onOrderBookUpdate;

    std::mt19937 m_rng;
    mutable std::mutex m_mutex;
};

} // namespace crypto
