#include "storage/Database.hpp"
#include "core/Logger.hpp"
#include "core/SystemMetrics.hpp"
#include <chrono>

namespace crypto {

Database& Database::instance() {
    static Database s_instance;
    return s_instance;
}

Database::~Database() {
    close();
}

bool Database::init(const std::string& dbPath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_initialized) return true;

    int rc = sqlite3_open(dbPath.c_str(), &m_db);
    if (rc != SQLITE_OK) {
        LOG_ERROR("Cannot open SQLite database at " + dbPath + ": " + sqlite3_errmsg(m_db));
        return false;
    }

    createTables();
    m_initialized = true;
    LOG_INFO("SQLite database initialized successfully at: " + dbPath);
    return true;
}

void Database::close() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_db) {
        sqlite3_close(m_db);
        m_db = nullptr;
    }
    m_initialized = false;
}

void Database::createTables() {
    const char* schema = R"(
        CREATE TABLE IF NOT EXISTS conversations (
            id TEXT PRIMARY KEY,
            title TEXT,
            created_at INTEGER,
            updated_at INTEGER
        );

        CREATE TABLE IF NOT EXISTS messages (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            conversation_id TEXT,
            role TEXT,
            content TEXT,
            timestamp INTEGER
        );

        CREATE TABLE IF NOT EXISTS portfolio (
            symbol TEXT PRIMARY KEY,
            quantity REAL,
            avg_buy_price REAL,
            updated_at INTEGER
        );

        CREATE TABLE IF NOT EXISTS paper_trades (
            id TEXT PRIMARY KEY,
            symbol TEXT,
            side TEXT,
            type TEXT,
            status TEXT,
            quantity REAL,
            limit_price REAL,
            executed_price REAL,
            fee REAL,
            total_value REAL,
            realized_pnl REAL,
            timestamp INTEGER
        );

        CREATE TABLE IF NOT EXISTS watchlist (
            symbol TEXT PRIMARY KEY,
            added_at INTEGER
        );

        CREATE TABLE IF NOT EXISTS alerts (
            id TEXT PRIMARY KEY,
            symbol TEXT,
            condition TEXT,
            target_value REAL,
            message TEXT,
            is_active INTEGER,
            is_triggered INTEGER,
            created_at INTEGER,
            triggered_at INTEGER
        );

        CREATE TABLE IF NOT EXISTS settings (
            key TEXT PRIMARY KEY,
            value TEXT
        );
    )";

    char* errMsg = nullptr;
    int rc = sqlite3_exec(m_db, schema, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        LOG_ERROR("Error creating database tables: " + std::string(errMsg ? errMsg : "unknown"));
        sqlite3_free(errMsg);
    }
    SystemMetricsCollector::instance().incrementDbQueries();
}

bool Database::saveConversation(const Conversation& conv) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_db) return false;

    const char* sql = "INSERT OR REPLACE INTO conversations (id, title, created_at, updated_at) VALUES (?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, conv.id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, conv.title.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, conv.createdAt);
    sqlite3_bind_int64(stmt, 4, conv.updatedAt);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    SystemMetricsCollector::instance().incrementDbQueries();
    return (rc == SQLITE_DONE);
}

std::vector<Conversation> Database::getConversations(int limit) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<Conversation> results;
    if (!m_db) return results;

    const char* sql = "SELECT id, title, created_at, updated_at FROM conversations ORDER BY updated_at DESC LIMIT ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return results;

    sqlite3_bind_int(stmt, 1, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Conversation c;
        c.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        c.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        c.createdAt = sqlite3_column_int64(stmt, 2);
        c.updatedAt = sqlite3_column_int64(stmt, 3);
        results.push_back(c);
    }
    sqlite3_finalize(stmt);
    SystemMetricsCollector::instance().incrementDbQueries();
    return results;
}

Conversation Database::getConversation(const std::string& convId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    Conversation c;
    if (!m_db) return c;

    // 1. Get metadata
    const char* convSql = "SELECT id, title, created_at, updated_at FROM conversations WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, convSql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, convId.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            c.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            c.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            c.createdAt = sqlite3_column_int64(stmt, 2);
            c.updatedAt = sqlite3_column_int64(stmt, 3);
        }
        sqlite3_finalize(stmt);
    }

    // 2. Get messages
    const char* msgSql = "SELECT role, content, timestamp FROM messages WHERE conversation_id = ? ORDER BY timestamp ASC;";
    if (sqlite3_prepare_v2(m_db, msgSql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, convId.c_str(), -1, SQLITE_TRANSIENT);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            AIMessage m;
            m.role = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            m.content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            m.timestamp = sqlite3_column_int64(stmt, 2);
            c.messages.push_back(m);
        }
        sqlite3_finalize(stmt);
    }
    SystemMetricsCollector::instance().incrementDbQueries();
    return c;
}

