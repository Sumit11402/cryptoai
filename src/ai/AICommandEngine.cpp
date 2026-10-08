#include "ai/AICommandEngine.hpp"
#include "market/MarketManager.hpp"
#include <algorithm>
#include <sstream>

namespace crypto {

ParsedAICommand AICommandEngine::parse(const std::string& input) {
    ParsedAICommand cmd;
    cmd.promptToSend = input;

    std::string lower = input;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    // 1. Analyze / TA command (e.g. "analyze btc", "ta eth", "analyze solusdt")
    if (lower.rfind("analyze ", 0) == 0 || lower.rfind("ta ", 0) == 0) {
        std::istringstream iss(lower);
        std::string verb, sym;
        iss >> verb >> sym;
        if (!sym.empty()) {
            std::transform(sym.begin(), sym.end(), sym.begin(), ::toupper);
            if (sym.find("USDT") == std::string::npos) sym += "USDT";
            cmd.action = AICommandAction::ANALYZE_SYMBOL;
            cmd.primarySymbol = sym;
            cmd.promptToSend = "Perform a detailed multi-timeframe quantitative technical analysis on " + sym + 
                               ", evaluating momentum oscillators, moving averages, and breakout levels.";
            return cmd;
        }
    }

    // 2. Compare command (e.g. "compare btc and eth", "compare btc eth")
    if (lower.rfind("compare ", 0) == 0) {
        std::string rest = lower.substr(8);
        std::replace(rest.begin(), rest.end(), ',', ' ');
        std::istringstream iss(rest);
        std::string sym1, sep, sym2;
        iss >> sym1;
        if (iss >> sep) {
            if (sep == "and" || sep == "with" || sep == "vs") {
                iss >> sym2;
            } else {
                sym2 = sep;
            }
        }
        if (!sym1.empty() && !sym2.empty()) {
            std::transform(sym1.begin(), sym1.end(), sym1.begin(), ::toupper);
            std::transform(sym2.begin(), sym2.end(), sym2.begin(), ::toupper);
            if (sym1.find("USDT") == std::string::npos) sym1 += "USDT";
            if (sym2.find("USDT") == std::string::npos) sym2 += "USDT";

            cmd.action = AICommandAction::COMPARE_SYMBOLS;
            cmd.primarySymbol = sym1;
            cmd.secondarySymbol = sym2;
            cmd.promptToSend = "Compare the relative strength, trend structure, volatility, and volume profiles of " + 
                               sym1 + " against " + sym2 + ". Which asset currently offers a higher probability setup?";
            return cmd;
        }
    }

    // 3. Explain Indicator (e.g. "explain rsi", "explain macd", "what is bollinger")
    if (lower.find("explain ") != std::string::npos || lower.find("what is ") != std::string::npos) {
        cmd.action = AICommandAction::EXPLAIN_INDICATOR;
        if (lower.find("rsi") != std::string::npos) cmd.indicatorName = "RSI";
        else if (lower.find("macd") != std::string::npos) cmd.indicatorName = "MACD";
        else if (lower.find("bollinger") != std::string::npos) cmd.indicatorName = "Bollinger Bands";
        else if (lower.find("ema") != std::string::npos) cmd.indicatorName = "EMA";
        else if (lower.find("atr") != std::string::npos) cmd.indicatorName = "ATR";
        else cmd.indicatorName = "technical indicators";

        cmd.promptToSend = "Explain the mathematics, market psychology, common failure modes, and best-practice trading applications of " + 
                           cmd.indicatorName + ". Include examples of divergence and false signals.";
        return cmd;
    }

    // 4. Portfolio commands
    if (lower.find("portfolio") != std::string::npos || lower.find("my pnl") != std::string::npos || lower.find("my balance") != std::string::npos) {
        cmd.action = AICommandAction::SHOW_PORTFOLIO;
        return cmd;
    }

    // 5. Watchlist command
    if (lower.find("watchlist") != std::string::npos) {
        cmd.action = AICommandAction::SHOW_WATCHLIST;
        return cmd;
    }

    // Default: general research prompt
    return cmd;
}

bool AICommandEngine::executeAction(const ParsedAICommand& cmd, std::string& feedbackMsg) {
    if (cmd.action == AICommandAction::ANALYZE_SYMBOL && !cmd.primarySymbol.empty()) {
        MarketManager::instance().setActiveSymbol(cmd.primarySymbol);
        feedbackMsg = "Switched active chart to " + cmd.primarySymbol + " and injected live context.";
        return true;
    }
    return false;
}

} // namespace crypto
