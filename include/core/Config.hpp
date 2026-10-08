#pragma once

#include <string>
#include <mutex>
#include <nlohmann/json.hpp>
#include "core/Types.hpp"

namespace crypto {

class Config {
public:
    static Config& instance();

    void load(const std::string& envPath = ".env");
    void save();

    // AI Provider Config
    std::string getGroqApiKey() const;
    void setGroqApiKey(const std::string& key);

    std::string getOllamaBaseUrl() const;
    void setOllamaBaseUrl(const std::string& url);

    std::string getGroqModel() const;
    void setGroqModel(const std::string& model);

    std::string getOllamaModel() const;
    void setOllamaModel(const std::string& model);

    AIProviderType getAIProvider() const;
    void setAIProvider(AIProviderType provider);

    // CoinGecko Market Data API
    std::string getCoinGeckoApiKey() const;
    void setCoinGeckoApiKey(const std::string& key);

    // Binance Public Data & optional future live trading
    std::string getBinanceRestUrl() const;
    std::string getBinanceWsUrl() const;
    std::string getBinanceApiKey() const;
    void setBinanceApiKey(const std::string& key);
    std::string getBinanceApiSecret() const;
    void setBinanceApiSecret(const std::string& secret);

    // Twilio SMS & WhatsApp Mobile Alert Config (100% Free Trial compatible)
    std::string getTwilioAccountSid() const;
    void setTwilioAccountSid(const std::string& sid);
    std::string getTwilioAuthToken() const;
    void setTwilioAuthToken(const std::string& token);
    std::string getTwilioFromNumber() const;
    void setTwilioFromNumber(const std::string& from);
    std::string getTwilioToNumber() const;
    void setTwilioToNumber(const std::string& to);
    bool isTwilioConfigured() const;

    // Application State
    std::string getDefaultSymbol() const;
    void setDefaultSymbol(const std::string& symbol);

    std::string getDefaultTimeframe() const;
    void setDefaultTimeframe(const std::string& timeframe);

    bool isDemoMode() const;
    void setDemoMode(bool demo);

    bool isNotificationsEnabled() const;
    void setNotificationsEnabled(bool enabled);

    int getRefreshIntervalMs() const;
    void setRefreshIntervalMs(int ms);

    // Security Helpers
    static std::string maskKey(const std::string& key);

private:
    Config();
    ~Config() = default;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    void parseEnvFile(const std::string& path);

    mutable std::mutex m_mutex;
    std::string m_groqApiKey;
    std::string m_coinGeckoApiKey;
    std::string m_ollamaBaseUrl{"http://127.0.0.1:11434"};
    std::string m_groqModel{"llama-3.3-70b-versatile"};
    std::string m_ollamaModel{"llama3.2"};
    AIProviderType m_aiProvider{AIProviderType::AUTO};

    std::string m_binanceRestUrl{"https://api.binance.com"};
    std::string m_binanceWsUrl{"wss://stream.binance.com:9443"};
    std::string m_binanceApiKey;
    std::string m_binanceApiSecret;

    std::string m_twilioAccountSid;
    std::string m_twilioAuthToken;
    std::string m_twilioFromNumber;
    std::string m_twilioToNumber;

    std::string m_defaultSymbol{"BTCUSDT"};
    std::string m_defaultTimeframe{"1h"};
    bool m_demoMode{false};
    bool m_notificationsEnabled{true};
    int m_refreshIntervalMs{1000};
};

} // namespace crypto
