#pragma once

#include "core/Types.hpp"
#include <string>
#include <vector>
#include <functional>

namespace crypto {

class IMarketDataProvider {
public:
    virtual ~IMarketDataProvider() = default;

    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;

    virtual std::vector<Candle> getHistoricalKlines(const std::string& symbol, const std::string& interval, int limit = 100) = 0;
    virtual Ticker getTicker(const std::string& symbol) = 0;
    virtual std::vector<Ticker> getAllTickers() = 0;
    virtual OrderBook getOrderBook(const std::string& symbol, int limit = 20) = 0;
    virtual std::vector<Trade> getRecentTrades(const std::string& symbol, int limit = 30) = 0;

    virtual void subscribeSymbols(const std::vector<std::string>& symbols) = 0;
    virtual void setOnTickerUpdate(std::function<void(const Ticker&)> callback) = 0;
    virtual void setOnCandleUpdate(std::function<void(const std::string& symbol, const std::string& interval, const Candle&)> callback) = 0;
    virtual void setOnOrderBookUpdate(std::function<void(const OrderBook&)> callback) = 0;

    virtual std::string getProviderName() const = 0;
};

} // namespace crypto
