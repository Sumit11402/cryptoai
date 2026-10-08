#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <optional>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace crypto {

// Market & Candlestick Models
struct Candle {
    int64_t timestamp{0}; // milliseconds
    double open{0.0};
    double high{0.0};
    double low{0.0};
    double close{0.0};
    double volume{0.0};
    double quoteVolume{0.0};
    int64_t tradesCount{0};
};

struct Ticker {
    std::string symbol;
    double lastPrice{0.0};
    double priceChange24h{0.0};
    double priceChangePercent24h{0.0};
    double high24h{0.0};
    double low24h{0.0};
    double volume24h{0.0};
    double quoteVolume24h{0.0};
    int64_t timestamp{0};
};

struct OrderBookLevel {
    double price{0.0};
    double quantity{0.0};
};

struct OrderBook {
    std::string symbol;
    int64_t lastUpdateId{0};
    std::vector<OrderBookLevel> bids;
    std::vector<OrderBookLevel> asks;
    int64_t timestamp{0};
};

struct Trade {
    std::string symbol;
    int64_t id{0};
    double price{0.0};
    double quantity{0.0};
    int64_t timestamp{0};
    bool isBuyerMaker{false};
};

// Technical Analysis Models
struct IndicatorSeries {
    std::vector<int64_t> timestamps;
    std::vector<double> values;
};

struct MACDResult {
    std::vector<int64_t> timestamps;
    std::vector<double> macd;
    std::vector<double> signal;
    std::vector<double> histogram;
};

struct BollingerBandsResult {
    std::vector<int64_t> timestamps;
    std::vector<double> upper;
    std::vector<double> middle;
    std::vector<double> lower;
    std::vector<double> bandwidth;
    std::vector<double> percentB;
};

// Advanced Technical Analysis Models
struct StochasticResult {
    std::vector<int64_t> timestamps;
    std::vector<double> k;
    std::vector<double> d;
};

struct SuperTrendResult {
    std::vector<int64_t> timestamps;
    std::vector<double> superTrend;
    std::vector<int> direction; // +1 Bullish (Green), -1 Bearish (Red)
};

struct VWAPResult {
    std::vector<int64_t> timestamps;
    std::vector<double> vwap;
    std::vector<double> upperBand;
    std::vector<double> lowerBand;
};

struct CandlestickPattern {
    std::string name;
    std::string bias; // "BULLISH", "BEARISH", "INDECISION"
    size_t index{0};
    int64_t timestamp{0};
    double price{0.0};
    std::string description;
};

struct CorrelationMatrix {
    std::vector<std::string> symbols;
    std::vector<std::vector<double>> correlations; // NxN matrix (-1.0 to +1.0)
};

struct TechnicalSummary {
    double currentPrice{0.0};
    double rsi14{50.0};
    double sma20{0.0};
    double sma50{0.0};
    double sma200{0.0};
    double ema20{0.0};
    double ema50{0.0};
    double ema200{0.0};
    double macdLine{0.0};
    double macdSignal{0.0};
    double macdHist{0.0};
    double bbUpper{0.0};
    double bbMiddle{0.0};
    double bbLower{0.0};
    double atr14{0.0};
    double vwap{0.0};
    double superTrend{0.0};
    int superTrendDirection{1};
    double stochK{50.0};
    double stochD{50.0};
    std::string trend{"NEUTRAL"}; // BULLISH, BEARISH, NEUTRAL
    std::string rsiState{"NEUTRAL"}; // OVERSOLD, OVERBOUGHT, NEUTRAL
    std::string macdCross{"NONE"}; // BULLISH_CROSS, BEARISH_CROSS, NONE
};

// AI Models
enum class AIProviderType {
    AUTO = 0,
    GROQ,
    OLLAMA,
    OFFLINE
};

struct AIMessage {
    std::string role; // "system", "user", "assistant"
    std::string content;
    int64_t timestamp{0};
};

struct AIRequest {
    std::string prompt;
    std::string systemPrompt;
    std::vector<AIMessage> history;
    std::string model;
    double temperature{0.7};
    int maxTokens{2048};
    bool stream{false};
    nlohmann::json structuredContext;
};

struct AIResponse {
    bool success{false};
    std::string content;
    std::string modelUsed;
    std::string providerName;
    int64_t latencyMs{0};
    int tokensUsed{0};
    std::string errorMessage;
};

