#pragma once

#include "core/Types.hpp"
#include <string>
#include <optional>
#include <functional>

namespace crypto {

enum class AICommandAction {
    ANALYZE_SYMBOL,
    COMPARE_SYMBOLS,
    EXPLAIN_INDICATOR,
    SHOW_PORTFOLIO,
    EXECUTE_PAPER_TRADE,
    SHOW_WATCHLIST,
    SET_ALERT,
    GENERAL_CHAT
};

struct ParsedAICommand {
    AICommandAction action{AICommandAction::GENERAL_CHAT};
    std::string primarySymbol;
    std::string secondarySymbol;
    std::string indicatorName;
    std::string tradeSide;
    double tradeQuantity{0.0};
    std::string promptToSend;
};

class AICommandEngine {
public:
    static ParsedAICommand parse(const std::string& input);
    static bool executeAction(const ParsedAICommand& cmd, std::string& feedbackMsg);
};

} // namespace crypto
