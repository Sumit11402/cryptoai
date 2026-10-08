#include "core/Config.hpp"
#include "core/Logger.hpp"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <algorithm>

namespace crypto {

Config& Config::instance() {
    static Config s_instance;
    return s_instance;
}

Config::Config() {
    // Check standard environment variables first
    const char* envGroq = std::getenv("GROQ_API_KEY");
    if (envGroq) m_groqApiKey = envGroq;

    const char* envCoinGecko = std::getenv("COINGECKO_API_KEY");
    if (envCoinGecko) m_coinGeckoApiKey = envCoinGecko;

    const char* envOllama = std::getenv("OLLAMA_BASE_URL");
    if (envOllama) m_ollamaBaseUrl = envOllama;

    const char* envBinanceKey = std::getenv("BINANCE_API_KEY");
    if (envBinanceKey) m_binanceApiKey = envBinanceKey;

    const char* envBinanceSecret = std::getenv("BINANCE_API_SECRET");
    if (envBinanceSecret) m_binanceApiSecret = envBinanceSecret;

    const char* envTwilioSid = std::getenv("TWILIO_ACCOUNT_SID");
    if (envTwilioSid) m_twilioAccountSid = envTwilioSid;

    const char* envTwilioToken = std::getenv("TWILIO_AUTH_TOKEN");
    if (envTwilioToken) m_twilioAuthToken = envTwilioToken;

    const char* envTwilioFrom = std::getenv("TWILIO_FROM_NUMBER");
    if (envTwilioFrom) m_twilioFromNumber = envTwilioFrom;

    const char* envTwilioTo = std::getenv("TWILIO_TO_NUMBER");
    if (envTwilioTo) m_twilioToNumber = envTwilioTo;
}

void Config::load(const std::string& envPath) {
    parseEnvFile(envPath);
    LOG_INFO("Configuration loaded. Groq configured: " + std::string(m_groqApiKey.empty() ? "NO" : "YES") +
             ", CoinGecko configured: " + std::string(m_coinGeckoApiKey.empty() ? "NO" : "YES") +
             ", Ollama URL: " + m_ollamaBaseUrl + 
             ", Twilio SMS: " + std::string(isTwilioConfigured() ? "ARMED" : "NOT CONFIGURED"));
}

void Config::parseEnvFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        LOG_DEBUG("No .env file found at " + path + ", using environment variables.");
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        if (line.empty() || line[0] == '#') continue;

        auto eqPos = line.find('=');
        if (eqPos != std::string::npos) {
            std::string key = line.substr(0, eqPos);
            std::string val = line.substr(eqPos + 1);

            // Trim key and value
            key.erase(key.find_last_not_of(" \t\r\n") + 1);
            val.erase(0, val.find_first_not_of(" \t\r\n"));
            val.erase(val.find_last_not_of(" \t\r\n") + 1);

            // Remove quotes if present
            if (val.size() >= 2 && ((val.front() == '"' && val.back() == '"') || (val.front() == '\'' && val.back() == '\''))) {
                val = val.substr(1, val.size() - 2);
            }

            std::lock_guard<std::mutex> lock(m_mutex);
            if (key == "GROQ_API_KEY") m_groqApiKey = val;
            else if (key == "COINGECKO_API_KEY") m_coinGeckoApiKey = val;
            else if (key == "OLLAMA_BASE_URL") m_ollamaBaseUrl = val;
            else if (key == "GROQ_MODEL") m_groqModel = val;
            else if (key == "OLLAMA_MODEL") m_ollamaModel = val;
            else if (key == "BINANCE_API_KEY") m_binanceApiKey = val;
            else if (key == "BINANCE_API_SECRET") m_binanceApiSecret = val;
            else if (key == "TWILIO_ACCOUNT_SID") m_twilioAccountSid = val;
            else if (key == "TWILIO_AUTH_TOKEN") m_twilioAuthToken = val;
            else if (key == "TWILIO_FROM_NUMBER") m_twilioFromNumber = val;
            else if (key == "TWILIO_TO_NUMBER") m_twilioToNumber = val;
            else if (key == "DEFAULT_SYMBOL") m_defaultSymbol = val;
            else if (key == "DEFAULT_TIMEFRAME") m_defaultTimeframe = val;
            else if (key == "DEMO_MODE") m_demoMode = (val == "true" || val == "1");
        }
    }
}

