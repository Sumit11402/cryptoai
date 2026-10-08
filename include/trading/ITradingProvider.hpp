#pragma once

#include "core/Types.hpp"
#include <string>
#include <vector>

namespace crypto {

class ITradingProvider {
public:
    virtual ~ITradingProvider() = default;

    virtual bool isLiveTradingEnabled() const { return false; } // STRICT DEFAULT: DISABLED
    virtual bool placeOrder(const PaperOrder& order, std::string& outOrderId, std::string& error) = 0;
    virtual bool cancelOrder(const std::string& orderId) = 0;
    virtual std::vector<PaperOrder> getOpenOrders() = 0;
    virtual std::vector<PaperPosition> getPositions() = 0;
    virtual double getAvailableBalance() const = 0;
};

} // namespace crypto
