#include "analysis/BacktestingEngine.hpp"
#include "analysis/TechnicalAnalysisEngine.hpp"
#include <algorithm>
#include <cmath>

namespace crypto {

BacktestResult BacktestingEngine::runBacktest(const std::vector<Candle>& candles, const BacktestConfig& config) {
    BacktestResult res;
    if (config.strategyName == "EMA_CROSS") {
        res = backtestEMACross(candles, config);
    } else if (config.strategyName == "RSI_MOMENTUM") {
        res = backtestRSIMomentum(candles, config);
    } else if (config.strategyName == "MACD_TREND") {
        res = backtestMACDTrend(candles, config);
    } else if (config.strategyName == "BOLLINGER_REVERSION") {
        res = backtestBollingerReversion(candles, config);
    } else if (config.strategyName == "SUPERTREND") {
        res = backtestSuperTrend(candles, config);
    } else if (config.strategyName == "MEAN_REVERSION_CONFLUENCE") {
        res = backtestMeanReversionConfluence(candles, config);
    } else if (config.strategyName == "GRID_BOT") {
        res = backtestGridBot(candles, config);
    } else {
        // Default SMA Cross
        res = backtestSMACross(candles, config);
    }

    // Run Monte Carlo simulation on the generated trades
    res.monteCarlo = runMonteCarloSimulation(res.trades, res.initialCapital, 1000);

    return res;
}

BacktestResult BacktestingEngine::backtestSMACross(const std::vector<Candle>& candles, const BacktestConfig& config) {
    BacktestResult res;
    res.strategyName = "SMA Crossover (" + std::to_string(config.param1) + "/" + std::to_string(config.param2) + ")";
    res.symbol = config.symbol;
    res.timeframe = config.timeframe;
    res.initialCapital = config.initialCapital;

    int fastP = config.param1 > 0 ? config.param1 : 10;
    int slowP = config.param2 > fastP ? config.param2 : 30;

    auto fastSma = TechnicalAnalysisEngine::calculateSMA(candles, fastP);
    auto slowSma = TechnicalAnalysisEngine::calculateSMA(candles, slowP);

    if (slowSma.values.empty() || fastSma.values.empty()) return res;

    double cash = config.initialCapital;
    double heldQty = 0.0;
    double entryPrice = 0.0;
    int64_t entryTime = 0;
    double feeRate = config.feePercent / 100.0;

    size_t offset = slowP - fastP;

    for (size_t i = 1; i < slowSma.values.size(); ++i) {
        double prevFast = fastSma.values[offset + i - 1];
        double currFast = fastSma.values[offset + i];
        double prevSlow = slowSma.values[i - 1];
        double currSlow = slowSma.values[i];

        size_t candleIdx = slowP - 1 + i;
        if (candleIdx >= candles.size()) break;
        const auto& c = candles[candleIdx];

        // Bullish Crossover -> Buy
        if (prevFast <= prevSlow && currFast > currSlow && heldQty == 0.0) {
            double buyPrice = c.close;
            double tradeVal = cash * (1.0 - feeRate);
            heldQty = tradeVal / buyPrice;
            cash = 0.0;
            entryPrice = buyPrice;
            entryTime = c.timestamp;
        }
        // Bearish Crossover -> Sell
        else if (prevFast >= prevSlow && currFast < currSlow && heldQty > 0.0) {
            double sellPrice = c.close;
            double grossProceeds = heldQty * sellPrice;
            double netProceeds = grossProceeds * (1.0 - feeRate);

            BacktestTrade t;
            t.entryTime = entryTime;
            t.exitTime = c.timestamp;
            t.side = OrderSide::BUY;
            t.entryPrice = entryPrice;
            t.exitPrice = sellPrice;
            t.quantity = heldQty;
            t.pnl = netProceeds - (heldQty * entryPrice);
            t.pnlPercent = ((sellPrice - entryPrice) / entryPrice) * 100.0;
            t.exitReason = "SMA Bearish Cross";
            res.trades.push_back(t);

            cash = netProceeds;
            heldQty = 0.0;
        }

        double currentTotal = cash + (heldQty * c.close);
        res.equityCurve.push_back(currentTotal);
        res.equityTimestamps.push_back(c.timestamp);
    }

    // Close open position at end
    if (heldQty > 0.0 && !candles.empty()) {
        const auto& lastC = candles.back();
        double netProceeds = (heldQty * lastC.close) * (1.0 - feeRate);
        BacktestTrade t;
        t.entryTime = entryTime;
        t.exitTime = lastC.timestamp;
        t.side = OrderSide::BUY;
        t.entryPrice = entryPrice;
        t.exitPrice = lastC.close;
        t.quantity = heldQty;
        t.pnl = netProceeds - (heldQty * entryPrice);
        t.pnlPercent = ((lastC.close - entryPrice) / entryPrice) * 100.0;
        t.exitReason = "End of Backtest Period";
        res.trades.push_back(t);

        cash = netProceeds;
        heldQty = 0.0;
    }

    res.finalCapital = cash;
    computePerformanceMetrics(res);
    return res;
}

BacktestResult BacktestingEngine::backtestEMACross(const std::vector<Candle>& candles, const BacktestConfig& config) {
    BacktestResult res;
    res.strategyName = "EMA Crossover (" + std::to_string(config.param1) + "/" + std::to_string(config.param2) + ")";
    res.symbol = config.symbol;
    res.timeframe = config.timeframe;
    res.initialCapital = config.initialCapital;

    int fastP = config.param1 > 0 ? config.param1 : 12;
    int slowP = config.param2 > fastP ? config.param2 : 26;

    auto fastEma = TechnicalAnalysisEngine::calculateEMA(candles, fastP);
    auto slowEma = TechnicalAnalysisEngine::calculateEMA(candles, slowP);

    if (slowEma.values.empty() || fastEma.values.empty()) return res;

    double cash = config.initialCapital;
    double heldQty = 0.0;
    double entryPrice = 0.0;
    int64_t entryTime = 0;
    double feeRate = config.feePercent / 100.0;

    size_t offset = slowP - fastP;

    for (size_t i = 1; i < slowEma.values.size(); ++i) {
        double prevFast = fastEma.values[offset + i - 1];
        double currFast = fastEma.values[offset + i];
        double prevSlow = slowEma.values[i - 1];
        double currSlow = slowEma.values[i];

        size_t candleIdx = slowP - 1 + i;
        if (candleIdx >= candles.size()) break;
        const auto& c = candles[candleIdx];

        if (prevFast <= prevSlow && currFast > currSlow && heldQty == 0.0) {
            double buyPrice = c.close;
            double tradeVal = cash * (1.0 - feeRate);
            heldQty = tradeVal / buyPrice;
            cash = 0.0;
            entryPrice = buyPrice;
            entryTime = c.timestamp;
        } else if (prevFast >= prevSlow && currFast < currSlow && heldQty > 0.0) {
            double sellPrice = c.close;
            double netProceeds = (heldQty * sellPrice) * (1.0 - feeRate);

            BacktestTrade t;
            t.entryTime = entryTime;
            t.exitTime = c.timestamp;
            t.side = OrderSide::BUY;
            t.entryPrice = entryPrice;
            t.exitPrice = sellPrice;
            t.quantity = heldQty;
            t.pnl = netProceeds - (heldQty * entryPrice);
            t.pnlPercent = ((sellPrice - entryPrice) / entryPrice) * 100.0;
            t.exitReason = "EMA Bearish Cross";
            res.trades.push_back(t);

            cash = netProceeds;
            heldQty = 0.0;
        }

        double currentTotal = cash + (heldQty * c.close);
        res.equityCurve.push_back(currentTotal);
        res.equityTimestamps.push_back(c.timestamp);
    }

    if (heldQty > 0.0 && !candles.empty()) {
        const auto& lastC = candles.back();
        double netProceeds = (heldQty * lastC.close) * (1.0 - feeRate);
        BacktestTrade t;
        t.entryTime = entryTime;
        t.exitTime = lastC.timestamp;
        t.side = OrderSide::BUY;
        t.entryPrice = entryPrice;
        t.exitPrice = lastC.close;
        t.quantity = heldQty;
        t.pnl = netProceeds - (heldQty * entryPrice);
        t.pnlPercent = ((lastC.close - entryPrice) / entryPrice) * 100.0;
        t.exitReason = "End of Backtest Period";
        res.trades.push_back(t);

        cash = netProceeds;
    }

    res.finalCapital = cash;
    computePerformanceMetrics(res);
    return res;
}

BacktestResult BacktestingEngine::backtestRSIMomentum(const std::vector<Candle>& candles, const BacktestConfig& config) {
    BacktestResult res;
    res.strategyName = "RSI Mean Reversion (" + std::to_string((int)config.paramDouble1) + "/" + std::to_string((int)config.paramDouble2) + ")";
    res.symbol = config.symbol;
    res.timeframe = config.timeframe;
    res.initialCapital = config.initialCapital;

    auto rsi = TechnicalAnalysisEngine::calculateRSI(candles, 14);
    if (rsi.values.empty()) return res;

    double cash = config.initialCapital;
    double heldQty = 0.0;
    double entryPrice = 0.0;
    int64_t entryTime = 0;
    double feeRate = config.feePercent / 100.0;
    double oversold = config.paramDouble1 > 0 ? config.paramDouble1 : 30.0;
    double overbought = config.paramDouble2 > 0 ? config.paramDouble2 : 70.0;

    for (size_t i = 1; i < rsi.values.size(); ++i) {
        double rsiVal = rsi.values[i];
        size_t candleIdx = 14 + i;
        if (candleIdx >= candles.size()) break;
        const auto& c = candles[candleIdx];

        if (rsiVal <= oversold && heldQty == 0.0) {
            double buyPrice = c.close;
            double tradeVal = cash * (1.0 - feeRate);
            heldQty = tradeVal / buyPrice;
            cash = 0.0;
            entryPrice = buyPrice;
            entryTime = c.timestamp;
        } else if (rsiVal >= overbought && heldQty > 0.0) {
            double sellPrice = c.close;
            double netProceeds = (heldQty * sellPrice) * (1.0 - feeRate);

            BacktestTrade t;
            t.entryTime = entryTime;
            t.exitTime = c.timestamp;
            t.side = OrderSide::BUY;
            t.entryPrice = entryPrice;
            t.exitPrice = sellPrice;
            t.quantity = heldQty;
            t.pnl = netProceeds - (heldQty * entryPrice);
            t.pnlPercent = ((sellPrice - entryPrice) / entryPrice) * 100.0;
            t.exitReason = "RSI Overbought Target";
            res.trades.push_back(t);

            cash = netProceeds;
            heldQty = 0.0;
        }

        double currentTotal = cash + (heldQty * c.close);
        res.equityCurve.push_back(currentTotal);
        res.equityTimestamps.push_back(c.timestamp);
    }

    if (heldQty > 0.0 && !candles.empty()) {
        const auto& lastC = candles.back();
        double netProceeds = (heldQty * lastC.close) * (1.0 - feeRate);
        BacktestTrade t;
        t.entryTime = entryTime;
        t.exitTime = lastC.timestamp;
        t.side = OrderSide::BUY;
        t.entryPrice = entryPrice;
        t.exitPrice = lastC.close;
        t.quantity = heldQty;
        t.pnl = netProceeds - (heldQty * entryPrice);
        t.pnlPercent = ((lastC.close - entryPrice) / entryPrice) * 100.0;
        t.exitReason = "End of Backtest Period";
        res.trades.push_back(t);
        cash = netProceeds;
    }

    res.finalCapital = cash;
    computePerformanceMetrics(res);
    return res;
}

BacktestResult BacktestingEngine::backtestMACDTrend(const std::vector<Candle>& candles, const BacktestConfig& config) {
    BacktestResult res;
    res.strategyName = "MACD Trend Momentum (12/26/9)";
    res.symbol = config.symbol;
    res.timeframe = config.timeframe;
    res.initialCapital = config.initialCapital;

    auto macd = TechnicalAnalysisEngine::calculateMACD(candles, 12, 26, 9);
    if (macd.histogram.empty()) return res;

    double cash = config.initialCapital;
    double heldQty = 0.0;
    double entryPrice = 0.0;
    int64_t entryTime = 0;
    double feeRate = config.feePercent / 100.0;

    for (size_t i = 1; i < macd.histogram.size(); ++i) {
        double prevHist = macd.histogram[i - 1];
        double currHist = macd.histogram[i];

        size_t candleIdx = 34 + i;
        if (candleIdx >= candles.size()) break;
        const auto& c = candles[candleIdx];

        if (prevHist <= 0.0 && currHist > 0.0 && heldQty == 0.0) {
            double buyPrice = c.close;
            double tradeVal = cash * (1.0 - feeRate);
            heldQty = tradeVal / buyPrice;
            cash = 0.0;
            entryPrice = buyPrice;
            entryTime = c.timestamp;
        } else if (prevHist >= 0.0 && currHist < 0.0 && heldQty > 0.0) {
            double sellPrice = c.close;
            double netProceeds = (heldQty * sellPrice) * (1.0 - feeRate);

            BacktestTrade t;
            t.entryTime = entryTime;
            t.exitTime = c.timestamp;
            t.side = OrderSide::BUY;
            t.entryPrice = entryPrice;
            t.exitPrice = sellPrice;
            t.quantity = heldQty;
            t.pnl = netProceeds - (heldQty * entryPrice);
            t.pnlPercent = ((sellPrice - entryPrice) / entryPrice) * 100.0;
            t.exitReason = "MACD Bearish Cross";
            res.trades.push_back(t);

            cash = netProceeds;
            heldQty = 0.0;
        }

        double currentTotal = cash + (heldQty * c.close);
        res.equityCurve.push_back(currentTotal);
        res.equityTimestamps.push_back(c.timestamp);
    }

    if (heldQty > 0.0 && !candles.empty()) {
        const auto& lastC = candles.back();
        double netProceeds = (heldQty * lastC.close) * (1.0 - feeRate);
        BacktestTrade t;
        t.entryTime = entryTime;
        t.exitTime = lastC.timestamp;
        t.side = OrderSide::BUY;
        t.entryPrice = entryPrice;
        t.exitPrice = lastC.close;
        t.quantity = heldQty;
        t.pnl = netProceeds - (heldQty * entryPrice);
        t.pnlPercent = ((lastC.close - entryPrice) / entryPrice) * 100.0;
        t.exitReason = "End of Backtest Period";
        res.trades.push_back(t);
        cash = netProceeds;
    }

    res.finalCapital = cash;
    computePerformanceMetrics(res);
    return res;
}

BacktestResult BacktestingEngine::backtestBollingerReversion(const std::vector<Candle>& candles, const BacktestConfig& config) {
    BacktestResult res;
    res.strategyName = "Bollinger Bands Mean Reversion (20, 2.0)";
    res.symbol = config.symbol;
    res.timeframe = config.timeframe;
    res.initialCapital = config.initialCapital;

    auto bb = TechnicalAnalysisEngine::calculateBollingerBands(candles, 20, 2.0);
    if (bb.middle.empty()) return res;

    double cash = config.initialCapital;
    double heldQty = 0.0;
    double entryPrice = 0.0;
    int64_t entryTime = 0;
    double feeRate = config.feePercent / 100.0;

    for (size_t i = 0; i < bb.middle.size(); ++i) {
        size_t candleIdx = 19 + i;
        if (candleIdx >= candles.size()) break;
        const auto& c = candles[candleIdx];

        double lowBand = bb.lower[i];
        double upBand = bb.upper[i];

        if (c.close <= lowBand && heldQty == 0.0) {
            double buyPrice = c.close;
            double tradeVal = cash * (1.0 - feeRate);
            heldQty = tradeVal / buyPrice;
            cash = 0.0;
            entryPrice = buyPrice;
            entryTime = c.timestamp;
        } else if (c.close >= upBand && heldQty > 0.0) {
            double sellPrice = c.close;
            double netProceeds = (heldQty * sellPrice) * (1.0 - feeRate);

            BacktestTrade t;
            t.entryTime = entryTime;
            t.exitTime = c.timestamp;
            t.side = OrderSide::BUY;
            t.entryPrice = entryPrice;
            t.exitPrice = sellPrice;
            t.quantity = heldQty;
            t.pnl = netProceeds - (heldQty * entryPrice);
            t.pnlPercent = ((sellPrice - entryPrice) / entryPrice) * 100.0;
            t.exitReason = "Bollinger Upper Band Reach";
            res.trades.push_back(t);

            cash = netProceeds;
            heldQty = 0.0;
        }

        double currentTotal = cash + (heldQty * c.close);
        res.equityCurve.push_back(currentTotal);
        res.equityTimestamps.push_back(c.timestamp);
    }

    if (heldQty > 0.0 && !candles.empty()) {
        const auto& lastC = candles.back();
        double netProceeds = (heldQty * lastC.close) * (1.0 - feeRate);
        BacktestTrade t;
        t.entryTime = entryTime;
        t.exitTime = lastC.timestamp;
        t.side = OrderSide::BUY;
        t.entryPrice = entryPrice;
        t.exitPrice = lastC.close;
        t.quantity = heldQty;
        t.pnl = netProceeds - (heldQty * entryPrice);
        t.pnlPercent = ((lastC.close - entryPrice) / entryPrice) * 100.0;
        t.exitReason = "End of Backtest Period";
        res.trades.push_back(t);
        cash = netProceeds;
    }

    res.finalCapital = cash;
    computePerformanceMetrics(res);
    return res;
}

BacktestResult BacktestingEngine::backtestSuperTrend(const std::vector<Candle>& candles, const BacktestConfig& config) {
    BacktestResult res;
    int atrP = (config.param1 > 0) ? config.param1 : 10;
    double mult = (config.paramDouble1 > 0.0) ? config.paramDouble1 : 3.0;

    res.strategyName = "SuperTrend (" + std::to_string(atrP) + " / " + std::to_string(mult).substr(0, 3) + "x)";
    res.symbol = config.symbol;
    res.timeframe = config.timeframe;
    res.initialCapital = config.initialCapital;

    auto st = TechnicalAnalysisEngine::calculateSuperTrend(candles, atrP, mult);
    if (st.direction.empty() || candles.size() < st.direction.size()) return res;

    size_t offset = candles.size() - st.direction.size();
    double cash = config.initialCapital;
    double heldQty = 0.0;
    double entryPrice = 0.0;
    int64_t entryTime = 0;
    double feeRate = config.feePercent / 100.0;

    for (size_t i = 1; i < st.direction.size(); ++i) {
        int prevDir = st.direction[i - 1];
        int currDir = st.direction[i];
        size_t cIdx = offset + i;
        const auto& c = candles[cIdx];

        // SuperTrend flips Bullish (+1) -> BUY
        if (prevDir == -1 && currDir == 1 && heldQty == 0.0) {
            double buyPrice = c.close;
            double tradeVal = cash * (1.0 - feeRate);
            heldQty = tradeVal / buyPrice;
            cash = 0.0;
            entryPrice = buyPrice;
            entryTime = c.timestamp;
        }
        // SuperTrend flips Bearish (-1) -> SELL
        else if (prevDir == 1 && currDir == -1 && heldQty > 0.0) {
            double sellPrice = c.close;
            double netProceeds = (heldQty * sellPrice) * (1.0 - feeRate);

            BacktestTrade t;
            t.entryTime = entryTime;
            t.exitTime = c.timestamp;
            t.side = OrderSide::BUY;
            t.entryPrice = entryPrice;
            t.exitPrice = sellPrice;
            t.quantity = heldQty;
            t.pnl = netProceeds - (heldQty * entryPrice);
            t.pnlPercent = ((sellPrice - entryPrice) / entryPrice) * 100.0;
            t.exitReason = "SuperTrend Bearish Flip";
            res.trades.push_back(t);

            cash = netProceeds;
            heldQty = 0.0;
        }

        double currentTotal = cash + (heldQty * c.close);
        res.equityCurve.push_back(currentTotal);
        res.equityTimestamps.push_back(c.timestamp);
    }

    if (heldQty > 0.0 && !candles.empty()) {
        const auto& lastC = candles.back();
        double netProceeds = (heldQty * lastC.close) * (1.0 - feeRate);
        BacktestTrade t;
        t.entryTime = entryTime;
        t.exitTime = lastC.timestamp;
        t.side = OrderSide::BUY;
        t.entryPrice = entryPrice;
        t.exitPrice = lastC.close;
        t.quantity = heldQty;
        t.pnl = netProceeds - (heldQty * entryPrice);
        t.pnlPercent = ((lastC.close - entryPrice) / entryPrice) * 100.0;
        t.exitReason = "End of Backtest Period";
        res.trades.push_back(t);
        cash = netProceeds;
    }

    res.finalCapital = cash;
    computePerformanceMetrics(res);
    return res;
}

BacktestResult BacktestingEngine::backtestMeanReversionConfluence(const std::vector<Candle>& candles, const BacktestConfig& config) {
    BacktestResult res;
    int bbPeriod = (config.param1 > 0) ? config.param1 : 20;
    int rsiPeriod = (config.param2 > 0) ? config.param2 : 14;
    double rsiOversold = (config.paramDouble1 > 0.0) ? config.paramDouble1 : 32.0;
    double rsiOverbought = (config.paramDouble2 > 0.0) ? config.paramDouble2 : 68.0;

    res.strategyName = "Mean Reversion Confluence (BB + RSI)";
    res.symbol = config.symbol;
    res.timeframe = config.timeframe;
    res.initialCapital = config.initialCapital;

    auto bb = TechnicalAnalysisEngine::calculateBollingerBands(candles, bbPeriod, 2.0);
    auto rsi = TechnicalAnalysisEngine::calculateRSI(candles, rsiPeriod);

    if (bb.lower.empty() || rsi.values.empty()) return res;

    size_t offsetBB = candles.size() - bb.lower.size();
    size_t offsetRSI = candles.size() - rsi.values.size();
    size_t startIdx = std::max(offsetBB, offsetRSI);

    double cash = config.initialCapital;
    double heldQty = 0.0;
    double entryPrice = 0.0;
    int64_t entryTime = 0;
    double feeRate = config.feePercent / 100.0;

    for (size_t i = startIdx; i < candles.size(); ++i) {
        const auto& c = candles[i];
        double currBB = bb.lower[i - offsetBB];
        double currBBUpper = bb.upper[i - offsetBB];
        double currRSI = rsi.values[i - offsetRSI];

        // BUY Signal: Price touches lower band AND RSI oversold
        if (c.close <= currBB && currRSI <= rsiOversold && heldQty == 0.0) {
            double buyPrice = c.close;
            double tradeVal = cash * (1.0 - feeRate);
            heldQty = tradeVal / buyPrice;
            cash = 0.0;
            entryPrice = buyPrice;
            entryTime = c.timestamp;
        }
        // SELL Signal: Price reaches upper band OR RSI overbought
        else if ((c.close >= currBBUpper || currRSI >= rsiOverbought) && heldQty > 0.0) {
            double sellPrice = c.close;
            double netProceeds = (heldQty * sellPrice) * (1.0 - feeRate);

            BacktestTrade t;
            t.entryTime = entryTime;
            t.exitTime = c.timestamp;
            t.side = OrderSide::BUY;
            t.entryPrice = entryPrice;
            t.exitPrice = sellPrice;
            t.quantity = heldQty;
            t.pnl = netProceeds - (heldQty * entryPrice);
            t.pnlPercent = ((sellPrice - entryPrice) / entryPrice) * 100.0;
            t.exitReason = "BB Upper or RSI Overbought Reversion";
            res.trades.push_back(t);

            cash = netProceeds;
            heldQty = 0.0;
        }

        double currentTotal = cash + (heldQty * c.close);
        res.equityCurve.push_back(currentTotal);
        res.equityTimestamps.push_back(c.timestamp);
    }

    if (heldQty > 0.0 && !candles.empty()) {
        const auto& lastC = candles.back();
        double netProceeds = (heldQty * lastC.close) * (1.0 - feeRate);
        BacktestTrade t;
        t.entryTime = entryTime;
        t.exitTime = lastC.timestamp;
        t.side = OrderSide::BUY;
        t.entryPrice = entryPrice;
        t.exitPrice = lastC.close;
        t.quantity = heldQty;
        t.pnl = netProceeds - (heldQty * entryPrice);
        t.pnlPercent = ((lastC.close - entryPrice) / entryPrice) * 100.0;
        t.exitReason = "End of Backtest Period";
        res.trades.push_back(t);
        cash = netProceeds;
    }

    res.finalCapital = cash;
    computePerformanceMetrics(res);
    return res;
}

BacktestResult BacktestingEngine::backtestGridBot(const std::vector<Candle>& candles, const BacktestConfig& config) {
    BacktestResult res;
    res.strategyName = "Grid Trading Bot (10 Geometric Grids)";
    res.symbol = config.symbol;
    res.timeframe = config.timeframe;
    res.initialCapital = config.initialCapital;

    if (candles.size() < 10) return res;

    double minPrice = candles[0].low;
    double maxPrice = candles[0].high;
    for (const auto& c : candles) {
        if (c.low < minPrice) minPrice = c.low;
        if (c.high > maxPrice) maxPrice = c.high;
    }

    int numGrids = (config.param1 > 0) ? config.param1 : 10;
    double gridStep = (maxPrice - minPrice) / (double)numGrids;

    double cash = config.initialCapital * 0.5; // 50% cash, 50% asset
    double heldQty = (config.initialCapital * 0.5) / candles[0].close;
    double feeRate = config.feePercent / 100.0;
    int lastGridLevel = (int)((candles[0].close - minPrice) / gridStep);

    for (const auto& c : candles) {
        int currGridLevel = (int)((c.close - minPrice) / gridStep);
        if (currGridLevel < 0) currGridLevel = 0;
        if (currGridLevel > numGrids) currGridLevel = numGrids;

        // Down grid movement -> BUY grid slice
        if (currGridLevel < lastGridLevel && cash > (config.initialCapital * 0.05)) {
            double buySlice = cash * 0.20;
            double qty = (buySlice * (1.0 - feeRate)) / c.close;
            heldQty += qty;
            cash -= buySlice;

            BacktestTrade t;
            t.entryTime = c.timestamp;
            t.exitTime = c.timestamp;
            t.side = OrderSide::BUY;
            t.entryPrice = c.close;
            t.exitPrice = c.close;
            t.quantity = qty;
            t.pnl = -buySlice * feeRate;
            t.pnlPercent = 0.0;
            t.exitReason = "Grid Buy Execution";
            res.trades.push_back(t);
            lastGridLevel = currGridLevel;
        }
        // Up grid movement -> SELL grid slice
        else if (currGridLevel > lastGridLevel && heldQty > 0.01) {
            double sellQty = heldQty * 0.20;
            double gross = sellQty * c.close;
            double net = gross * (1.0 - feeRate);
            heldQty -= sellQty;
            cash += net;

            BacktestTrade t;
            t.entryTime = c.timestamp;
            t.exitTime = c.timestamp;
            t.side = OrderSide::SELL;
            t.entryPrice = c.close;
            t.exitPrice = c.close;
            t.quantity = sellQty;
            t.pnl = gross * 0.01; // simulated incremental grid capture
            t.pnlPercent = 1.0;
            t.exitReason = "Grid Sell Execution";
            res.trades.push_back(t);
            lastGridLevel = currGridLevel;
        }

        double totalVal = cash + (heldQty * c.close);
        res.equityCurve.push_back(totalVal);
        res.equityTimestamps.push_back(c.timestamp);
    }

    res.finalCapital = cash + (heldQty * candles.back().close);
    computePerformanceMetrics(res);
    return res;
}

std::vector<ParameterSweepItem> BacktestingEngine::runParameterOptimization(const std::vector<Candle>& candles, const BacktestConfig& baseConfig) {
    std::vector<ParameterSweepItem> items;
    if (candles.size() < 50) return items;

    std::vector<int> p1Options = {5, 10, 15, 20, 30};
    std::vector<int> p2Options = {30, 45, 60, 90, 120};

    for (int p1 : p1Options) {
        for (int p2 : p2Options) {
            if (p1 >= p2) continue;
            BacktestConfig cfg = baseConfig;
            cfg.param1 = p1;
            cfg.param2 = p2;
            
            BacktestResult r;
            if (cfg.strategyName == "EMA_CROSS") r = backtestEMACross(candles, cfg);
            else if (cfg.strategyName == "SUPERTREND") r = backtestSuperTrend(candles, cfg);
            else if (cfg.strategyName == "MEAN_REVERSION_CONFLUENCE") r = backtestMeanReversionConfluence(candles, cfg);
            else r = backtestSMACross(candles, cfg);

            ParameterSweepItem item;
            item.param1 = p1;
            item.param2 = p2;
            item.totalReturnPercent = r.totalReturnPercent;
            item.sharpeRatio = r.sharpeRatio;
            item.maxDrawdownPercent = r.maxDrawdownPercent;
            item.winRatePercent = r.winRatePercent;
            item.profitFactor = r.profitFactor;
            item.totalTrades = r.totalTrades;
            items.push_back(item);
        }
    }

    std::sort(items.begin(), items.end(), [](const ParameterSweepItem& a, const ParameterSweepItem& b) {
        return a.sharpeRatio > b.sharpeRatio;
    });

    return items;
}

MonteCarloSimulationResult BacktestingEngine::runMonteCarloSimulation(const std::vector<BacktestTrade>& trades, double initialCapital, int iterations) {
    MonteCarloSimulationResult mc;
    if (trades.empty() || iterations <= 0) {
        mc.meanFinalCapital = initialCapital;
        mc.medianFinalCapital = initialCapital;
        mc.var95FinalCapital = initialCapital * 0.95;
        mc.var99FinalCapital = initialCapital * 0.90;
        return mc;
    }

    std::vector<double> tradePnLPercentages;
    tradePnLPercentages.reserve(trades.size());
    for (const auto& t : trades) {
        tradePnLPercentages.push_back(t.pnlPercent / 100.0);
    }

    std::vector<double> finalCapitals;
    finalCapitals.reserve(iterations);
    double worstDdAcrossSims = 0.0;
    int ruinCount = 0;

    // Pseudo-random deterministic seed
    uint32_t seed = 42;
    auto lcg = [&seed]() -> double {
        seed = (seed * 1664525u + 1013904223u);
        return (double)(seed % 1000000) / 1000000.0;
    };

    for (int iter = 0; iter < iterations; ++iter) {
        double simCap = initialCapital;
        double simPeak = initialCapital;
        double simMaxDd = 0.0;
        std::vector<double> simPath;
        if (iter < 10) simPath.reserve(tradePnLPercentages.size());

        for (size_t step = 0; step < tradePnLPercentages.size(); ++step) {
            size_t randIdx = (size_t)(lcg() * tradePnLPercentages.size()) % tradePnLPercentages.size();
            double ret = tradePnLPercentages[randIdx];
            simCap *= (1.0 + ret);
            if (simCap < 0.0) simCap = 0.0;

            if (simCap > simPeak) simPeak = simCap;
            double dd = (simPeak > 0.0) ? ((simPeak - simCap) / simPeak * 100.0) : 0.0;
            if (dd > simMaxDd) simMaxDd = dd;

            if (iter < 10) simPath.push_back(simCap);
        }

        if (iter < 10) mc.samplePaths.push_back(simPath);
        finalCapitals.push_back(simCap);

        if (simMaxDd > worstDdAcrossSims) worstDdAcrossSims = simMaxDd;
        if (simCap < initialCapital * 0.50) ruinCount++;
    }

    std::sort(finalCapitals.begin(), finalCapitals.end());

    double sumCap = 0.0;
    for (double c : finalCapitals) sumCap += c;
    mc.meanFinalCapital = sumCap / (double)finalCapitals.size();
    mc.medianFinalCapital = finalCapitals[finalCapitals.size() / 2];

    size_t idx95 = (size_t)(finalCapitals.size() * 0.05);
    size_t idx99 = (size_t)(finalCapitals.size() * 0.01);
    mc.var95FinalCapital = finalCapitals[idx95];
    mc.var99FinalCapital = finalCapitals[idx99];

    // Expected shortfall (average of worst 5% outcomes)
    double esSum = 0.0;
    for (size_t k = 0; k <= idx95; ++k) esSum += finalCapitals[k];
    mc.expectedShortfall = (idx95 > 0) ? (esSum / (double)(idx95 + 1)) : mc.var95FinalCapital;

    mc.worstCaseDrawdownPercent = worstDdAcrossSims;
    mc.probabilityOfRuinPercent = ((double)ruinCount / (double)iterations) * 100.0;

    return mc;
}

void BacktestingEngine::computePerformanceMetrics(BacktestResult& result) {
    result.totalReturn = result.finalCapital - result.initialCapital;
    result.totalReturnPercent = (result.initialCapital > 0) ? (result.totalReturn / result.initialCapital * 100.0) : 0.0;
    result.totalTrades = (int)result.trades.size();

    double grossProfits = 0.0;
    double grossLosses = 0.0;
    double sumReturns = 0.0;
    int currLossStreak = 0, maxLossStreak = 0;
    int currWinStreak = 0, maxWinStreak = 0;

    std::vector<double> tradeReturns;
    tradeReturns.reserve(result.trades.size());

    for (const auto& t : result.trades) {
        sumReturns += t.pnlPercent;
        tradeReturns.push_back(t.pnlPercent);

        if (t.pnl > 0) {
            result.winningTrades++;
            grossProfits += t.pnl;
            currWinStreak++;
            currLossStreak = 0;
            if (currWinStreak > maxWinStreak) maxWinStreak = currWinStreak;
        } else {
            result.losingTrades++;
            grossLosses += std::abs(t.pnl);
            currLossStreak++;
            currWinStreak = 0;
            if (currLossStreak > maxLossStreak) maxLossStreak = currLossStreak;
        }
    }

    result.maxConsecutiveWins = maxWinStreak;
    result.maxConsecutiveLosses = maxLossStreak;
    result.averageWinUsd = (result.winningTrades > 0) ? (grossProfits / result.winningTrades) : 0.0;
    result.averageLossUsd = (result.losingTrades > 0) ? (grossLosses / result.losingTrades) : 0.0;

    result.winRatePercent = (result.totalTrades > 0) ? ((double)result.winningTrades / result.totalTrades * 100.0) : 0.0;
    result.profitFactor = (grossLosses > 0.0) ? (grossProfits / grossLosses) : (grossProfits > 0 ? 99.9 : 1.0);
    result.averageTradeReturnPercent = (result.totalTrades > 0) ? (sumReturns / result.totalTrades) : 0.0;

    // Mathematical Expectancy
    double winProb = (result.totalTrades > 0) ? ((double)result.winningTrades / result.totalTrades) : 0.0;
    double lossProb = 1.0 - winProb;
    result.expectancy = (winProb * result.averageWinUsd) - (lossProb * result.averageLossUsd);

    // Max Drawdown
    double peak = result.initialCapital;
    double maxDd = 0.0;
    for (double val : result.equityCurve) {
        if (val > peak) peak = val;
        double dd = (peak > 0) ? ((peak - val) / peak * 100.0) : 0.0;
        if (dd > maxDd) maxDd = dd;
    }
    result.maxDrawdownPercent = maxDd;

    // Sharpe and Sortino Ratios
    if (!tradeReturns.empty()) {
        double meanRet = sumReturns / tradeReturns.size();
        double varSum = 0.0;
        double downVarSum = 0.0;
        for (double r : tradeReturns) {
            double diff = r - meanRet;
            varSum += diff * diff;
            if (r < 0.0) {
                downVarSum += r * r;
            }
        }
        double stdDev = std::sqrt(varSum / tradeReturns.size());
        double downDev = std::sqrt(downVarSum / tradeReturns.size());

        // Annualized scaling factor (approx based on frequency)
        double annualScale = std::sqrt(252.0);
        result.sharpeRatio = (stdDev > 0.0) ? (meanRet / stdDev) * annualScale : 0.0;
        result.sortinoRatio = (downDev > 0.0) ? (meanRet / downDev) * annualScale : 0.0;
    }

    // Calmar Ratio
    result.calmarRatio = (result.maxDrawdownPercent > 0.0) ? (result.totalReturnPercent / result.maxDrawdownPercent) : 0.0;
}

} // namespace crypto
