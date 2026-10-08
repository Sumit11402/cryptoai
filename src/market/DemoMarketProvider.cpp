#include "market/DemoMarketProvider.hpp"
#include "core/Logger.hpp"
#include <chrono>

namespace crypto {

DemoMarketProvider::DemoMarketProvider() : m_rng(42) {
    m_basePrices = {
        {"BTCUSDT", 96540.0},
        {"ETHUSDT", 2780.0},
        {"SOLUSDT", 188.5},
        {"BNBUSDT", 645.0},
        {"XRPUSDT", 2.45},
        {"DOGEUSDT", 0.285},
        {"ADAUSDT", 0.78}
    };
    generateInitialData();
}

DemoMarketProvider::~DemoMarketProvider() {
    disconnect();
}

void DemoMarketProvider::generateInitialData() {
    int64_t nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    std::normal_distribution<double> retDist(0.0001, 0.008);

    for (const auto& [sym, basePrice] : m_basePrices) {
        std::vector<Candle> candles;
        candles.reserve(150);

        double currentPrice = basePrice * 0.92; // start a bit lower
        int64_t stepMs = 3600 * 1000; // 1h candles

        for (int i = 149; i >= 0; --i) {
            int64_t candleTime = nowMs - (i * stepMs);
            double open = currentPrice;
            double shock = retDist(m_rng);
            double close = open * (1.0 + shock);
            double high = std::max(open, close) * (1.0 + std::abs(retDist(m_rng)) * 0.5);
            double low = std::min(open, close) * (1.0 - std::abs(retDist(m_rng)) * 0.5);
            double vol = (basePrice > 1000.0 ? 150.0 : 15000.0) * (0.8 + 0.4 * (m_rng() % 100) / 100.0);

            Candle c;
            c.timestamp = candleTime;
            c.open = open;
            c.high = high;
            c.low = low;
            c.close = close;
            c.volume = vol;
            c.quoteVolume = vol * close;
            c.tradesCount = 1200 + (m_rng() % 500);
            candles.push_back(c);

            currentPrice = close;
        }

        m_candleCache[sym] = candles;

        Ticker t;
        t.symbol = sym;
        t.lastPrice = currentPrice;
        t.priceChange24h = currentPrice - candles[candles.size() - 24].close;
        t.priceChangePercent24h = (t.priceChange24h / candles[candles.size() - 24].close) * 100.0;
        t.high24h = currentPrice * 1.03;
        t.low24h = currentPrice * 0.97;
        t.volume24h = 45000000.0;
        t.quoteVolume24h = 45000000.0 * currentPrice;
        t.timestamp = nowMs;
        m_tickers[sym] = t;
    }
}

bool DemoMarketProvider::connect() {
    if (m_running.load()) return true;
    m_running.store(true);
    m_simThread = std::thread(&DemoMarketProvider::simulationLoop, this);
    LOG_INFO("DemoMarketProvider started simulation loop.");
    return true;
}

void DemoMarketProvider::disconnect() {
    if (!m_running.load()) return;
    m_running.store(false);
    if (m_simThread.joinable()) {
        m_simThread.join();
    }
    LOG_INFO("DemoMarketProvider stopped simulation loop.");
}

void DemoMarketProvider::simulationLoop() {
    std::normal_distribution<double> microDist(0.0, 0.001);

    while (m_running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        if (!m_running.load()) break;

        int64_t nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        for (auto& [sym, ticker] : m_tickers) {
            double delta = microDist(m_rng);
            ticker.lastPrice *= (1.0 + delta);
            ticker.priceChange24h += (ticker.lastPrice * delta);
            ticker.priceChangePercent24h = (ticker.priceChange24h / (ticker.lastPrice - ticker.priceChange24h)) * 100.0;
            ticker.timestamp = nowMs;

            std::function<void(const Ticker&)> tCb;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                tCb = m_onTickerUpdate;
            }
            if (tCb) tCb(ticker);

            // Update latest candle
            std::function<void(const std::string&, const std::string&, const Candle&)> cCb;
            Candle lastC;
            bool hasCandle = false;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                auto& candleVec = m_candleCache[sym];
                if (!candleVec.empty()) {
                    Candle& c = candleVec.back();
                    c.close = ticker.lastPrice;
                    if (ticker.lastPrice > c.high) c.high = ticker.lastPrice;
                    if (ticker.lastPrice < c.low) c.low = ticker.lastPrice;
                    c.volume += 0.5;
                    lastC = c;
                    hasCandle = true;
                }
                cCb = m_onCandleUpdate;
            }
            if (hasCandle && cCb) {
                cCb(sym, "1h", lastC);
            }
        }
    }
}

std::vector<Candle> DemoMarketProvider::getHistoricalKlines(const std::string& symbol, const std::string& /*interval*/, int limit) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_candleCache.find(symbol);
    if (it != m_candleCache.end()) {
        const auto& full = it->second;
        if ((int)full.size() > limit) {
            return std::vector<Candle>(full.end() - limit, full.end());
        }
        return full;
    }
    return {};
}

Ticker DemoMarketProvider::getTicker(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_tickers.find(symbol);
    if (it != m_tickers.end()) return it->second;
    return {};
}

std::vector<Ticker> DemoMarketProvider::getAllTickers() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<Ticker> res;
    for (const auto& [sym, t] : m_tickers) {
        res.push_back(t);
    }
    return res;
}

OrderBook DemoMarketProvider::getOrderBook(const std::string& symbol, int limit) {
    std::lock_guard<std::mutex> lock(m_mutex);
    OrderBook ob;
    ob.symbol = symbol;
    ob.lastUpdateId = 1000;
    double midPrice = 50000.0;
    auto it = m_tickers.find(symbol);
    if (it != m_tickers.end()) midPrice = it->second.lastPrice;

    for (int i = 1; i <= limit; ++i) {
        double bidP = midPrice * (1.0 - (0.0003 * i));
        double askP = midPrice * (1.0 + (0.0003 * i));
        double qty = 0.5 + (i * 0.2);
        ob.bids.push_back({bidP, qty});
        ob.asks.push_back({askP, qty});
    }
    return ob;
}

std::vector<Trade> DemoMarketProvider::getRecentTrades(const std::string& symbol, int limit) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<Trade> trades;
    double midPrice = 50000.0;
    auto it = m_tickers.find(symbol);
    if (it != m_tickers.end()) midPrice = it->second.lastPrice;

    int64_t nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    for (int i = 0; i < limit; ++i) {
        Trade t;
        t.symbol = symbol;
        t.id = 10000 + i;
        t.price = midPrice * (1.0 + (microDistVal(i)));
        t.quantity = 0.1 + (i % 5) * 0.05;
        t.timestamp = nowMs - (i * 1000);
        t.isBuyerMaker = (i % 2 == 0);
        trades.push_back(t);
    }
    return trades;
}

double DemoMarketProvider::microDistVal(int idx) {
    return ((idx % 7) - 3) * 0.0002;
}

void DemoMarketProvider::subscribeSymbols(const std::vector<std::string>& /*symbols*/) {}

void DemoMarketProvider::setOnTickerUpdate(std::function<void(const Ticker&)> callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_onTickerUpdate = callback;
}

void DemoMarketProvider::setOnCandleUpdate(std::function<void(const std::string& symbol, const std::string& interval, const Candle&)> callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_onCandleUpdate = callback;
}

void DemoMarketProvider::setOnOrderBookUpdate(std::function<void(const OrderBook&)> callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_onOrderBookUpdate = callback;
}

} // namespace crypto
