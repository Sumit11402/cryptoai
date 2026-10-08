#include "network/HttpClient.hpp"
#include "core/Logger.hpp"
#include "core/ThreadPool.hpp"
#include <curl/curl.h>
#include <chrono>

namespace crypto {

std::optional<nlohmann::json> HttpResponse::asJson() const {
    if (body.empty()) return std::nullopt;
    try {
        return nlohmann::json::parse(body);
    } catch (const std::exception& e) {
        LOG_DEBUG("JSON parse error: " + std::string(e.what()));
        return std::nullopt;
    }
}

HttpClient& HttpClient::instance() {
    static HttpClient s_instance;
    return s_instance;
}

void HttpClient::init() {
    curl_global_init(CURL_GLOBAL_ALL);
    LOG_INFO("HttpClient (libcurl " + std::string(curl_version()) + ") initialized.");
}

void HttpClient::cleanup() {
    curl_global_cleanup();
    LOG_INFO("HttpClient cleaned up.");
}

size_t HttpClient::writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    std::string* response = static_cast<std::string*>(userp);
    response->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

HttpResponse HttpClient::executeRequest(const std::string& url, 
                                        const std::string& method,
                                        const std::string& body,
                                        const std::unordered_map<std::string, std::string>& headers,
                                        int timeoutSec) {
    HttpResponse response;
    CURL* curl = curl_easy_init();
    if (!curl) {
        response.error = "Failed to initialize CURL handle";
        return response;
    }

    struct curl_slist* headerList = nullptr;
    for (const auto& [key, val] : headers) {
        std::string headerStr = key + ": " + val;
        headerList = curl_slist_append(headerList, headerStr.c_str());
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, HttpClient::writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeoutSec);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "CryptoAITerminal/1.0.0");

    if (headerList) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);
    }

    if (method == "POST") {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, body.length());
    }

    auto start = std::chrono::steady_clock::now();
    CURLcode res = curl_easy_perform(curl);
    auto end = std::chrono::steady_clock::now();
    response.latencyMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    if (res != CURLE_OK) {
        response.error = curl_easy_strerror(res);
        LOG_WARN("HTTP " + method + " to " + url + " failed: " + response.error);
    } else {
        long httpCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        response.statusCode = static_cast<int>(httpCode);
    }

    if (headerList) {
        curl_slist_free_all(headerList);
    }
    curl_easy_cleanup(curl);

    return response;
}

HttpResponse HttpClient::get(const std::string& url, 
                             const std::unordered_map<std::string, std::string>& headers,
                             int timeoutSec) {
    return executeRequest(url, "GET", "", headers, timeoutSec);
}

HttpResponse HttpClient::post(const std::string& url, 
                              const std::string& body,
                              const std::unordered_map<std::string, std::string>& headers,
                              int timeoutSec) {
    return executeRequest(url, "POST", body, headers, timeoutSec);
}

void HttpClient::getAsync(const std::string& url,
                          std::function<void(const HttpResponse&)> callback,
                          const std::unordered_map<std::string, std::string>& headers,
                          int timeoutSec) {
    ThreadPool::instance().enqueue([this, url, callback, headers, timeoutSec]() {
        HttpResponse res = this->get(url, headers, timeoutSec);
        if (callback) {
            ThreadPool::instance().postToMainThread([callback, res]() {
                callback(res);
            });
        }
    });
}

void HttpClient::postAsync(const std::string& url,
                           const std::string& body,
                           std::function<void(const HttpResponse&)> callback,
                           const std::unordered_map<std::string, std::string>& headers,
                           int timeoutSec) {
    ThreadPool::instance().enqueue([this, url, body, callback, headers, timeoutSec]() {
        HttpResponse res = this->post(url, body, headers, timeoutSec);
        if (callback) {
            ThreadPool::instance().postToMainThread([callback, res]() {
                callback(res);
            });
        }
    });
}

} // namespace crypto
