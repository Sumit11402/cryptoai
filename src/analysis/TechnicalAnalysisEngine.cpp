#include "analysis/TechnicalAnalysisEngine.hpp"
#include <cmath>
#include <numeric>
#include <algorithm>

namespace crypto {

double TechnicalAnalysisEngine::calculateStdDev(const std::vector<double>& data, size_t startIdx, size_t count, double mean) {
    if (count == 0 || startIdx + count > data.size()) return 0.0;
    double sumSqDiff = 0.0;
    for (size_t i = 0; i < count; ++i) {
        double diff = data[startIdx + i] - mean;
        sumSqDiff += diff * diff;
    }
    return std::sqrt(sumSqDiff / (double)count);
}

IndicatorSeries TechnicalAnalysisEngine::calculateSMA(const std::vector<Candle>& candles, int period) {
    IndicatorSeries result;
    if ((int)candles.size() < period || period <= 0) return result;

    result.timestamps.reserve(candles.size() - period + 1);
    result.values.reserve(candles.size() - period + 1);

    double currentSum = 0.0;
    for (int i = 0; i < period; ++i) {
        currentSum += candles[i].close;
    }

    result.timestamps.push_back(candles[period - 1].timestamp);
    result.values.push_back(currentSum / period);

    for (size_t i = period; i < candles.size(); ++i) {
        currentSum += candles[i].close - candles[i - period].close;
        result.timestamps.push_back(candles[i].timestamp);
        result.values.push_back(currentSum / period);
    }

    return result;
}

IndicatorSeries TechnicalAnalysisEngine::calculateEMA(const std::vector<Candle>& candles, int period) {
    IndicatorSeries result;
    if ((int)candles.size() < period || period <= 0) return result;

    result.timestamps.reserve(candles.size() - period + 1);
    result.values.reserve(candles.size() - period + 1);

    // Initial SMA
    double initialSum = 0.0;
    for (int i = 0; i < period; ++i) {
        initialSum += candles[i].close;
    }
    double currentEma = initialSum / period;

    result.timestamps.push_back(candles[period - 1].timestamp);
    result.values.push_back(currentEma);

    double multiplier = 2.0 / (period + 1.0);

    for (size_t i = period; i < candles.size(); ++i) {
        currentEma = (candles[i].close - currentEma) * multiplier + currentEma;
        result.timestamps.push_back(candles[i].timestamp);
        result.values.push_back(currentEma);
    }

    return result;
}

IndicatorSeries TechnicalAnalysisEngine::calculateRSI(const std::vector<Candle>& candles, int period) {
    IndicatorSeries result;
    if ((int)candles.size() <= period || period <= 0) return result;

    std::vector<double> gains;
    std::vector<double> losses;
    gains.reserve(candles.size() - 1);
    losses.reserve(candles.size() - 1);

    for (size_t i = 1; i < candles.size(); ++i) {
        double change = candles[i].close - candles[i - 1].close;
        if (change >= 0.0) {
            gains.push_back(change);
            losses.push_back(0.0);
        } else {
            gains.push_back(0.0);
            losses.push_back(-change);
        }
    }

    // Initial average gain and loss (SMA)
    double avgGain = 0.0;
    double avgLoss = 0.0;
    for (int i = 0; i < period; ++i) {
        avgGain += gains[i];
        avgLoss += losses[i];
    }
    avgGain /= period;
    avgLoss /= period;

    auto computeRSI = [](double ag, double al) -> double {
        if (al == 0.0) return 100.0;
        if (ag == 0.0) return 0.0;
        double rs = ag / al;
        return 100.0 - (100.0 / (1.0 + rs));
    };

    result.timestamps.push_back(candles[period].timestamp);
    result.values.push_back(computeRSI(avgGain, avgLoss));

    // Wilder's smoothing
    for (size_t i = period; i < gains.size(); ++i) {
        avgGain = (avgGain * (period - 1) + gains[i]) / period;
        avgLoss = (avgLoss * (period - 1) + losses[i]) / period;

        result.timestamps.push_back(candles[i + 1].timestamp);
        result.values.push_back(computeRSI(avgGain, avgLoss));
    }

    return result;
}

MACDResult TechnicalAnalysisEngine::calculateMACD(const std::vector<Candle>& candles, int fastPeriod, int slowPeriod, int signalPeriod) {
    MACDResult result;
    if ((int)candles.size() < slowPeriod + signalPeriod) return result;

    auto fastEma = calculateEMA(candles, fastPeriod);
    auto slowEma = calculateEMA(candles, slowPeriod);

    if (fastEma.values.empty() || slowEma.values.empty()) return result;

    // Align fast and slow EMAs by timestamp
    size_t fastOffset = slowPeriod - fastPeriod;
    std::vector<double> macdLine;
    std::vector<int64_t> macdTimestamps;
    macdLine.reserve(slowEma.values.size());
    macdTimestamps.reserve(slowEma.values.size());

    for (size_t i = 0; i < slowEma.values.size(); ++i) {
        double fastVal = fastEma.values[fastOffset + i];
        double slowVal = slowEma.values[i];
        macdLine.push_back(fastVal - slowVal);
        macdTimestamps.push_back(slowEma.timestamps[i]);
    }

    // Calculate Signal line (EMA of MACD Line)
    if ((int)macdLine.size() < signalPeriod) return result;

    double initialSum = 0.0;
    for (int i = 0; i < signalPeriod; ++i) {
        initialSum += macdLine[i];
    }
    double signalEma = initialSum / signalPeriod;

    double multiplier = 2.0 / (signalPeriod + 1.0);

    for (size_t i = signalPeriod - 1; i < macdLine.size(); ++i) {
        if (i >= (size_t)signalPeriod) {
            signalEma = (macdLine[i] - signalEma) * multiplier + signalEma;
        }
        double mVal = macdLine[i];
        double sVal = signalEma;
        double hist = mVal - sVal;

        result.timestamps.push_back(macdTimestamps[i]);
        result.macd.push_back(mVal);
        result.signal.push_back(sVal);
        result.histogram.push_back(hist);
    }

    return result;
}

BollingerBandsResult TechnicalAnalysisEngine::calculateBollingerBands(const std::vector<Candle>& candles, int period, double stdDevMultiplier) {
    BollingerBandsResult result;
    if ((int)candles.size() < period || period <= 0) return result;

    std::vector<double> closes;
    closes.reserve(candles.size());
    for (const auto& c : candles) closes.push_back(c.close);

    auto sma = calculateSMA(candles, period);
    result.timestamps = sma.timestamps;
    result.middle = sma.values;

    result.upper.reserve(sma.values.size());
    result.lower.reserve(sma.values.size());
    result.bandwidth.reserve(sma.values.size());
    result.percentB.reserve(sma.values.size());

    for (size_t i = 0; i < sma.values.size(); ++i) {
        size_t startIdx = i; // in closes
        double mean = sma.values[i];
        double stdDev = calculateStdDev(closes, startIdx, period, mean);

        double up = mean + (stdDevMultiplier * stdDev);
        double low = mean - (stdDevMultiplier * stdDev);
        double bw = (mean > 0.0) ? ((up - low) / mean * 100.0) : 0.0;
        double currentClose = closes[startIdx + period - 1];
        double pB = (up != low) ? ((currentClose - low) / (up - low)) : 0.5;

        result.upper.push_back(up);
        result.lower.push_back(low);
        result.bandwidth.push_back(bw);
        result.percentB.push_back(pB);
    }

    return result;
}

IndicatorSeries TechnicalAnalysisEngine::calculateATR(const std::vector<Candle>& candles, int period) {
    IndicatorSeries result;
    if ((int)candles.size() <= period || period <= 0) return result;

    std::vector<double> trs;
    trs.reserve(candles.size() - 1);

    for (size_t i = 1; i < candles.size(); ++i) {
        double hl = candles[i].high - candles[i].low;
        double hc = std::abs(candles[i].high - candles[i - 1].close);
        double lc = std::abs(candles[i].low - candles[i - 1].close);
        double tr = std::max({hl, hc, lc});
        trs.push_back(tr);
    }

    // Initial ATR (SMA of TR)
    double currentAtr = 0.0;
    for (int i = 0; i < period; ++i) {
        currentAtr += trs[i];
    }
    currentAtr /= period;

    result.timestamps.push_back(candles[period].timestamp);
    result.values.push_back(currentAtr);

    // Wilder's smoothing
    for (size_t i = period; i < trs.size(); ++i) {
        currentAtr = (currentAtr * (period - 1) + trs[i]) / period;
        result.timestamps.push_back(candles[i + 1].timestamp);
        result.values.push_back(currentAtr);
    }

    return result;
}

VWAPResult TechnicalAnalysisEngine::calculateVWAP(const std::vector<Candle>& candles, double stdDevMultiplier) {
    VWAPResult result;
    if (candles.empty()) return result;

    result.timestamps.reserve(candles.size());
    result.vwap.reserve(candles.size());
    result.upperBand.reserve(candles.size());
    result.lowerBand.reserve(candles.size());

    double cumVolume = 0.0;
    double cumTypicalVol = 0.0;
    double cumTypicalVolSq = 0.0;

    for (const auto& c : candles) {
        double typicalPrice = (c.high + c.low + c.close) / 3.0;
        double vol = (c.volume > 0.0) ? c.volume : 1.0;

        cumVolume += vol;
        cumTypicalVol += typicalPrice * vol;
        cumTypicalVolSq += (typicalPrice * typicalPrice) * vol;

        double vwapVal = cumTypicalVol / cumVolume;
        double variance = (cumTypicalVolSq / cumVolume) - (vwapVal * vwapVal);
        double stdDev = (variance > 0.0) ? std::sqrt(variance) : 0.0;

        result.timestamps.push_back(c.timestamp);
        result.vwap.push_back(vwapVal);
        result.upperBand.push_back(vwapVal + (stdDevMultiplier * stdDev));
        result.lowerBand.push_back(vwapVal - (stdDevMultiplier * stdDev));
    }

    return result;
}

SuperTrendResult TechnicalAnalysisEngine::calculateSuperTrend(const std::vector<Candle>& candles, int atrPeriod, double multiplier) {
    SuperTrendResult result;
    if ((int)candles.size() < atrPeriod || atrPeriod <= 0) return result;

    auto atr = calculateATR(candles, atrPeriod);
    if (atr.values.empty()) return result;

    size_t N = candles.size();
    size_t atrOffset = N - atr.values.size();

    result.timestamps.resize(atr.values.size());
    result.superTrend.resize(atr.values.size());
    result.direction.resize(atr.values.size());

    double prevUpper = 0.0;
    double prevLower = 0.0;
    double prevSuperTrend = 0.0;
    int prevDir = 1; // +1 Bullish, -1 Bearish

    for (size_t i = 0; i < atr.values.size(); ++i) {
        size_t cIdx = atrOffset + i;
        const auto& c = candles[cIdx];
        double currentAtr = atr.values[i];
        double hl2 = (c.high + c.low) / 2.0;

        double basicUpper = hl2 + (multiplier * currentAtr);
        double basicLower = hl2 - (multiplier * currentAtr);

        double finalUpper = basicUpper;
        double finalLower = basicLower;

        if (i > 0) {
            double prevClose = candles[cIdx - 1].close;
            if (basicUpper < prevUpper || prevClose > prevUpper) {
                finalUpper = basicUpper;
            } else {
                finalUpper = prevUpper;
            }

            if (basicLower > prevLower || prevClose < prevLower) {
                finalLower = basicLower;
            } else {
                finalLower = prevLower;
            }
        }

        int currentDir = prevDir;
        if (prevDir == 1 && c.close < finalLower) {
            currentDir = -1;
        } else if (prevDir == -1 && c.close > finalUpper) {
            currentDir = 1;
        }

        double superTrendVal = (currentDir == 1) ? finalLower : finalUpper;

        result.timestamps[i] = c.timestamp;
        result.superTrend[i] = superTrendVal;
        result.direction[i] = currentDir;

        prevUpper = finalUpper;
        prevLower = finalLower;
        prevSuperTrend = superTrendVal;
        prevDir = currentDir;
    }

    return result;
}

StochasticResult TechnicalAnalysisEngine::calculateStochastic(const std::vector<Candle>& candles, int kPeriod, int dPeriod, int smoothK) {
    StochasticResult result;
    if ((int)candles.size() < kPeriod + smoothK + dPeriod || kPeriod <= 0) return result;

    std::vector<double> rawK;
    std::vector<int64_t> rawTimestamps;
    rawK.reserve(candles.size() - kPeriod + 1);
    rawTimestamps.reserve(candles.size() - kPeriod + 1);

    for (size_t i = kPeriod - 1; i < candles.size(); ++i) {
        double highestHigh = candles[i].high;
        double lowestLow = candles[i].low;

        for (int k = 0; k < kPeriod; ++k) {
            highestHigh = std::max(highestHigh, candles[i - k].high);
            lowestLow = std::min(lowestLow, candles[i - k].low);
        }

        double denom = highestHigh - lowestLow;
        double val = (denom > 0.0) ? ((candles[i].close - lowestLow) / denom) * 100.0 : 50.0;
        rawK.push_back(val);
        rawTimestamps.push_back(candles[i].timestamp);
    }

    // Smooth %K
    std::vector<double> smoothedK;
    smoothedK.reserve(rawK.size() - smoothK + 1);
    for (size_t i = smoothK - 1; i < rawK.size(); ++i) {
        double sum = 0.0;
        for (int s = 0; s < smoothK; ++s) {
            sum += rawK[i - s];
        }
        smoothedK.push_back(sum / smoothK);
    }

    // Calculate %D (SMA of smoothed %K)
    if ((int)smoothedK.size() < dPeriod) return result;

    size_t finalCount = smoothedK.size() - dPeriod + 1;
    result.timestamps.reserve(finalCount);
    result.k.reserve(finalCount);
    result.d.reserve(finalCount);

    size_t tsOffset = rawTimestamps.size() - finalCount;

    for (size_t i = dPeriod - 1; i < smoothedK.size(); ++i) {
        double sumD = 0.0;
        for (int d = 0; d < dPeriod; ++d) {
            sumD += smoothedK[i - d];
        }
        result.timestamps.push_back(rawTimestamps[tsOffset + (i - dPeriod + 1)]);
        result.k.push_back(smoothedK[i]);
        result.d.push_back(sumD / dPeriod);
    }

    return result;
}

std::vector<CandlestickPattern> TechnicalAnalysisEngine::detectCandlestickPatterns(const std::vector<Candle>& candles) {
    std::vector<CandlestickPattern> patterns;
    if (candles.size() < 3) return patterns;

    for (size_t i = 2; i < candles.size(); ++i) {
        const auto& c = candles[i];
        const auto& prev = candles[i - 1];
        const auto& prev2 = candles[i - 2];

        double body = std::abs(c.close - c.open);
        double range = c.high - c.low;
        double upperWick = c.high - std::max(c.open, c.close);
        double lowerWick = std::min(c.open, c.close) - c.low;

        // 1. Doji (Body < 10% of range)
        if (range > 0 && (body / range) < 0.10) {
            patterns.push_back({"Doji", "INDECISION", i, c.timestamp, c.close, "Indecision market state with neutral pressure"});
        }

        // 2. Hammer (Lower wick >= 2x body, tiny upper wick)
        if (body > 0 && lowerWick >= 2.0 * body && upperWick <= body * 0.3) {
            patterns.push_back({"Hammer", "BULLISH", i, c.timestamp, c.close, "Bullish rejection of lower prices"});
        }

        // 3. Shooting Star / Inverted Hammer (Upper wick >= 2x body, tiny lower wick)
        if (body > 0 && upperWick >= 2.0 * body && lowerWick <= body * 0.3) {
            patterns.push_back({"Shooting Star", "BEARISH", i, c.timestamp, c.close, "Bearish rejection of higher prices"});
        }

        // 4. Bullish Engulfing (Prev red candle engulfed by larger green candle)
        if (prev.close < prev.open && c.close > c.open && c.open <= prev.close && c.close >= prev.open) {
            patterns.push_back({"Bullish Engulfing", "BULLISH", i, c.timestamp, c.close, "Strong buyer momentum reversal"});
        }

        // 5. Bearish Engulfing (Prev green candle engulfed by larger red candle)
        if (prev.close > prev.open && c.close < c.open && c.open >= prev.close && c.close <= prev.open) {
            patterns.push_back({"Bearish Engulfing", "BEARISH", i, c.timestamp, c.close, "Strong seller distribution reversal"});
        }

        // 6. Morning Star (3-candle bullish reversal)
        double prevBody = std::abs(prev.close - prev.open);
        double prev2Body = std::abs(prev2.close - prev2.open);
        if (prev2.close < prev2.open && prevBody < prev2Body * 0.3 && c.close > c.open && c.close > (prev2.open + prev2.close) / 2.0) {
            patterns.push_back({"Morning Star", "BULLISH", i, c.timestamp, c.close, "High-probability bottom reversal pattern"});
        }
    }

    return patterns;
}

double TechnicalAnalysisEngine::calculatePearsonCorrelation(const std::vector<double>& a, const std::vector<double>& b) {
    if (a.size() != b.size() || a.size() < 2) return 0.0;

    size_t N = a.size();
    double sumA = 0.0, sumB = 0.0;
    for (size_t i = 0; i < N; ++i) {
        sumA += a[i];
        sumB += b[i];
    }
    double meanA = sumA / N;
    double meanB = sumB / N;

    double num = 0.0, denomA = 0.0, denomB = 0.0;
    for (size_t i = 0; i < N; ++i) {
        double diffA = a[i] - meanA;
        double diffB = b[i] - meanB;
        num += diffA * diffB;
        denomA += diffA * diffA;
        denomB += diffB * diffB;
    }

    double denom = std::sqrt(denomA * denomB);
    if (denom <= 0.0) return 0.0;
    double r = num / denom;
    return std::clamp(r, -1.0, 1.0);
}

CorrelationMatrix TechnicalAnalysisEngine::calculateCorrelationMatrix(
    const std::vector<std::string>& symbols, 
    const std::unordered_map<std::string, std::vector<Candle>>& marketCandles) 
{
    CorrelationMatrix matrix;
    matrix.symbols = symbols;
    size_t S = symbols.size();
    matrix.correlations.assign(S, std::vector<double>(S, 1.0));

    // Extract return series
    std::vector<std::vector<double>> returns(S);
    for (size_t i = 0; i < S; ++i) {
        auto it = marketCandles.find(symbols[i]);
        if (it != marketCandles.end() && it->second.size() >= 2) {
            const auto& candles = it->second;
            returns[i].reserve(candles.size() - 1);
            for (size_t k = 1; k < candles.size(); ++k) {
                double ret = (candles[k].close - candles[k - 1].close) / candles[k - 1].close;
                returns[i].push_back(ret);
            }
        }
    }

    for (size_t i = 0; i < S; ++i) {
        for (size_t j = i + 1; j < S; ++j) {
            if (!returns[i].empty() && !returns[j].empty()) {
                size_t minLen = std::min(returns[i].size(), returns[j].size());
                std::vector<double> subA(returns[i].end() - minLen, returns[i].end());
                std::vector<double> subB(returns[j].end() - minLen, returns[j].end());
                double corr = calculatePearsonCorrelation(subA, subB);
                matrix.correlations[i][j] = corr;
                matrix.correlations[j][i] = corr;
            }
        }
    }

    return matrix;
}

MultiTimeframeConfluence TechnicalAnalysisEngine::calculateMultiTimeframeConfluence(
    const std::string& symbol, 
    const std::unordered_map<std::string, std::vector<Candle>>& tfCandles) 
{
    MultiTimeframeConfluence confluence;
    confluence.symbol = symbol;

    std::vector<std::string> standardTfs = {"1m", "5m", "15m", "1h", "4h", "1d"};
    double totalWeight = 0.0;
    double weightedScore = 0.0;

    for (const auto& tf : standardTfs) {
        auto it = tfCandles.find(tf);
        if (it == tfCandles.end() || it->second.size() < 14) continue;

        auto summary = generateSummary(it->second);
        TimeframeSignal sig;
        sig.timeframe = tf;
        sig.trend = summary.trend;
        sig.rsi = summary.rsi14;
        sig.macd = (summary.macdLine > summary.macdSignal) ? "BULLISH" : "BEARISH";

        double tfScore = 50.0;
        if (summary.trend == "BULLISH") tfScore += 25.0;
        else if (summary.trend == "BEARISH") tfScore -= 25.0;

        if (summary.rsi14 > 50.0) tfScore += 15.0;
        else tfScore -= 15.0;

        if (sig.macd == "BULLISH") tfScore += 10.0;
        else tfScore -= 10.0;

        sig.score = std::clamp(tfScore, 0.0, 100.0);
        confluence.signals.push_back(sig);

        // Higher weight for higher timeframes
        double w = 1.0;
        if (tf == "1h") w = 2.0;
        else if (tf == "4h") w = 3.0;
        else if (tf == "1d") w = 4.0;

        weightedScore += sig.score * w;
        totalWeight += w;
    }

    if (totalWeight > 0.0) {
        confluence.confluenceScore = weightedScore / totalWeight;
    } else {
        confluence.confluenceScore = 50.0;
    }

    if (confluence.confluenceScore >= 75.0) {
        confluence.overallBias = "STRONG_BUY";
        confluence.recommendedAction = "High-confidence long confluence across macro and intraday horizons.";
    } else if (confluence.confluenceScore >= 60.0) {
        confluence.overallBias = "BUY";
        confluence.recommendedAction = "Bullish momentum aligned on primary timeframes.";
    } else if (confluence.confluenceScore <= 25.0) {
        confluence.overallBias = "STRONG_SELL";
        confluence.recommendedAction = "Severe breakdown across multi-timeframe trend structures.";
    } else if (confluence.confluenceScore <= 40.0) {
        confluence.overallBias = "SELL";
        confluence.recommendedAction = "Bearish bias confirmed by intraday indicators.";
    } else {
        confluence.overallBias = "NEUTRAL";
        confluence.recommendedAction = "Conflicting signals between timeframes; range-bound consolidation.";
    }

    return confluence;
}

TechnicalSummary TechnicalAnalysisEngine::generateSummary(const std::vector<Candle>& candles) {
    TechnicalSummary summary;
    if (candles.empty()) return summary;

    summary.currentPrice = candles.back().close;

    // Calculate SMA & EMA
    auto sma20 = calculateSMA(candles, 20);
    auto sma50 = calculateSMA(candles, 50);
    auto sma200 = calculateSMA(candles, 200);

    auto ema20 = calculateEMA(candles, 20);
    auto ema50 = calculateEMA(candles, 50);
    auto ema200 = calculateEMA(candles, 200);

    if (!sma20.values.empty()) summary.sma20 = sma20.values.back();
    if (!sma50.values.empty()) summary.sma50 = sma50.values.back();
    if (!sma200.values.empty()) summary.sma200 = sma200.values.back();

    if (!ema20.values.empty()) summary.ema20 = ema20.values.back();
    if (!ema50.values.empty()) summary.ema50 = ema50.values.back();
    if (!ema200.values.empty()) summary.ema200 = ema200.values.back();

    // RSI
    auto rsi = calculateRSI(candles, 14);
    if (!rsi.values.empty()) {
        summary.rsi14 = rsi.values.back();
        if (summary.rsi14 <= 30.0) summary.rsiState = "OVERSOLD";
        else if (summary.rsi14 >= 70.0) summary.rsiState = "OVERBOUGHT";
        else summary.rsiState = "NEUTRAL";
    }

    // MACD
    auto macd = calculateMACD(candles, 12, 26, 9);
    if (!macd.macd.empty()) {
        summary.macdLine = macd.macd.back();
        summary.macdSignal = macd.signal.back();
        summary.macdHist = macd.histogram.back();

        if (macd.histogram.size() >= 2) {
            double prevHist = macd.histogram[macd.histogram.size() - 2];
            double currHist = macd.histogram.back();
            if (prevHist <= 0.0 && currHist > 0.0) summary.macdCross = "BULLISH_CROSS";
            else if (prevHist >= 0.0 && currHist < 0.0) summary.macdCross = "BEARISH_CROSS";
        }
    }

    // Bollinger Bands
    auto bb = calculateBollingerBands(candles, 20, 2.0);
    if (!bb.middle.empty()) {
        summary.bbUpper = bb.upper.back();
        summary.bbMiddle = bb.middle.back();
        summary.bbLower = bb.lower.back();
    }

    // ATR
    auto atr = calculateATR(candles, 14);
    if (!atr.values.empty()) {
        summary.atr14 = atr.values.back();
    }

    // VWAP
    auto vwap = calculateVWAP(candles);
    if (!vwap.vwap.empty()) {
        summary.vwap = vwap.vwap.back();
    }

    // SuperTrend
    auto st = calculateSuperTrend(candles);
    if (!st.superTrend.empty()) {
        summary.superTrend = st.superTrend.back();
        summary.superTrendDirection = st.direction.back();
    }

    // Stochastic
    auto stoch = calculateStochastic(candles);
    if (!stoch.k.empty()) {
        summary.stochK = stoch.k.back();
        summary.stochD = stoch.d.back();
    }

    // Trend assessment
    if (summary.ema20 > 0 && summary.ema50 > 0) {
        if (summary.currentPrice > summary.ema20 && summary.ema20 > summary.ema50) {
            summary.trend = "BULLISH";
        } else if (summary.currentPrice < summary.ema20 && summary.ema20 < summary.ema50) {
            summary.trend = "BEARISH";
        } else {
            summary.trend = "NEUTRAL";
        }
    }

    return summary;
}

} // namespace crypto
