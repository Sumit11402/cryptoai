#include "analysis/AlertManager.hpp"
#include "storage/Database.hpp"
#include "core/Logger.hpp"
#include "core/EventBus.hpp"
#include "core/Config.hpp"
#include "network/HttpClient.hpp"
#include <chrono>
#include <sstream>
#include <iomanip>

namespace crypto {

static std::string base64Encode(const std::string& input) {
    static const char* b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    int val = 0, valb = -6;
    for (unsigned char c : input) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(b64[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back(b64[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}

static std::string urlEncode(const std::string& s) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;
    for (char c : s) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~' || c == '+') {
            escaped << c;
        } else {
            escaped << '%' << std::setw(2) << static_cast<int>(static_cast<unsigned char>(c));
        }
    }
    return escaped.str();
}

static void dispatchTwilioSMS(const std::string& smsText) {
    if (!Config::instance().isTwilioConfigured()) return;

    std::string sid = Config::instance().getTwilioAccountSid();
    std::string token = Config::instance().getTwilioAuthToken();
    std::string from = Config::instance().getTwilioFromNumber();
    std::string to = Config::instance().getTwilioToNumber();

    std::string url = "https://api.twilio.com/2010-04-01/Accounts/" + sid + "/Messages.json";
    std::unordered_map<std::string, std::string> headers = {
        {"Authorization", "Basic " + base64Encode(sid + ":" + token)},
        {"Content-Type", "application/x-www-form-urlencoded"}
    };

    std::string body = "To=" + urlEncode(to) + "&From=" + urlEncode(from) + "&Body=" + urlEncode(smsText);

    HttpClient::instance().postAsync(url, body, [smsText](const HttpResponse& resp) {
        if (resp.isSuccess()) {
            LOG_INFO("📱 Twilio Mobile Alert sent successfully: " + smsText);
        } else {
            LOG_WARN("Twilio SMS Dispatch failed (HTTP " + std::to_string(resp.statusCode) + "): " + resp.body);
        }
    }, headers, 10);
}

AlertManager& AlertManager::instance() {
    static AlertManager s_instance;
    return s_instance;
}

void AlertManager::init() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_alerts = Database::instance().getAlerts();
    if (m_alerts.empty()) {
        // Add sample default alerts
        AlertRule a1;
        a1.id = "ALT-BTC-100K";
        a1.symbol = "BTCUSDT";
        a1.condition = AlertCondition::PRICE_ABOVE;
        a1.targetValue = 100000.0;
        a1.message = "BTC breaking 6-figure ATH milestone!";
        a1.isActive = true;
        a1.isTriggered = false;
        a1.createdAt = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        m_alerts.push_back(a1);
        Database::instance().saveAlert(a1);

        AlertRule a2;
        a2.id = "ALT-ETH-RSI-30";
        a2.symbol = "ETHUSDT";
        a2.condition = AlertCondition::RSI_BELOW;
        a2.targetValue = 30.0;
        a2.message = "ETH RSI Oversold condition reached.";
        a2.isActive = true;
        a2.isTriggered = false;
        a2.createdAt = a1.createdAt;
        m_alerts.push_back(a2);
        Database::instance().saveAlert(a2);
    }
    LOG_INFO("AlertManager initialized with " + std::to_string(m_alerts.size()) + " alert rules.");
}

std::string AlertManager::generateAlertId() {
    auto now = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return "ALT-" + std::to_string(now);
}

void AlertManager::addAlert(const std::string& symbol, AlertCondition condition, double targetValue, const std::string& customMsg) {
    AlertRule alert;
    alert.id = generateAlertId();
    alert.symbol = symbol;
    alert.condition = condition;
    alert.targetValue = targetValue;
    alert.message = customMsg.empty() ? ("Alert for " + symbol) : customMsg;
    alert.isActive = true;
    alert.isTriggered = false;
    alert.createdAt = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_alerts.push_back(alert);
    }
    Database::instance().saveAlert(alert);
    LOG_INFO("New alert added: " + alert.id + " (" + symbol + ")");
}

void AlertManager::removeAlert(const std::string& alertId) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_alerts.erase(std::remove_if(m_alerts.begin(), m_alerts.end(), 
            [&alertId](const AlertRule& a) { return a.id == alertId; }), m_alerts.end());
    }
    Database::instance().deleteAlert(alertId);
    LOG_INFO("Removed alert: " + alertId);
}

void AlertManager::toggleAlert(const std::string& alertId, bool active) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& a : m_alerts) {
        if (a.id == alertId) {
            a.isActive = active;
            Database::instance().saveAlert(a);
            break;
        }
    }
}

std::vector<AlertRule> AlertManager::getAlerts() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_alerts;
}

std::vector<AlertRule> AlertManager::getTriggeredAlerts() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_triggeredHistory;
}

void AlertManager::triggerAlert(AlertRule& alert, double triggerValue, const std::string& reason) {
    alert.isTriggered = true;
    alert.triggeredAt = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    m_triggeredHistory.insert(m_triggeredHistory.begin(), alert);
    Database::instance().saveAlert(alert);

    EventBus::instance().publish(AlertTriggeredEvent{alert, triggerValue, reason});
    LOG_INFO("🚨 ALERT TRIGGERED: " + alert.symbol + " - " + reason);

    // Dispatch Mobile SMS / WhatsApp Alert via Twilio (if configured)
    std::string smsText = "🚨 [CRYPTØ ALERT] " + alert.symbol + ": " + reason;
    if (!alert.message.empty()) {
        smsText += " | Note: " + alert.message;
    }
    dispatchTwilioSMS(smsText);
}

void AlertManager::evaluatePriceUpdate(const std::string& symbol, double currentPrice) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& a : m_alerts) {
        if (!a.isActive || a.isTriggered || a.symbol != symbol) continue;

        if (a.condition == AlertCondition::PRICE_ABOVE && currentPrice >= a.targetValue) {
            triggerAlert(a, currentPrice, "Price crossed ABOVE $" + std::to_string(a.targetValue));
        } else if (a.condition == AlertCondition::PRICE_BELOW && currentPrice <= a.targetValue) {
            triggerAlert(a, currentPrice, "Price dropped BELOW $" + std::to_string(a.targetValue));
        }
    }
}

void AlertManager::evaluateTechnicalUpdate(const std::string& symbol, const TechnicalSummary& summary) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& a : m_alerts) {
        if (!a.isActive || a.isTriggered || a.symbol != symbol) continue;

        if (a.condition == AlertCondition::RSI_ABOVE && summary.rsi14 >= a.targetValue) {
            triggerAlert(a, summary.rsi14, "RSI crossed ABOVE " + std::to_string(a.targetValue));
        } else if (a.condition == AlertCondition::RSI_BELOW && summary.rsi14 <= a.targetValue) {
            triggerAlert(a, summary.rsi14, "RSI dropped BELOW " + std::to_string(a.targetValue));
        } else if (a.condition == AlertCondition::MACD_BULLISH_CROSS && summary.macdCross == "BULLISH_CROSS") {
            triggerAlert(a, summary.macdHist, "MACD Bullish Crossover detected");
        } else if (a.condition == AlertCondition::MACD_BEARISH_CROSS && summary.macdCross == "BEARISH_CROSS") {
            triggerAlert(a, summary.macdHist, "MACD Bearish Crossover detected");
        }
    }
}

} // namespace crypto
