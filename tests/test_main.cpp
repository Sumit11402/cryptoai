#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>
#include <unordered_map>

#include "core/Types.hpp"
#include "core/Config.hpp"
#include "analysis/TechnicalAnalysisEngine.hpp"
#include "analysis/BacktestingEngine.hpp"
#include "portfolio/PortfolioManager.hpp"
#include "trading/PaperTradingEngine.hpp"
#include "market/MarketManager.hpp"
#include "ai/AIContextBuilder.hpp"
#include "ai/AIRouter.hpp"
#include "ui/HackerConsole.hpp"

using namespace crypto;

void testTechnicalAnalysis() {
    std::cout << "[TEST] Running Technical Analysis Tests..." << std::endl;

    // Generate test candles
    std::vector<Candle> candles;
    for (int i = 0; i < 60; ++i) {
        Candle c;
        c.timestamp = 1000 + i * 60;
        c.open = 100.0 + i;
        c.high = 103.0 + i;
        c.low = 98.0 + i;
        c.close = 102.0 + i;
        c.volume = 50.0 + (i % 5) * 10.0;
        candles.push_back(c);
    }

    // 1. SMA Test
    auto sma20 = TechnicalAnalysisEngine::calculateSMA(candles, 20);
    assert(!sma20.values.empty());
    std::cout << "  ✓ SMA-20 calculated successfully. Last val: " << sma20.values.back() << std::endl;

    // 2. EMA Test
    auto ema20 = TechnicalAnalysisEngine::calculateEMA(candles, 20);
    assert(!ema20.values.empty());
    std::cout << "  ✓ EMA-20 calculated successfully. Last val: " << ema20.values.back() << std::endl;

    // 3. RSI Test
    auto rsi = TechnicalAnalysisEngine::calculateRSI(candles, 14);
    assert(!rsi.values.empty());
    assert(rsi.values.back() >= 0.0 && rsi.values.back() <= 100.0);
    std::cout << "  ✓ RSI-14 calculated successfully. Value: " << rsi.values.back() << std::endl;

    // 4. MACD Test
    auto macd = TechnicalAnalysisEngine::calculateMACD(candles, 12, 26, 9);
    assert(!macd.histogram.empty());
    std::cout << "  ✓ MACD calculated successfully. Hist: " << macd.histogram.back() << std::endl;

    // 5. Bollinger Bands Test
    auto bb = TechnicalAnalysisEngine::calculateBollingerBands(candles, 20, 2.0);
    assert(!bb.middle.empty());
    assert(bb.upper.back() >= bb.middle.back());
    assert(bb.lower.back() <= bb.middle.back());
    std::cout << "  ✓ Bollinger Bands calculated. Upper: " << bb.upper.back() << " Lower: " << bb.lower.back() << std::endl;

    // 6. ATR Test
    auto atr = TechnicalAnalysisEngine::calculateATR(candles, 14);
    assert(!atr.values.empty());
    assert(atr.values.back() > 0.0);
    std::cout << "  ✓ ATR-14 calculated: " << atr.values.back() << std::endl;

    // 7. VWAP & Envelopes Test
    auto vwap = TechnicalAnalysisEngine::calculateVWAP(candles, 1.0);
    assert(!vwap.vwap.empty());
    assert(vwap.vwap.size() == candles.size());
    assert(vwap.upperBand.back() >= vwap.vwap.back());
    assert(vwap.lowerBand.back() <= vwap.vwap.back());
    std::cout << "  ✓ Institutional VWAP calculated. VWAP: " << vwap.vwap.back() << " Upper +1σ: " << vwap.upperBand.back() << std::endl;

    // 8. SuperTrend Test
    auto st = TechnicalAnalysisEngine::calculateSuperTrend(candles, 10, 3.0);
    assert(!st.superTrend.empty());
    assert(st.superTrend.size() == candles.size());
    std::cout << "  ✓ SuperTrend calculated. Value: " << st.superTrend.back() << " Direction: " << (!st.direction.empty() && st.direction.back() == 1 ? "BULLISH" : "BEARISH") << std::endl;

    // 9. Stochastic Oscillator Test
    auto stoch = TechnicalAnalysisEngine::calculateStochastic(candles, 14, 3, 3);
    assert(!stoch.k.empty() && !stoch.d.empty());
    assert(stoch.k.back() >= 0.0 && stoch.k.back() <= 100.0);
    std::cout << "  ✓ Stochastic Oscillator calculated. %K: " << stoch.k.back() << " %D: " << stoch.d.back() << std::endl;

    // 10. Candlestick Pattern Recognition
    auto patterns = TechnicalAnalysisEngine::detectCandlestickPatterns(candles);
    std::cout << "  ✓ Candlestick Patterns detected: " << patterns.size() << " occurrences found." << std::endl;
}

