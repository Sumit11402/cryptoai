#pragma once

#include "core/Types.hpp"
#include <vector>

namespace crypto {

class BacktestingEngine {
public:
    static BacktestResult runBacktest(const std::vector<Candle>& candles, const BacktestConfig& config);
    static std::vector<ParameterSweepItem> runParameterOptimization(const std::vector<Candle>& candles, const BacktestConfig& config);
    static MonteCarloSimulationResult runMonteCarloSimulation(const std::vector<BacktestTrade>& trades, double initialCapital, int iterations = 1000);

private:
    static BacktestResult backtestSMACross(const std::vector<Candle>& candles, const BacktestConfig& config);
    static BacktestResult backtestEMACross(const std::vector<Candle>& candles, const BacktestConfig& config);
    static BacktestResult backtestRSIMomentum(const std::vector<Candle>& candles, const BacktestConfig& config);
    static BacktestResult backtestMACDTrend(const std::vector<Candle>& candles, const BacktestConfig& config);
    static BacktestResult backtestBollingerReversion(const std::vector<Candle>& candles, const BacktestConfig& config);
    static BacktestResult backtestSuperTrend(const std::vector<Candle>& candles, const BacktestConfig& config);
    static BacktestResult backtestMeanReversionConfluence(const std::vector<Candle>& candles, const BacktestConfig& config);
    static BacktestResult backtestGridBot(const std::vector<Candle>& candles, const BacktestConfig& config);

    static void computePerformanceMetrics(BacktestResult& result);
};

} // namespace crypto

