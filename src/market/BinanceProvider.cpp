#include "market/BinanceProvider.hpp"
#include "core/Logger.hpp"
#include "core/Config.hpp"
#include "core/SystemMetrics.hpp"
#include <algorithm>
#include <sstream>

namespace crypto {

BinanceProvider::BinanceProvider() {
    m_subscribedSymbols = {"BTCUSDT", "ETHUSDT", "SOLUSDT", "BNBUSDT", "XRPUSDT", "DOGEUSDT", "ADAUSDT"};

    m_wsClient.setOnMessage([this](const std::string& msg) {
        this->handleWebSocketMessage(msg);
    });

    m_wsClient.setOnStateChanged([](ConnectionState state) {
        bool isConn = (state == ConnectionState::CONNECTED);
        SystemMetricsCollector::instance().setWsConnected(isConn);
        LOG_INFO("Binance WS State: " + std::to_string(static_cast<int>(state)));
    });
}

BinanceProvider::~BinanceProvider() {
    disconnect();
}

bool BinanceProvider::connect() {
    rebuildWebSocketStreamUrl();
    m_wsClient.connect();
    return true;
}

void BinanceProvider::disconnect() {
    m_wsClient.disconnect();
}

bool BinanceProvider::isConnected() const {
    return m_wsClient.isConnected();
}

std::string BinanceProvider::normalizeSymbol(const std::string& symbol) const {
    std::string s = symbol;
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);
    if (s.find("USDT") == std::string::npos && s.find("BUSD") == std::string::npos && s.find("BTC") == std::string::npos) {
        s += "USDT";
    }
    return s;
}

std::string BinanceProvider::intervalToBinance(const std::string& interval) const {
    std::string i = interval;
    std::transform(i.begin(), i.end(), i.begin(), ::tolower);
    if (i == "1d") return "1d";
    if (i == "1w") return "1w";
    return i;
}

std::vector<Candle> BinanceProvider::getHistoricalKlines(const std::string& symbol, const std::string& interval, int limit) {
    std::string normSym = normalizeSymbol(symbol);
    std::string normInt = intervalToBinance(interval);
    std::string url = Config::instance().getBinanceRestUrl() + "/api/v3/klines?symbol=" + normSym + 
                      "&interval=" + normInt + "&limit=" + std::to_string(limit);

    auto resp = HttpClient::instance().get(url);
    SystemMetricsCollector::instance().setRestLatency(resp.latencyMs);

    std::vector<Candle> candles;
    if (!resp.isSuccess()) {
        LOG_WARN("Failed to fetch klines from Binance for " + normSym + ": " + resp.error);
        return candles;
    }

    auto jsonOpt = resp.asJson();
    if (!jsonOpt || !jsonOpt->is_array()) {
        LOG_WARN("Malformed klines JSON for " + normSym);
        return candles;
    }

    for (const auto& item : *jsonOpt) {
        if (!item.is_array() || item.size() < 9) continue;
        Candle c;
        c.timestamp = item[0].get<int64_t>();
        c.open = std::stod(item[1].get<std::string>());
        c.high = std::stod(item[2].get<std::string>());
        c.low = std::stod(item[3].get<std::string>());
        c.close = std::stod(item[4].get<std::string>());
        c.volume = std::stod(item[5].get<std::string>());
        c.quoteVolume = std::stod(item[7].get<std::string>());
        c.tradesCount = item[8].get<int64_t>();
        candles.push_back(c);
    }

    LOG_DEBUG("Fetched " + std::to_string(candles.size()) + " klines for " + normSym);
    return candles;
}

Ticker BinanceProvider::getTicker(const std::string& symbol) {
    std::string normSym = normalizeSymbol(symbol);
    std::string url = Config::instance().getBinanceRestUrl() + "/api/v3/ticker/24hr?symbol=" + normSym;

    auto resp = HttpClient::instance().get(url);
    SystemMetricsCollector::instance().setRestLatency(resp.latencyMs);

    Ticker t;
    t.symbol = normSym;
    if (!resp.isSuccess()) {
        LOG_WARN("Failed to fetch ticker for " + normSym + ": " + resp.error);
        return t;
    }

    auto jsonOpt = resp.asJson();
    if (!jsonOpt || !jsonOpt->is_object()) return t;

    const auto& j = *jsonOpt;
    try {
        if (j.contains("lastPrice")) t.lastPrice = std::stod(j["lastPrice"].get<std::string>());
        if (j.contains("priceChange")) t.priceChange24h = std::stod(j["priceChange"].get<std::string>());
        if (j.contains("priceChangePercent")) t.priceChangePercent24h = std::stod(j["priceChangePercent"].get<std::string>());
        if (j.contains("highPrice")) t.high24h = std::stod(j["highPrice"].get<std::string>());
        if (j.contains("lowPrice")) t.low24h = std::stod(j["lowPrice"].get<std::string>());
        if (j.contains("volume")) t.volume24h = std::stod(j["volume"].get<std::string>());
        if (j.contains("quoteVolume")) t.quoteVolume24h = std::stod(j["quoteVolume"].get<std::string>());
        if (j.contains("closeTime")) t.timestamp = j["closeTime"].get<int64_t>();
    } catch (const std::exception& e) {
        LOG_WARN("Ticker parsing exception for " + normSym + ": " + std::string(e.what()));
    }

    return t;
}