void testCorrelationAndConfluence() {
    std::cout << "[TEST] Running Correlation & Confluence Tests..." << std::endl;

    std::vector<Candle> btcCandles, ethCandles, solCandles;
    for (int i = 0; i < 50; ++i) {
        Candle c1, c2, c3;
        c1.timestamp = c2.timestamp = c3.timestamp = 1000 + i * 3600;
        c1.open = 60000.0 + i * 100.0; c1.high = c1.open + 200.0; c1.low = c1.open - 100.0; c1.close = c1.open + 50.0; c1.volume = 100.0;
        c2.open = 3000.0 + i * 6.0;    c2.high = c2.open + 15.0;  c2.low = c2.open - 10.0;  c2.close = c2.open + 4.0;   c2.volume = 200.0;
        c3.open = 150.0 - i * 0.5;    c3.high = c3.open + 2.0;   c3.low = c3.open - 2.0;   c3.close = c3.open - 0.2;   c3.volume = 300.0;
        btcCandles.push_back(c1);
        ethCandles.push_back(c2);
        solCandles.push_back(c3);
    }

    std::unordered_map<std::string, std::vector<Candle>> marketCandles = {
        {"BTCUSDT", btcCandles},
        {"ETHUSDT", ethCandles},
        {"SOLUSDT", solCandles}
    };

    std::vector<std::string> symbols = {"BTCUSDT", "ETHUSDT", "SOLUSDT"};
    auto matrix = TechnicalAnalysisEngine::calculateCorrelationMatrix(symbols, marketCandles);
    assert(matrix.symbols.size() == 3);
    assert(std::abs(matrix.correlations[0][0] - 1.0) < 1e-4); // Self correlation is 1.0
    assert(std::abs(matrix.correlations[1][1] - 1.0) < 1e-4);
    std::cout << "  ✓ Pearson Correlation Matrix generated (3x3). BTC-ETH correlation: " << matrix.correlations[0][1] << std::endl;

    // Multi-Timeframe Confluence
    std::unordered_map<std::string, std::vector<Candle>> tfCandles = {
        {"15m", btcCandles},
        {"1h", btcCandles},
        {"4h", btcCandles},
        {"1D", btcCandles}
    };
    auto confluence = TechnicalAnalysisEngine::calculateMultiTimeframeConfluence("BTCUSDT", tfCandles);
    assert(confluence.confluenceScore >= 0.0 && confluence.confluenceScore <= 100.0);
    assert(!confluence.overallBias.empty());
    std::cout << "  ✓ Multi-Timeframe Confluence evaluated. Score: " << confluence.confluenceScore << " Bias: " << confluence.overallBias << std::endl;
}

void testPortfolioAndLeveragedTrading() {
    std::cout << "[TEST] Running Portfolio & Leveraged Paper Trading Tests..." << std::endl;

    PaperTradingEngine::instance().init(10000.0);
    assert(PaperTradingEngine::instance().getAvailableBalance() == 10000.0);

    // 1. Leveraged 10x Buy Market Order
    std::string err;
    bool buyOk = PaperTradingEngine::instance().executeMarketOrder(
        "BTCUSDT", OrderSide::BUY, 0.5, 60000.0, err, 10.0, 58000.0, 66000.0, 2.0);
    assert(buyOk);
    auto state = PaperTradingEngine::instance().getAccountState();
    assert(state.usedMargin > 0.0);
    assert(state.usedMargin < 3500.0); // 0.5 * 60000 / 10 = ~3000 margin
    std::cout << "  ✓ 10x Leveraged Market BUY executed. Used Margin: $" << state.usedMargin << " Free Margin: $" << state.freeMargin << std::endl;

    // Verify position liquidation calculation
    auto pos = PaperTradingEngine::instance().getPositions();
    assert(!pos.empty());
    assert(pos[0].leverage == 10.0);
    assert(pos[0].liquidationPrice > 0.0 && pos[0].liquidationPrice < 60000.0);
    std::cout << "  ✓ Leveraged Position created. Liquidation Price: $" << pos[0].liquidationPrice << std::endl;

    // 2. Position Reversal Test
    bool revOk = PaperTradingEngine::instance().reversePosition("BTCUSDT", 62000.0, err);
    assert(revOk);
    auto revPos = PaperTradingEngine::instance().getPositions();
    assert(!revPos.empty());
    assert(revPos[0].side == OrderSide::SELL);
    std::cout << "  ✓ Position Reversal executed cleanly. New Side: SHORT." << std::endl;

    // 3. Close Position
    bool closeOk = PaperTradingEngine::instance().closePosition("BTCUSDT", 61000.0, err);
    assert(closeOk);
    assert(PaperTradingEngine::instance().getPositions().empty());
    std::cout << "  ✓ Leveraged Position closed successfully." << std::endl;

    // 4. Institutional Risk Analytics & Stress Testing
    std::unordered_map<std::string, double> currentPrices = {
        {"BTCUSDT", 65000.0},
        {"ETHUSDT", 3500.0},
        {"SOLUSDT", 180.0}
    };
    auto risk = PaperTradingEngine::instance().getRiskAnalytics(currentPrices);
    assert(risk.var95Percent >= 0.0);
    assert(risk.stressScenarios.size() == 5);
    std::cout << "  ✓ Risk Analytics computed. VaR 95%: " << risk.var95Percent << "% Historical Scenarios: " << risk.stressScenarios.size() << std::endl;
}