struct Conversation {
    std::string id;
    std::string title;
    int64_t createdAt{0};
    int64_t updatedAt{0};
    std::vector<AIMessage> messages;
};

// Multi-Timeframe Confluence Analysis
struct TimeframeSignal {
    std::string timeframe;
    std::string trend; // "BULLISH", "BEARISH", "NEUTRAL"
    double rsi{50.0};
    std::string macd{"NEUTRAL"};
    double score{50.0}; // 0 - 100
};

struct MultiTimeframeConfluence {
    std::string symbol;
    double confluenceScore{50.0}; // 0 - 100%
    std::string overallBias{"NEUTRAL"}; // "STRONG_BUY", "BUY", "NEUTRAL", "SELL", "STRONG_SELL"
    std::string recommendedAction;
    std::vector<TimeframeSignal> signals;
};

// Portfolio & Institutional Risk Models
struct PortfolioAsset {
    std::string symbol;
    double quantity{0.0};
    double avgBuyPrice{0.0};
    double currentPrice{0.0};
    double totalValue{0.0};
    double unrealizedPnL{0.0};
    double unrealizedPnLPercent{0.0};
    double realizedPnL{0.0};
    double allocationPercent{0.0};
    int64_t lastUpdated{0};
};

struct StressScenario {
    std::string name;
    std::string description;
    double shockPercent{0.0}; // e.g. -40.0%
    double impactUsd{0.0};
    double impactPercent{0.0};
    double postShockTotalValue{0.0};
};

struct PortfolioRiskAnalytics {
    double totalPortfolioValue{0.0};
    double var95Daily{0.0}; // 95% 1-Day Value at Risk ($)
    double var99Daily{0.0}; // 99% 1-Day Value at Risk ($)
    double var95Percent{0.0};
    double var99Percent{0.0};
    double portfolioBeta{1.0}; // Beta relative to Bitcoin
    double sharpeRatio{1.45};
    double diversificationScore{78.0}; // 0 - 100
    double concentrationRiskPct{0.0}; // % in top single asset
    std::vector<StressScenario> stressScenarios;
};

enum class OrderSide {
    BUY,
    SELL
};

enum class OrderType {
    MARKET,
    LIMIT,
    STOP_MARKET,
    STOP_LIMIT,
    TAKE_PROFIT,
    TRAILING_STOP
};

enum class OrderStatus {
    OPEN,
    FILLED,
    CANCELLED,
    REJECTED
};

struct PaperOrder {
    std::string orderId;
    std::string symbol;
    OrderSide side{OrderSide::BUY};
    OrderType type{OrderType::MARKET};
    OrderStatus status{OrderStatus::OPEN};
    double quantity{0.0};
    double limitPrice{0.0};
    double stopPrice{0.0};
    double takeProfitPrice{0.0};
    double trailingPercent{0.0}; // e.g. 2.0%
    double leverage{1.0};        // 1x (Spot) up to 50x (Leveraged Margin)
    double executedPrice{0.0};
    double fee{0.0};
    double totalValue{0.0};
    double realizedPnL{0.0};
    int64_t timestamp{0};
};

struct PaperPosition {
    std::string symbol;
    OrderSide side{OrderSide::BUY}; // LONG or SHORT
    double quantity{0.0};
    double avgEntryPrice{0.0};
    double currentPrice{0.0};
    double leverage{1.0};           // 1x up to 50x
    double initialMargin{0.0};      // Required margin collateral
    double liquidationPrice{0.0};   // Estimated liquidation threshold
    double peakPriceSinceEntry{0.0}; // For Trailing Stop calculation
    double stopLossPrice{0.0};
    double takeProfitPrice{0.0};
    double trailingStopPercent{0.0};
    double unrealizedPnL{0.0};
    double unrealizedPnLPercent{0.0};
    double totalValue{0.0};
    int64_t openedAt{0};
};

// Alert Model
enum class AlertCondition {
    PRICE_ABOVE,
    PRICE_BELOW,
    RSI_ABOVE,
    RSI_BELOW,
    EMA_CROSS_UP,
    EMA_CROSS_DOWN,
    MACD_BULLISH_CROSS,
    MACD_BEARISH_CROSS,
    SUPERTREND_FLIP,
    VOLATILITY_SPIKE
};

