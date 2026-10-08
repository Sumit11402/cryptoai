#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <atomic>
#include <mutex>
#include <ixwebsocket/IXWebSocket.h>

namespace crypto {

enum class ConnectionState {
    DISCONNECTED = 0,
    CONNECTING,
    CONNECTED,
    RECONNECTING,
    FAILED
};

class WebSocketClient {
public:
    WebSocketClient();
    ~WebSocketClient();

    void setUrl(const std::string& url);
    void connect();
    void disconnect();

    void send(const std::string& message);

    void setOnMessage(std::function<void(const std::string&)> callback);
    void setOnStateChanged(std::function<void(ConnectionState)> callback);

    ConnectionState getState() const { return m_state.load(); }
    std::string getStateString() const;
    bool isConnected() const { return m_state.load() == ConnectionState::CONNECTED; }

private:
    void handleSocketEvent(const ix::WebSocketMessagePtr& msg);
    void updateState(ConnectionState newState);

    std::string m_url;
    std::unique_ptr<ix::WebSocket> m_socket;
    std::atomic<ConnectionState> m_state{ConnectionState::DISCONNECTED};
    
    std::function<void(const std::string&)> m_onMessage;
    std::function<void(ConnectionState)> m_onStateChanged;
    mutable std::mutex m_mutex;
};

} // namespace crypto
