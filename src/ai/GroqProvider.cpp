#include "ai/GroqProvider.hpp"
#include "core/Config.hpp"
#include "core/Logger.hpp"
#include "network/HttpClient.hpp"
#include <nlohmann/json.hpp>
#include <chrono>

namespace crypto {

bool GroqProvider::isAvailable() const {
    std::string key = Config::instance().getGroqApiKey();
    return !key.empty();
}

std::vector<std::string> GroqProvider::getAvailableModels() {
    return {
        "llama-3.3-70b-versatile",
        "llama-3.1-8b-instant",
        "mixtral-8x7b-32768",
        "gemma2-9b-it"
    };
}

AIResponse GroqProvider::generate(const AIRequest& request) {
    AIResponse response;
    std::string apiKey = Config::instance().getGroqApiKey();

    if (apiKey.empty()) {
        response.success = false;
        response.errorMessage = "Groq API key not configured. Set GROQ_API_KEY in .env or Settings.";
        return response;
    }

    std::string model = request.model.empty() ? Config::instance().getGroqModel() : request.model;
    response.modelUsed = model;
    response.providerName = "Groq (" + model + ")";

    nlohmann::json payload;
    payload["model"] = model;
    payload["temperature"] = request.temperature;
    payload["max_tokens"] = request.maxTokens;

    nlohmann::json messages = nlohmann::json::array();

    // System prompt with structured context
    std::string systemMsg = request.systemPrompt;
    if (!request.structuredContext.empty()) {
        systemMsg += "\n\nCURRENT MARKET DATA CONTEXT (JSON):\n" + request.structuredContext.dump(2);
    }
    messages.push_back({{"role", "system"}, {"content", systemMsg}});

    // History
    for (const auto& msg : request.history) {
        messages.push_back({{"role", msg.role}, {"content", msg.content}});
    }

    // Current Prompt
    messages.push_back({{"role", "user"}, {"content", request.prompt}});
    payload["messages"] = messages;

    std::unordered_map<std::string, std::string> headers = {
        {"Authorization", "Bearer " + apiKey},
        {"Content-Type", "application/json"}
    };

    auto start = std::chrono::steady_clock::now();
    auto httpResp = HttpClient::instance().post(m_endpoint, payload.dump(), headers, 30);
    auto end = std::chrono::steady_clock::now();
    response.latencyMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    if (!httpResp.isSuccess()) {
        response.success = false;
        response.errorMessage = "Groq API Error (HTTP " + std::to_string(httpResp.statusCode) + "): " + httpResp.error;
        if (!httpResp.body.empty()) {
            try {
                auto errJson = nlohmann::json::parse(httpResp.body);
                if (errJson.contains("error") && errJson["error"].contains("message")) {
                    response.errorMessage = errJson["error"]["message"].get<std::string>();
                }
            } catch (...) {
                response.errorMessage += " - " + httpResp.body;
            }
        }
        LOG_WARN("Groq request failed: " + response.errorMessage);
        return response;
    }

    auto jsonOpt = httpResp.asJson();
    if (!jsonOpt || !jsonOpt->is_object()) {
        response.success = false;
        response.errorMessage = "Failed to parse Groq response JSON";
        return response;
    }

    const auto& j = *jsonOpt;
    try {
        if (j.contains("choices") && j["choices"].is_array() && !j["choices"].empty()) {
            response.content = j["choices"][0]["message"]["content"].get<std::string>();
            response.success = true;
        }
        if (j.contains("usage") && j["usage"].contains("total_tokens")) {
            response.tokensUsed = j["usage"]["total_tokens"].get<int>();
        }
    } catch (const std::exception& e) {
        response.success = false;
        response.errorMessage = "Exception parsing Groq response: " + std::string(e.what());
    }

    return response;
}

} // namespace crypto