bool Database::deleteConversation(const std::string& convId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_db) return false;

    const char* sql1 = "DELETE FROM messages WHERE conversation_id = ?;";
    const char* sql2 = "DELETE FROM conversations WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql1, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, convId.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    if (sqlite3_prepare_v2(m_db, sql2, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, convId.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    SystemMetricsCollector::instance().incrementDbQueries();
    return true;
}

bool Database::addMessage(const std::string& convId, const AIMessage& msg) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_db) return false;

    const char* sql = "INSERT INTO messages (conversation_id, role, content, timestamp) VALUES (?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, convId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, msg.role.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, msg.content.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 4, msg.timestamp);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    // Update conversation timestamp
    const char* updateSql = "UPDATE conversations SET updated_at = ? WHERE id = ?;";
    if (sqlite3_prepare_v2(m_db, updateSql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, msg.timestamp);
        sqlite3_bind_text(stmt, 2, convId.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    SystemMetricsCollector::instance().incrementDbQueries();
    return (rc == SQLITE_DONE);
}

void Database::clearAllConversations() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_db) return;
    sqlite3_exec(m_db, "DELETE FROM messages; DELETE FROM conversations;", nullptr, nullptr, nullptr);
    SystemMetricsCollector::instance().incrementDbQueries();
}

bool Database::savePortfolioAsset(const PortfolioAsset& asset) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_db) return false;

    const char* sql = "INSERT OR REPLACE INTO portfolio (symbol, quantity, avg_buy_price, updated_at) VALUES (?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, asset.symbol.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 2, asset.quantity);
    sqlite3_bind_double(stmt, 3, asset.avgBuyPrice);
    sqlite3_bind_int64(stmt, 4, asset.lastUpdated);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    SystemMetricsCollector::instance().incrementDbQueries();
    return (rc == SQLITE_DONE);
}

bool Database::removePortfolioAsset(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_db) return false;

    const char* sql = "DELETE FROM portfolio WHERE symbol = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, symbol.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    SystemMetricsCollector::instance().incrementDbQueries();
    return (rc == SQLITE_DONE);
}

std::vector<PortfolioAsset> Database::getPortfolioAssets() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<PortfolioAsset> list;
    if (!m_db) return list;

    const char* sql = "SELECT symbol, quantity, avg_buy_price, updated_at FROM portfolio;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        PortfolioAsset a;
        a.symbol = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        a.quantity = sqlite3_column_double(stmt, 1);
        a.avgBuyPrice = sqlite3_column_double(stmt, 2);
        a.lastUpdated = sqlite3_column_int64(stmt, 3);
        list.push_back(a);
    }
    sqlite3_finalize(stmt);
    SystemMetricsCollector::instance().incrementDbQueries();
    return list;
}

bool Database::savePaperOrder(const PaperOrder& order) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_db) return false;

    const char* sql = R"(
        INSERT OR REPLACE INTO paper_trades 
        (id, symbol, side, type, status, quantity, limit_price, executed_price, fee, total_value, realized_pnl, timestamp) 
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
    )";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    std::string sideStr = (order.side == OrderSide::BUY) ? "BUY" : "SELL";
    std::string typeStr = (order.type == OrderType::MARKET) ? "MARKET" : "LIMIT";
    std::string statusStr = (order.status == OrderStatus::FILLED) ? "FILLED" : "OPEN";

    sqlite3_bind_text(stmt, 1, order.orderId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, order.symbol.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, sideStr.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, typeStr.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, statusStr.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 6, order.quantity);
    sqlite3_bind_double(stmt, 7, order.limitPrice);
    sqlite3_bind_double(stmt, 8, order.executedPrice);
    sqlite3_bind_double(stmt, 9, order.fee);
    sqlite3_bind_double(stmt, 10, order.totalValue);
    sqlite3_bind_double(stmt, 11, order.realizedPnL);
    sqlite3_bind_int64(stmt, 12, order.timestamp);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    SystemMetricsCollector::instance().incrementDbQueries();
    return (rc == SQLITE_DONE);
}

