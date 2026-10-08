#include "market/MarketManager.hpp"
#include "core/Logger.hpp"
#include "core/Config.hpp"
#include "core/ThreadPool.hpp"
#include "core/EventBus.hpp"

namespace crypto {

MarketManager& MarketManager::instance() {
    static MarketManager s_instance;
    return s_instance;
}

MarketManager::MarketManager() {
    m_supportedSymbols = {
        "BTCUSDT", "ETHUSDT", "SOLUSDT", "BNBUSDT", "XRPUSDT", "DOGEUSDT", 
        "ADAUSDT", "AVAXUSDT", "LINKUSDT", "DOTUSDT", "NEARUSDT", "MATICUSDT"
    };
    m_supportedTimeframes = {"1m", "5m", "15m", "30m", "1h", "4h", "1D", "1W"};
    m_activeSymbol = Config::instance().getDefaultSymbol();
    m_activeTimeframe = Config::instance().getDefaultTimeframe();

    m_binanceProvider = std::make_unique<BinanceProvider>();
    m_demoProvider = std::make_unique<DemoMarketProvider>();
}

void MarketManager::init() {
    bool demo = Config::instance().isDemoMode();
    if (demo) {
        m_currentProvider = m_demoProvider.get();
    } else {
        m_currentProvider = m_binanceProvider.get();
    }

    setupProviderCallbacks(m_currentProvider);
    m_currentProvider->connect();
    m_currentProvider->subscribeSymbols(m_supportedSymbols);

    LOG_INFO("MarketManager initialized with provider: " + m_currentProvider->getProviderName());

    // Initial load
    refreshDataAsync();
}

void MarketManager::shutdown() {
    if (m_binanceProvider) m_binanceProvider->disconnect();
    if (m_demoProvider) m_demoProvider->disconnect();
    LOG_INFO("MarketManager shutdown cleanly.");
}

void MarketManager::setDemoMode(bool demo) {
    if (isDemoMode() == demo) return;

    LOG_INFO("Switching market data provider mode to: " + std::string(demo ? "DEMO" : "LIVE BINANCE"));
    if (m_currentProvider) m_currentProvider->disconnect();

    Config::instance().setDemoMode(demo);
    m_currentProvider = demo ? static_cast<IMarketDataProvider*>(m_demoProvider.get()) 
                             : static_cast<IMarketDataProvider*>(m_binanceProvider.get());

    setupProviderCallbacks(m_currentProvider);
    m_currentProvider->connect();
    m_currentProvider->subscribeSymbols(m_supportedSymbols);

    refreshDataAsync();
}

bool MarketManager::isDemoMode() const {
    return Config::instance().isDemoMode();
}

void MarketManager::setupProviderCallbacks(IMarketDataProvider* provider) {
    if (!provider) return;

    provider->setOnTickerUpdate([this](const Ticker& t) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_tickerMap[t.symbol] = t;
            for (auto& item : m_allTickers) {
                if (item.symbol == t.symbol) {
                    item = t;
                    break;
                }
            }
        }
        EventBus::instance().publish(TickerUpdatedEvent{t.symbol, t.lastPrice, t.priceChangePercent24h, t.volume24h});
    });

    provider->setOnCandleUpdate([this](const std::string& symbol, const std::string& interval, const Candle& c) {
        if (symbol == m_activeSymbol && interval == m_activeTimeframe) {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!m_activeCandles.empty()) {
                if (m_activeCandles.back().timestamp == c.timestamp) {
                    m_activeCandles.back() = c;
                } else if (c.timestamp > m_activeCandles.back().timestamp) {
                    m_activeCandles.push_back(c);
                    if (m_activeCandles.size() > 500) {
                        m_activeCandles.erase(m_activeCandles.begin());
                    }
                }
            } else {
                m_activeCandles.push_back(c);
            }
        }
        EventBus::instance().publish(CandleUpdatedEvent{symbol, interval, c});
    });

    provider->setOnOrderBookUpdate([this](const OrderBook& ob) {
        if (ob.symbol == m_activeSymbol) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_currentOrderBook = ob;
        }
    });
}