std::vector<Ticker> BinanceProvider::getAllTickers() {
    std::string url = Config::instance().getBinanceRestUrl() + "/api/v3/ticker/24hr";
    auto resp = HttpClient::instance().get(url);
    SystemMetricsCollector::instance().setRestLatency(resp.latencyMs);

    std::vector<Ticker> tickers;
    if (!resp.isSuccess()) {
        LOG_WARN("Failed to fetch all tickers from Binance: " + resp.error);
        return tickers;
    }

    auto jsonOpt = resp.asJson();
    if (!jsonOpt || !jsonOpt->is_array()) return tickers;

    for (const auto& j : *jsonOpt) {
        if (!j.is_object()) continue;
        try {
            std::string sym = j.value("symbol", "");
            // Filter primarily for USDT pairs to keep relevant
            if (sym.size() <= 4 || sym.substr(sym.size() - 4) != "USDT") continue;

            Ticker t;
            t.symbol = sym;
            t.lastPrice = std::stod(j.value("lastPrice", "0.0"));
            t.priceChange24h = std::stod(j.value("priceChange", "0.0"));
            t.priceChangePercent24h = std::stod(j.value("priceChangePercent", "0.0"));
            t.high24h = std::stod(j.value("highPrice", "0.0"));
            t.low24h = std::stod(j.value("lowPrice", "0.0"));
            t.volume24h = std::stod(j.value("volume", "0.0"));
            t.quoteVolume24h = std::stod(j.value("quoteVolume", "0.0"));
            t.timestamp = j.value("closeTime", (int64_t)0);
            tickers.push_back(t);
        } catch (...) {}
    }

    return tickers;
}

OrderBook BinanceProvider::getOrderBook(const std::string& symbol, int limit) {
    std::string normSym = normalizeSymbol(symbol);
    std::string url = Config::instance().getBinanceRestUrl() + "/api/v3/depth?symbol=" + normSym + "&limit=" + std::to_string(limit);

    auto resp = HttpClient::instance().get(url);
    OrderBook ob;
    ob.symbol = normSym;
    if (!resp.isSuccess()) return ob;

    auto jsonOpt = resp.asJson();
    if (!jsonOpt || !jsonOpt->is_object()) return ob;

    const auto& j = *jsonOpt;
    ob.lastUpdateId = j.value("lastUpdateId", (int64_t)0);

    if (j.contains("bids") && j["bids"].is_array()) {
        for (const auto& b : j["bids"]) {
            if (b.is_array() && b.size() >= 2) {
                ob.bids.push_back({std::stod(b[0].get<std::string>()), std::stod(b[1].get<std::string>())});
            }
        }
    }

    if (j.contains("asks") && j["asks"].is_array()) {
        for (const auto& a : j["asks"]) {
            if (a.is_array() && a.size() >= 2) {
                ob.asks.push_back({std::stod(a[0].get<std::string>()), std::stod(a[1].get<std::string>())});
            }
        }
    }

    return ob;
}

std::vector<Trade> BinanceProvider::getRecentTrades(const std::string& symbol, int limit) {
    std::string normSym = normalizeSymbol(symbol);
    std::string url = Config::instance().getBinanceRestUrl() + "/api/v3/trades?symbol=" + normSym + "&limit=" + std::to_string(limit);

    auto resp = HttpClient::instance().get(url);
    std::vector<Trade> trades;
    if (!resp.isSuccess()) return trades;

    auto jsonOpt = resp.asJson();
    if (!jsonOpt || !jsonOpt->is_array()) return trades;

    for (const auto& j : *jsonOpt) {
        Trade t;
        t.symbol = normSym;
        t.id = j.value("id", (int64_t)0);
        t.price = std::stod(j.value("price", "0.0"));
        t.quantity = std::stod(j.value("qty", "0.0"));
        t.timestamp = j.value("time", (int64_t)0);
        t.isBuyerMaker = j.value("isBuyerMaker", false);
        trades.push_back(t);
    }
    return trades;
}