struct AlertRule {
    std::string id;
    std::string symbol;
    AlertCondition condition{AlertCondition::PRICE_ABOVE};
    double targetValue{0.0};
    std::string message;
    bool isActive{true};
    bool isTriggered{false};
    int64_t createdAt{0};
    int64_t triggeredAt{0};
};

// News Model
struct NewsItem {
    std::string id;
    std::string title;
    std::string source;
    std::string url;
    std::string summary;
    std::string sentiment; // "BULLISH", "BEARISH", "NEUTRAL"
    int64_t publishedAt{0};
    std::vector<std::string> relatedSymbols;
};

// Backtesting & Optimization Models
struct BacktestConfig {
    std::string strategyName; // "SMA_CROSS", "EMA_CROSS", "RSI_MOMENTUM", "MACD_TREND", "BOLLINGER_REVERSION", "SUPERTREND", "MEAN_REVERSION_CONFLUENCE", "GRID_BOT"
    std::string symbol{"BTCUSDT"};
    std::string timeframe{"1h"};
    double initialCapital{10000.0};
    double feePercent{0.1}; // 0.1% per trade
    double leverage{1.0};
    int param1{20};
    int param2{50};
    double paramDouble1{30.0};
    double paramDouble2{70.0};
};

struct BacktestTrade {
    int64_t entryTime{0};
    int64_t exitTime{0};
    OrderSide side{OrderSide::BUY};
    double entryPrice{0.0};
    double exitPrice{0.0};
    double quantity{0.0};
    double pnl{0.0};
    double pnlPercent{0.0};
    double returnOnCapital{0.0};
    std::string exitReason;
};

struct ParameterSweepItem {
    int param1{0};
    int param2{0};
    double paramDouble1{0.0};
    double paramDouble2{0.0};
    double totalReturnPercent{0.0};
    double sharpeRatio{0.0};
    double maxDrawdownPercent{0.0};
    double winRatePercent{0.0};
    double profitFactor{0.0};
    int totalTrades{0};
};

struct MonteCarloSimulationResult {
    double meanFinalCapital{0.0};
    double medianFinalCapital{0.0};
    double var95FinalCapital{0.0};
    double var99FinalCapital{0.0};
    double expectedShortfall{0.0};
    double worstCaseDrawdownPercent{0.0};
    double probabilityOfRuinPercent{0.0}; // Probability of losing > 50% capital
    std::vector<std::vector<double>> samplePaths; // 10 representative paths for plotting
};

struct BacktestResult {
    std::string strategyName;
    std::string symbol;
    std::string timeframe;
    double initialCapital{0.0};
    double finalCapital{0.0};
    double totalReturn{0.0};
    double totalReturnPercent{0.0};
    double maxDrawdownPercent{0.0};
    int totalTrades{0};
    int winningTrades{0};
    int losingTrades{0};
    double winRatePercent{0.0};
    double profitFactor{0.0};
    double averageTradeReturnPercent{0.0};
    double sharpeRatio{0.0};
    double sortinoRatio{0.0};
    double calmarRatio{0.0};
    double expectancy{0.0};
    int maxConsecutiveWins{0};
    int maxConsecutiveLosses{0};
    double averageWinUsd{0.0};
    double averageLossUsd{0.0};
    std::vector<double> equityCurve;
    std::vector<int64_t> equityTimestamps;
    std::vector<BacktestTrade> trades;
    std::vector<ParameterSweepItem> parameterOptimization;
    MonteCarloSimulationResult monteCarlo;
};

// System Telemetry & Monitor
struct SystemMetrics {
    double cpuUsagePercent{0.0};
    double memoryUsageMb{0.0};
    double memoryTotalMb{0.0};
    double fps{60.0};
    double frameTimeMs{16.6};
    int64_t wsLatencyMs{0};
    int64_t restLatencyMs{0};
    int64_t aiLatencyMs{0};
    size_t activeThreads{0};
    size_t dbQueryCount{0};
    bool wsConnected{false};
    bool internetConnected{true};
    std::string activeAIProvider{"GROQ"};
    std::string aiModel{"llama-3.3-70b-versatile"};
    std::string appUptime{"00:00:00"};
};

} // namespace crypto