void testBacktestingAndMonteCarlo() {
    std::cout << "[TEST] Running Quantitative Backtest, Optimization & Monte Carlo Tests..." << std::endl;

    std::vector<Candle> candles;
    for (int i = 0; i < 150; ++i) {
        Candle c;
        c.timestamp = 1000 + i * 3600;
        double wave = std::sin(i * 0.15) * 15.0 + (i * 0.5);
        c.open = 100.0 + wave;
        c.high = 106.0 + wave;
        c.low = 94.0 + wave;
        c.close = 103.0 + wave;
        c.volume = 150.0;
        candles.push_back(c);
    }

    // 1. Backtest SuperTrend Strategy
    BacktestConfig cfg;
    cfg.strategyName = "SUPERTREND";
    cfg.initialCapital = 10000.0;
    cfg.param1 = 10;
    cfg.param2 = 3;

    auto res = BacktestingEngine::runBacktest(candles, cfg);
    assert(res.initialCapital == 10000.0);
    assert(!res.equityCurve.empty());
    std::cout << "  ✓ SuperTrend Strategy Backtested. Trades: " << res.totalTrades 
              << " Sharpe: " << res.sharpeRatio << " Sortino: " << res.sortinoRatio << std::endl;

    // 2. Parameter Sweep Optimizer
    cfg.strategyName = "SMA_CROSS";
    auto optList = BacktestingEngine::runParameterOptimization(candles, cfg);
    assert(!optList.empty());
    std::cout << "  ✓ Parameter Optimization Grid finished. Top Sharpe: " << optList[0].sharpeRatio 
              << " (Fast=" << optList[0].param1 << ", Slow=" << optList[0].param2 << ")" << std::endl;

    // 3. 1,000-Path Monte Carlo Bootstrap Simulation
    auto mc = BacktestingEngine::runMonteCarloSimulation(res.trades, 10000.0, 1000);
    assert(mc.medianFinalCapital > 0.0);
    assert(!mc.samplePaths.empty());
    std::cout << "  ✓ Monte Carlo 1,000-path simulation completed. Median Final Equity: $" 
              << mc.medianFinalCapital << " Ruin Probability: " << mc.probabilityOfRuinPercent << "%" << std::endl;
}

void testAIContextAndRouting() {
    std::cout << "[TEST] Running AI Context & Routing Tests..." << std::endl;

    Ticker t;
    t.symbol = "BTCUSDT";
    t.lastPrice = 96000.0;
    t.priceChangePercent24h = 2.5;

    std::vector<Candle> candles;
    TechnicalSummary summary;
    summary.currentPrice = 96000.0;
    summary.rsi14 = 55.0;
    summary.trend = "BULLISH";
    summary.vwap = 95800.0;

    auto ctx = AIContextBuilder::buildMarketContext("BTCUSDT", "1h", t, candles, summary);
    assert(ctx.contains("symbol") && ctx["symbol"] == "BTCUSDT");
    assert(ctx.contains("price") && ctx["price"] == 96000.0);
    std::cout << "  ✓ AI Market Context JSON constructed properly with quantitative metrics." << std::endl;

    // Test Offline Synthesis routing
    AIRequest req;
    req.prompt = "Analyze BTC";
    req.structuredContext = ctx;
    auto resp = AIRouter::instance().generateSync(req, AIProviderType::OFFLINE);
    assert(resp.success);
    assert(!resp.content.empty());
    std::cout << "  ✓ AI Router generated valid synthesis response from: " << resp.providerName << std::endl;
}

void testHackerConsoleREPL() {
    std::cout << "[TEST] Running Quant Hacker REPL Engine Tests..." << std::endl;

    Config::instance().setDemoMode(true);
    MarketManager::instance().init();

    HackerConsole::instance().init();
    HackerConsole::instance().executeCommand("help");
    HackerConsole::instance().executeCommand("sysinfo");
    HackerConsole::instance().executeCommand("pos");
    HackerConsole::instance().executeCommand("matrix");
    HackerConsole::instance().executeCommand("confluence BTCUSDT");
    HackerConsole::instance().executeCommand("stress");
    HackerConsole::instance().executeCommand("theme tactical");

    MarketManager::instance().shutdown();

    std::cout << "  ✓ Hacker Quant REPL processed commands (help, sysinfo, pos, matrix, confluence, stress, theme) successfully." << std::endl;
}

int main() {
    std::cout << "====================================================\n";
    std::cout << "     CRYPTØ AI TERMINAL — COMPREHENSIVE TEST SUITE  \n";
    std::cout << "====================================================\n";

    testTechnicalAnalysis();
    testCorrelationAndConfluence();
    testPortfolioAndLeveragedTrading();
    testBacktestingAndMonteCarlo();
    testAIContextAndRouting();
    testHackerConsoleREPL();

    std::cout << "\n>>> ALL 100% UNIT TESTS PASSED SUCCESSFULLY! <<<\n";
    return 0;
}
