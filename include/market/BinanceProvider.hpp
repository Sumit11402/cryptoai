#pragma once

#include "market/IMarketDataProvider.hpp"
#include "network/HttpClient.hpp"
#include "network/WebSocketClient.hpp"
#include <mutex>
#include <unordered_map>
#include <unordered_set>

namespace crypto {

class BinanceProvider : public IMarketDataProvider {
public:
    BinanceProvider();
    ~BinanceProvider() override;

    bool connect() override;
    void disconnect() override;
    bool isConnected() const override;

    std::vector<Candle> getHistoricalKlines(const std::string& symbol, const std::string& interval, int limit = 100) override;
    Ticker getTicker(const std::string& symbol) override;
    std::vector<Ticker> getAllTickers() override;
    OrderBook getOrderBook(const std::string& symbol, int limit = 20) override;
    std::vector<Trade> getRecentTrades(const std::string& symbol, int limit = 30) override;

    void subscribeSymbols(const std::vector<std::string>& symbols) override;
    void setOnTickerUpdate(std::function<void(const Ticker&)> callback) override;
    void setOnCandleUpdate(std::function<void(const std::string& symbol, const std::string& interval, const Candle&)> callback) override;
    void setOnOrderBookUpdate(std::function<void(const OrderBook&)> callback) override;

    std::string getProviderName() const override { return "Binance Public Market Data"; }

private:
    void handleWebSocketMessage(const std::string& payload);
    void rebuildWebSocketStreamUrl();

    std::string normalizeSymbol(const std::string& symbol) const;
    std::string intervalToBinance(const std::string& interval) const;

    WebSocketClient m_wsClient;
    std::unordered_set<std::string> m_subscribedSymbols;
    std::string m_activeChartSymbol{"BTCUSDT"};
    std::string m_activeChartInterval{"1h"};

    std::function<void(const Ticker&)> m_onTickerUpdate;
    std::function<void(const std::string&, const std::string&, const Candle&)> m_onCandleUpdate;
    std::function<void(const OrderBook&)> m_onOrderBookUpdate;
    mutable std::mutex m_mutex;
};

} // namespace crypto