std::vector<PaperOrder> Database::getPaperOrders(int limit) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<PaperOrder> list;
    if (!m_db) return list;

    const char* sql = "SELECT id, symbol, side, type, status, quantity, limit_price, executed_price, fee, total_value, realized_pnl, timestamp FROM paper_trades ORDER BY timestamp DESC LIMIT ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;

    sqlite3_bind_int(stmt, 1, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        PaperOrder o;
        o.orderId = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        o.symbol = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        std::string sideStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        o.side = (sideStr == "BUY") ? OrderSide::BUY : OrderSide::SELL;
        std::string typeStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        o.type = (typeStr == "MARKET") ? OrderType::MARKET : OrderType::LIMIT;
        std::string statusStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        o.status = (statusStr == "FILLED") ? OrderStatus::FILLED : OrderStatus::OPEN;
        o.quantity = sqlite3_column_double(stmt, 5);
        o.limitPrice = sqlite3_column_double(stmt, 6);
        o.executedPrice = sqlite3_column_double(stmt, 7);
        o.fee = sqlite3_column_double(stmt, 8);
        o.totalValue = sqlite3_column_double(stmt, 9);
        o.realizedPnL = sqlite3_column_double(stmt, 10);
        o.timestamp = sqlite3_column_int64(stmt, 11);
        list.push_back(o);
    }
    sqlite3_finalize(stmt);
    SystemMetricsCollector::instance().incrementDbQueries();
    return list;
}

bool Database::clearPaperOrders() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_db) return false;
    sqlite3_exec(m_db, "DELETE FROM paper_trades;", nullptr, nullptr, nullptr);
    SystemMetricsCollector::instance().incrementDbQueries();
    return true;
}

bool Database::addToWatchlist(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_db) return false;

    const char* sql = "INSERT OR IGNORE INTO watchlist (symbol, added_at) VALUES (?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    sqlite3_bind_text(stmt, 1, symbol.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, now);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    SystemMetricsCollector::instance().incrementDbQueries();
    return (rc == SQLITE_DONE);
}

bool Database::removeFromWatchlist(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_db) return false;

    const char* sql = "DELETE FROM watchlist WHERE symbol = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, symbol.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    SystemMetricsCollector::instance().incrementDbQueries();
    return (rc == SQLITE_DONE);
}

std::vector<std::string> Database::getWatchlist() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::string> list;
    if (!m_db) return list;

    const char* sql = "SELECT symbol FROM watchlist ORDER BY added_at ASC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        list.push_back(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
    }
    sqlite3_finalize(stmt);
    SystemMetricsCollector::instance().incrementDbQueries();
    return list;
}

bool Database::saveAlert(const AlertRule& alert) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_db) return false;

    const char* sql = R"(
        INSERT OR REPLACE INTO alerts 
        (id, symbol, condition, target_value, message, is_active, is_triggered, created_at, triggered_at) 
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);
    )";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    std::string condStr = std::to_string(static_cast<int>(alert.condition));

    sqlite3_bind_text(stmt, 1, alert.id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, alert.symbol.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, condStr.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 4, alert.targetValue);
    sqlite3_bind_text(stmt, 5, alert.message.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 6, alert.isActive ? 1 : 0);
    sqlite3_bind_int(stmt, 7, alert.isTriggered ? 1 : 0);
    sqlite3_bind_int64(stmt, 8, alert.createdAt);
    sqlite3_bind_int64(stmt, 9, alert.triggeredAt);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    SystemMetricsCollector::instance().incrementDbQueries();
    return (rc == SQLITE_DONE);
}

bool Database::updateAlert(const AlertRule& alert) {
    return saveAlert(alert);
}

bool Database::deleteAlert(const std::string& alertId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_db) return false;

    const char* sql = "DELETE FROM alerts WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, alertId.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    SystemMetricsCollector::instance().incrementDbQueries();
    return (rc == SQLITE_DONE);
}

std::vector<AlertRule> Database::getAlerts() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<AlertRule> list;
    if (!m_db) return list;

    const char* sql = "SELECT id, symbol, condition, target_value, message, is_active, is_triggered, created_at, triggered_at FROM alerts;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        AlertRule a;
        a.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        a.symbol = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        std::string condStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        a.condition = static_cast<AlertCondition>(std::stoi(condStr));
        a.targetValue = sqlite3_column_double(stmt, 3);
        a.message = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        a.isActive = (sqlite3_column_int(stmt, 5) != 0);
        a.isTriggered = (sqlite3_column_int(stmt, 6) != 0);
        a.createdAt = sqlite3_column_int64(stmt, 7);
        a.triggeredAt = sqlite3_column_int64(stmt, 8);
        list.push_back(a);
    }
    sqlite3_finalize(stmt);
    SystemMetricsCollector::instance().incrementDbQueries();
    return list;
}

bool Database::setSetting(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_db) return false;

    const char* sql = "INSERT OR REPLACE INTO settings (key, value) VALUES (?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, value.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    SystemMetricsCollector::instance().incrementDbQueries();
    return (rc == SQLITE_DONE);
}

std::string Database::getSetting(const std::string& key, const std::string& defaultValue) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_db) return defaultValue;

    const char* sql = "SELECT value FROM settings WHERE key = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return defaultValue;

    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);

    std::string result = defaultValue;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    }
    sqlite3_finalize(stmt);
    SystemMetricsCollector::instance().incrementDbQueries();
    return result;
}

} // namespace crypto
