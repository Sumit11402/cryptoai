#include "network/WebSocketClient.hpp"
#include "core/Logger.hpp"
#include "core/ThreadPool.hpp"
#include <ixwebsocket/IXNetSystem.h>

namespace crypto {

WebSocketClient::WebSocketClient() {
    m_socket = std::make_unique<ix::WebSocket>();
    ix::initNetSystem();

    // Configure reconnect backoff
    m_socket->enableAutomaticReconnection();
    m_socket->setMinWaitBetweenReconnectionRetries(1);
    m_socket->setMaxWaitBetweenReconnectionRetries(10);
    m_socket->setPingInterval(30);

    m_socket->setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg) {
        this->handleSocketEvent(msg);
    });
}

WebSocketClient::~WebSocketClient() {
    disconnect();
    ix::uninitNetSystem();
}

void WebSocketClient::setUrl(const std::string& url) {
    m_url = url;
    if (m_socket) {
        m_socket->setUrl(m_url);
    }
}

void WebSocketClient::connect() {
    if (m_url.empty()) {
        LOG_WARN("WebSocketClient: Cannot connect with empty URL");
        return;
    }

    updateState(ConnectionState::CONNECTING);
    LOG_INFO("WebSocket connecting to: " + m_url);
    m_socket->setUrl(m_url);
    m_socket->start();
}

void WebSocketClient::disconnect() {
    if (m_socket) {
        m_socket->stop();
    }
    updateState(ConnectionState::DISCONNECTED);
}

void WebSocketClient::send(const std::string& message) {
    if (m_socket && isConnected()) {
        m_socket->send(message);
    }
}

void WebSocketClient::setOnMessage(std::function<void(const std::string&)> callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_onMessage = callback;
}

void WebSocketClient::setOnStateChanged(std::function<void(ConnectionState)> callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_onStateChanged = callback;
}

void WebSocketClient::updateState(ConnectionState newState) {
    m_state.store(newState);
    std::function<void(ConnectionState)> cb;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        cb = m_onStateChanged;
    }
    if (cb) {
        ThreadPool::instance().postToMainThread([cb, newState]() {
            cb(newState);
        });
    }
}

std::string WebSocketClient::getStateString() const {
    switch (m_state.load()) {
        case ConnectionState::DISCONNECTED: return "DISCONNECTED";
        case ConnectionState::CONNECTING:   return "CONNECTING";
        case ConnectionState::CONNECTED:    return "CONNECTED";
        case ConnectionState::RECONNECTING: return "RECONNECTING";
        case ConnectionState::FAILED:       return "FAILED";
    }
    return "UNKNOWN";
}

void WebSocketClient::handleSocketEvent(const ix::WebSocketMessagePtr& msg) {
    if (!msg) return;

    switch (msg->type) {
        case ix::WebSocketMessageType::Open: {
            LOG_INFO("WebSocket Connected successfully to " + m_url);
            updateState(ConnectionState::CONNECTED);
            break;
        }
        case ix::WebSocketMessageType::Close: {
            LOG_INFO("WebSocket Closed (code: " + std::to_string(msg->closeInfo.code) + ", reason: " + msg->closeInfo.reason + ")");
            updateState(ConnectionState::RECONNECTING);
            break;
        }
        case ix::WebSocketMessageType::Error: {
            LOG_WARN("WebSocket Error: " + msg->errorInfo.reason + " (HTTP " + std::to_string(msg->errorInfo.http_status) + ")");
            updateState(ConnectionState::RECONNECTING);
            break;
        }
        case ix::WebSocketMessageType::Message: {
            std::function<void(const std::string&)> cb;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                cb = m_onMessage;
            }
            if (cb) {
                std::string payload = msg->str;
                // Dispatch directly to avoid deep copying in IO loop
                cb(payload);
            }
            break;
        }
        case ix::WebSocketMessageType::Ping:
        case ix::WebSocketMessageType::Pong:
        case ix::WebSocketMessageType::Fragment:
            break;
    }
}

} // namespace crypto