void MarketManager::setActiveSymbol(const std::string& symbol) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_activeSymbol == symbol) return;
        m_activeSymbol = symbol;
    }
    LOG_INFO("Active market symbol changed to: " + symbol);
    fetchHistoricalKlinesAsync(symbol, m_activeTimeframe);
    if (m_currentProvider) {
        m_currentProvider->subscribeSymbols({symbol});
    }
}

std::string MarketManager::getActiveSymbol() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_activeSymbol;
}

void MarketManager::setActiveTimeframe(const std::string& interval) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_activeTimeframe == interval) return;
        m_activeTimeframe = interval;
    }
    LOG_INFO("Active timeframe changed to: " + interval);
    fetchHistoricalKlinesAsync(m_activeSymbol, interval);
}

std::string MarketManager::getActiveTimeframe() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_activeTimeframe;
}

std::vector<Candle> MarketManager::getActiveCandles() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_activeCandles;
}

std::vector<Candle> MarketManager::getCandles(const std::string& symbol, const std::string& interval) const {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (symbol == m_activeSymbol && interval == m_activeTimeframe && !m_activeCandles.empty()) {
            return m_activeCandles;
        }
    }
    if (m_currentProvider) {
        auto klines = m_currentProvider->getHistoricalKlines(symbol, interval, 100);
        if (!klines.empty()) return klines;
    }
    if (m_demoProvider) {
        return m_demoProvider->getHistoricalKlines(symbol, interval, 100);
    }
    return {};
}

Ticker MarketManager::getTicker(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_tickerMap.find(symbol);
    if (it != m_tickerMap.end()) return it->second;
    return {};
}

std::vector<Ticker> MarketManager::getAllTickers() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_allTickers;
}

OrderBook MarketManager::getOrderBook() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentOrderBook;
}

std::vector<Trade> MarketManager::getRecentTrades() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_recentTrades;
}

void MarketManager::refreshDataAsync() {
    fetchAllTickersAsync();
    fetchHistoricalKlinesAsync(m_activeSymbol, m_activeTimeframe);
}

void MarketManager::fetchHistoricalKlinesAsync(const std::string& symbol, const std::string& interval) {
    ThreadPool::instance().enqueue([this, symbol, interval]() {
        if (!m_currentProvider) return;
        auto klines = m_currentProvider->getHistoricalKlines(symbol, interval, 200);
        auto ob = m_currentProvider->getOrderBook(symbol, 20);
        auto trades = m_currentProvider->getRecentTrades(symbol, 30);
        auto ticker = m_currentProvider->getTicker(symbol);

        ThreadPool::instance().postToMainThread([this, symbol, interval, klines, ob, trades, ticker]() {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (symbol == m_activeSymbol && interval == m_activeTimeframe) {
                m_activeCandles = klines;
                m_currentOrderBook = ob;
                m_recentTrades = trades;
            }
            m_tickerMap[symbol] = ticker;
        });
    });
}

void MarketManager::fetchAllTickersAsync() {
    ThreadPool::instance().enqueue([this]() {
        if (!m_currentProvider) return;
        auto tickers = m_currentProvider->getAllTickers();
        ThreadPool::instance().postToMainThread([this, tickers]() {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_allTickers = tickers;
            for (const auto& t : tickers) {
                m_tickerMap[t.symbol] = t;
            }
        });
    });
}

std::vector<std::string> MarketManager::getSupportedSymbols() const {
    return m_supportedSymbols;
}

std::vector<std::string> MarketManager::getSupportedTimeframes() const {
    return m_supportedTimeframes;
}

IMarketDataProvider* MarketManager::getActiveProvider() const {
    return m_currentProvider;
}

} // namespace crypto
