#pragma once

#include "core/Types.hpp"
#include <string>
#include <vector>
#include <mutex>
#include <sqlite3.h>

namespace crypto {

class Database {
public:
    static Database& instance();

    bool init(const std::string& dbPath = "crypto_terminal.db");
    void close();

    // Conversations & Messages
    bool saveConversation(const Conversation& conv);
    std::vector<Conversation> getConversations(int limit = 50);
    Conversation getConversation(const std::string& convId);
    bool deleteConversation(const std::string& convId);
    bool addMessage(const std::string& convId, const AIMessage& msg);
    void clearAllConversations();

    // Portfolio
    bool savePortfolioAsset(const PortfolioAsset& asset);
    bool removePortfolioAsset(const std::string& symbol);
    std::vector<PortfolioAsset> getPortfolioAssets();

    // Paper Trades
    bool savePaperOrder(const PaperOrder& order);
    std::vector<PaperOrder> getPaperOrders(int limit = 100);
    bool clearPaperOrders();

    // Watchlist
    bool addToWatchlist(const std::string& symbol);
    bool removeFromWatchlist(const std::string& symbol);
    std::vector<std::string> getWatchlist();

    // Alerts
    bool saveAlert(const AlertRule& alert);
    bool updateAlert(const AlertRule& alert);
    bool deleteAlert(const std::string& alertId);
    std::vector<AlertRule> getAlerts();

    // Settings
    bool setSetting(const std::string& key, const std::string& value);
    std::string getSetting(const std::string& key, const std::string& defaultValue = "");

private:
    Database() = default;
    ~Database();
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    void createTables();

    sqlite3* m_db{nullptr};
    mutable std::mutex m_mutex;
    bool m_initialized{false};
};

} // namespace crypto
