#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <future>
#include <functional>
#include <nlohmann/json.hpp>

namespace crypto {

struct HttpResponse {
    int statusCode{0};
    std::string body;
    std::string error;
    int64_t latencyMs{0};

    bool isSuccess() const { return statusCode >= 200 && statusCode < 300; }
    std::optional<nlohmann::json> asJson() const;
};

class HttpClient {
public:
    static HttpClient& instance();

    void init();
    void cleanup();

    // Synchronous requests (intended to be called from background worker threads)
    HttpResponse get(const std::string& url, 
                     const std::unordered_map<std::string, std::string>& headers = {},
                     int timeoutSec = 10);

    HttpResponse post(const std::string& url, 
                      const std::string& body,
                      const std::unordered_map<std::string, std::string>& headers = {},
                      int timeoutSec = 15);

    // Asynchronous requests (dispatches to ThreadPool and invokes callback)
    void getAsync(const std::string& url,
                  std::function<void(const HttpResponse&)> callback,
                  const std::unordered_map<std::string, std::string>& headers = {},
                  int timeoutSec = 10);

    void postAsync(const std::string& url,
                   const std::string& body,
                   std::function<void(const HttpResponse&)> callback,
                   const std::unordered_map<std::string, std::string>& headers = {},
                   int timeoutSec = 15);

private:
    HttpClient() = default;
    ~HttpClient() = default;
    HttpClient(const HttpClient&) = delete;
    HttpClient& operator=(const HttpClient&) = delete;

    HttpResponse executeRequest(const std::string& url, 
                                const std::string& method,
                                const std::string& body,
                                const std::unordered_map<std::string, std::string>& headers,
                                int timeoutSec);

    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp);
};

} // namespace crypto
