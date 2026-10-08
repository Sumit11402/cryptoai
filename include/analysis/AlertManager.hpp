#pragma once

#include "core/Types.hpp"
#include <vector>
#include <mutex>
#include <string>

namespace crypto {

class AlertManager {
public:
    static AlertManager& instance();

    void init();
    void addAlert(const std::string& symbol, AlertCondition condition, double targetValue, const std::string& customMsg = "");
    void removeAlert(const std::string& alertId);
    void toggleAlert(const std::string& alertId, bool active);

    std::vector<AlertRule> getAlerts();
    std::vector<AlertRule> getTriggeredAlerts();

    void evaluatePriceUpdate(const std::string& symbol, double currentPrice);
    void evaluateTechnicalUpdate(const std::string& symbol, const TechnicalSummary& summary);

private:
    AlertManager() = default;
    ~AlertManager() = default;

    std::string generateAlertId();
    void triggerAlert(AlertRule& alert, double triggerValue, const std::string& reason);

    mutable std::mutex m_mutex;
    std::vector<AlertRule> m_alerts;
    std::vector<AlertRule> m_triggeredHistory;
};

} // namespace crypto