void BinanceProvider::subscribeSymbols(const std::vector<std::string>& symbols) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& s : symbols) {
        m_subscribedSymbols.insert(normalizeSymbol(s));
    }
    rebuildWebSocketStreamUrl();
    m_wsClient.connect();
}

void BinanceProvider::rebuildWebSocketStreamUrl() {
    // Construct multi-stream URL
    // Format: wss://stream.binance.com:9443/stream?streams=btcusdt@ticker/ethusdt@ticker/...
    std::stringstream ss;
    ss << Config::instance().getBinanceWsUrl() << "/stream?streams=";

    bool first = true;
    for (const auto& sym : m_subscribedSymbols) {
        std::string lowerSym = sym;
        std::transform(lowerSym.begin(), lowerSym.end(), lowerSym.begin(), ::tolower);
        if (!first) ss << "/";
        ss << lowerSym << "@ticker";
        first = false;
    }

    // Also subscribe to chart kline and depth for active symbol
    std::string activeLower = m_activeChartSymbol;
    std::transform(activeLower.begin(), activeLower.end(), activeLower.begin(), ::tolower);
    ss << "/" << activeLower << "@kline_" << intervalToBinance(m_activeChartInterval);
    ss << "/" << activeLower << "@depth20@100ms";

    m_wsClient.setUrl(ss.str());
}

void BinanceProvider::handleWebSocketMessage(const std::string& payload) {
    try {
        auto j = nlohmann::json::parse(payload);
        if (!j.is_object()) return;

        nlohmann::json data = j.contains("data") ? j["data"] : j;
        std::string stream = j.value("stream", "");

        // 1. 24hr Ticker Stream
        if (data.contains("e") && data["e"] == "24hrTicker") {
            Ticker t;
            t.symbol = data.value("s", "");
            t.lastPrice = std::stod(data.value("c", "0.0"));
            t.priceChange24h = std::stod(data.value("p", "0.0"));
            t.priceChangePercent24h = std::stod(data.value("P", "0.0"));
            t.high24h = std::stod(data.value("h", "0.0"));
            t.low24h = std::stod(data.value("l", "0.0"));
            t.volume24h = std::stod(data.value("v", "0.0"));
            t.quoteVolume24h = std::stod(data.value("q", "0.0"));
            t.timestamp = data.value("E", (int64_t)0);

            std::function<void(const Ticker&)> cb;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                cb = m_onTickerUpdate;
            }
            if (cb) cb(t);
        }
        // 2. Kline Stream
        else if (data.contains("e") && data["e"] == "kline" && data.contains("k")) {
            const auto& k = data["k"];
            std::string sym = data.value("s", "");
            std::string interval = k.value("i", "1h");

            Candle c;
            c.timestamp = k.value("t", (int64_t)0);
            c.open = std::stod(k.value("o", "0.0"));
            c.high = std::stod(k.value("h", "0.0"));
            c.low = std::stod(k.value("l", "0.0"));
            c.close = std::stod(k.value("c", "0.0"));
            c.volume = std::stod(k.value("v", "0.0"));
            c.quoteVolume = std::stod(k.value("q", "0.0"));
            c.tradesCount = k.value("n", (int64_t)0);

            std::function<void(const std::string&, const std::string&, const Candle&)> cb;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                cb = m_onCandleUpdate;
            }
            if (cb) cb(sym, interval, c);
        }
        // 3. Depth Stream
        else if (data.contains("bids") && data.contains("asks")) {
            OrderBook ob;
            ob.symbol = m_activeChartSymbol;
            ob.lastUpdateId = data.value("lastUpdateId", (int64_t)0);
            for (const auto& b : data["bids"]) {
                if (b.is_array() && b.size() >= 2) {
                    ob.bids.push_back({std::stod(b[0].get<std::string>()), std::stod(b[1].get<std::string>())});
                }
            }
            for (const auto& a : data["asks"]) {
                if (a.is_array() && a.size() >= 2) {
                    ob.asks.push_back({std::stod(a[0].get<std::string>()), std::stod(a[1].get<std::string>())});
                }
            }

            std::function<void(const OrderBook&)> cb;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                cb = m_onOrderBookUpdate;
            }
            if (cb) cb(ob);
        }
    } catch (...) {}
}

void BinanceProvider::setOnTickerUpdate(std::function<void(const Ticker&)> callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_onTickerUpdate = callback;
}

void BinanceProvider::setOnCandleUpdate(std::function<void(const std::string&, const std::string&, const Candle&)> callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_onCandleUpdate = callback;
}

void BinanceProvider::setOnOrderBookUpdate(std::function<void(const OrderBook&)> callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_onOrderBookUpdate = callback;
}

} // namespace crypto