void Config::save() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::ofstream file(".env");
    if (!file.is_open()) {
        LOG_ERROR("Failed to open .env for writing settings");
        return;
    }

    file << "# CRYPTØ AI TERMINAL ENVIRONMENT CONFIGURATION\n";
    file << "GROQ_API_KEY=" << m_groqApiKey << "\n";
    file << "COINGECKO_API_KEY=" << m_coinGeckoApiKey << "\n";
    file << "OLLAMA_BASE_URL=" << m_ollamaBaseUrl << "\n";
    file << "GROQ_MODEL=" << m_groqModel << "\n";
    file << "OLLAMA_MODEL=" << m_ollamaModel << "\n\n";
    file << "# Optional Binance Keys (Not required for public market data)\n";
    file << "BINANCE_API_KEY=" << m_binanceApiKey << "\n";
    file << "BINANCE_API_SECRET=" << m_binanceApiSecret << "\n\n";
    file << "# Twilio Mobile SMS / WhatsApp Alert Keys (Free Trial compatible)\n";
    file << "TWILIO_ACCOUNT_SID=" << m_twilioAccountSid << "\n";
    file << "TWILIO_AUTH_TOKEN=" << m_twilioAuthToken << "\n";
    file << "TWILIO_FROM_NUMBER=" << m_twilioFromNumber << "\n";
    file << "TWILIO_TO_NUMBER=" << m_twilioToNumber << "\n\n";
    file << "DEFAULT_SYMBOL=" << m_defaultSymbol << "\n";
    file << "DEFAULT_TIMEFRAME=" << m_defaultTimeframe << "\n";
    file << "DEMO_MODE=" << (m_demoMode ? "true" : "false") << "\n";

    LOG_INFO("Settings successfully saved to .env");
}

std::string Config::getGroqApiKey() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_groqApiKey;
}

void Config::setGroqApiKey(const std::string& key) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_groqApiKey = key;
}

std::string Config::getCoinGeckoApiKey() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_coinGeckoApiKey;
}

void Config::setCoinGeckoApiKey(const std::string& key) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_coinGeckoApiKey = key;
}

std::string Config::getOllamaBaseUrl() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_ollamaBaseUrl;
}

void Config::setOllamaBaseUrl(const std::string& url) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_ollamaBaseUrl = url;
}

std::string Config::getGroqModel() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_groqModel;
}

void Config::setGroqModel(const std::string& model) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_groqModel = model;
}

std::string Config::getOllamaModel() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_ollamaModel;
}

void Config::setOllamaModel(const std::string& model) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_ollamaModel = model;
}

AIProviderType Config::getAIProvider() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_aiProvider;
}

void Config::setAIProvider(AIProviderType provider) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_aiProvider = provider;
}

std::string Config::getBinanceRestUrl() const { return m_binanceRestUrl; }
std::string Config::getBinanceWsUrl() const { return m_binanceWsUrl; }

std::string Config::getBinanceApiKey() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_binanceApiKey;
}

void Config::setBinanceApiKey(const std::string& key) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_binanceApiKey = key;
}

std::string Config::getBinanceApiSecret() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_binanceApiSecret;
}

void Config::setBinanceApiSecret(const std::string& secret) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_binanceApiSecret = secret;
}

std::string Config::getDefaultSymbol() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_defaultSymbol;
}

void Config::setDefaultSymbol(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_defaultSymbol = symbol;
}

std::string Config::getDefaultTimeframe() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_defaultTimeframe;
}

void Config::setDefaultTimeframe(const std::string& timeframe) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_defaultTimeframe = timeframe;
}

bool Config::isDemoMode() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_demoMode;
}

void Config::setDemoMode(bool demo) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_demoMode = demo;
}

bool Config::isNotificationsEnabled() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_notificationsEnabled;
}

void Config::setNotificationsEnabled(bool enabled) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_notificationsEnabled = enabled;
}

int Config::getRefreshIntervalMs() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_refreshIntervalMs;
}

void Config::setRefreshIntervalMs(int ms) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_refreshIntervalMs = ms;
}

std::string Config::getTwilioAccountSid() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_twilioAccountSid;
}

void Config::setTwilioAccountSid(const std::string& sid) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_twilioAccountSid = sid;
}

std::string Config::getTwilioAuthToken() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_twilioAuthToken;
}

void Config::setTwilioAuthToken(const std::string& token) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_twilioAuthToken = token;
}

std::string Config::getTwilioFromNumber() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_twilioFromNumber;
}

void Config::setTwilioFromNumber(const std::string& from) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_twilioFromNumber = from;
}

std::string Config::getTwilioToNumber() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_twilioToNumber;
}

void Config::setTwilioToNumber(const std::string& to) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_twilioToNumber = to;
}

bool Config::isTwilioConfigured() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return !m_twilioAccountSid.empty() && !m_twilioAuthToken.empty() && 
           !m_twilioFromNumber.empty() && !m_twilioToNumber.empty();
}

std::string Config::maskKey(const std::string& key) {
    if (key.empty()) return "NOT SET";
    if (key.size() <= 8) return "••••••••";
    return key.substr(0, 4) + "••••••••••••" + key.substr(key.size() - 4);
}

} // namespace crypto
