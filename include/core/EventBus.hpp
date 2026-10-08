#pragma once

#include <functional>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <typeindex>
#include <memory>

namespace crypto {

class EventBus {
public:
    static EventBus& instance() {
        static EventBus s_instance;
        return s_instance;
    }

    template<typename EventType>
    void subscribe(std::function<void(const EventType&)> handler) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto typeIndex = std::type_index(typeid(EventType));
        m_subscribers[typeIndex].push_back([handler](const void* eventPtr) {
            handler(*static_cast<const EventType*>(eventPtr));
        });
    }

    template<typename EventType>
    void publish(const EventType& event) {
        std::vector<std::function<void(const void*)>> handlers;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto typeIndex = std::type_index(typeid(EventType));
            auto it = m_subscribers.find(typeIndex);
            if (it != m_subscribers.end()) {
                handlers = it->second;
            }
        }
        for (const auto& handler : handlers) {
            if (handler) {
                handler(&event);
            }
        }
    }

private:
    EventBus() = default;
    ~EventBus() = default;
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;

    std::mutex m_mutex;
    std::unordered_map<std::type_index, std::vector<std::function<void(const void*)>>> m_subscribers;
};

// Common Event Types
struct TickerUpdatedEvent {
    std::string symbol;
    double price;
    double change24h;
    double volume24h;
};

struct CandleUpdatedEvent {
    std::string symbol;
    std::string timeframe;
    Candle candle;
};

struct AIResponseReadyEvent {
    std::string conversationId;
    AIResponse response;
};

struct AlertTriggeredEvent {
    AlertRule alert;
    double currentPrice;
    std::string message;
};

struct PaperTradeExecutedEvent {
    PaperOrder order;
};

} // namespace crypto
