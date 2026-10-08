#pragma once

#include "core/Types.hpp"
#include <vector>
#include <unordered_map>
#include <string>

namespace crypto {

class TechnicalAnalysisEngine {
public:
    static IndicatorSeries calculateSMA(const std::vector<Candle>& candles, int period);
    static IndicatorSeries calculateEMA(const std::vector<Candle>& candles, int period);
    static IndicatorSeries calculateRSI(const std::vector<Candle>& candles, int period = 14);
    static MACDResult calculateMACD(const std::vector<Candle>& candles, int fastPeriod = 12, int slowPeriod = 26, int signalPeriod = 9);
    static BollingerBandsResult calculateBollingerBands(const std::vector<Candle>& candles, int period = 20, double stdDevMultiplier = 2.0);
    static IndicatorSeries calculateATR(const std::vector<Candle>& candles, int period = 14);
    static VWAPResult calculateVWAP(const std::vector<Candle>& candles, double stdDevMultiplier = 1.0);
    static SuperTrendResult calculateSuperTrend(const std::vector<Candle>& candles, int atrPeriod = 10, double multiplier = 3.0);
    static StochasticResult calculateStochastic(const std::vector<Candle>& candles, int kPeriod = 14, int dPeriod = 3, int smoothK = 3);
    static std::vector<CandlestickPattern> detectCandlestickPatterns(const std::vector<Candle>& candles);
    
    // Quantitative Matrix & Confluence
    static CorrelationMatrix calculateCorrelationMatrix(
        const std::vector<std::string>& symbols, 
        const std::unordered_map<std::string, std::vector<Candle>>& marketCandles);
    
    static MultiTimeframeConfluence calculateMultiTimeframeConfluence(
        const std::string& symbol, 
        const std::unordered_map<std::string, std::vector<Candle>>& tfCandles);

    static TechnicalSummary generateSummary(const std::vector<Candle>& candles);

private:
    static double calculateStdDev(const std::vector<double>& data, size_t startIdx, size_t count, double mean);
    static double calculatePearsonCorrelation(const std::vector<double>& a, const std::vector<double>& b);
};

} // namespace crypto

