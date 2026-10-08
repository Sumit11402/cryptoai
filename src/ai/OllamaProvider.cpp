#include "ai/OllamaProvider.hpp"
#include "core/Config.hpp"
#include "core/Logger.hpp"
#include "network/HttpClient.hpp"
#include <nlohmann/json.hpp>
#include <chrono>

namespace crypto {

std::string OllamaProvider::getBaseUrl() const {
    return Config::instance().getOllamaBaseUrl();
}

bool OllamaProvider::isAvailable() const {
    // Quick ping to /api/tags
    std::string url = getBaseUrl() + "/api/tags";
    auto resp = HttpClient::instance().get(url, {}, 2);
    return resp.isSuccess();
}

std::vector<std::string> OllamaProvider::getAvailableModels() {
    std::string url = getBaseUrl() + "/api/tags";
    auto resp = HttpClient::instance().get(url, {}, 3);
    std::vector<std::string> models;

    if (resp.isSuccess()) {
        auto jsonOpt = resp.asJson();
        if (jsonOpt && jsonOpt->contains("models") && (*jsonOpt)["models"].is_array()) {
            for (const auto& m : (*jsonOpt)["models"]) {
                if (m.contains("name")) {
                    models.push_back(m["name"].get<std::string>());
                }
            }
        }
    }

    if (models.empty()) {
        models = {"llama3.2", "llama3.1", "mistral", "deepseek-r1", "qwen2.5"};
    }
    return models;
}

AIResponse OllamaProvider::generate(const AIRequest& request) {
    AIResponse response;
    std::string model = request.model.empty() ? Config::instance().getOllamaModel() : request.model;
    response.modelUsed = model;
    response.providerName = "Ollama (" + model + ")";

    std::string url = getBaseUrl() + "/api/chat";

    nlohmann::json payload;
    payload["model"] = model;
    payload["stream"] = false;

    nlohmann::json options;
    options["temperature"] = request.temperature;
    options["num_predict"] = request.maxTokens;
    payload["options"] = options;

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
        {"Content-Type", "application/json"}
    };

    auto start = std::chrono::steady_clock::now();
    auto httpResp = HttpClient::instance().post(url, payload.dump(), headers, 60);
    auto end = std::chrono::steady_clock::now();
    response.latencyMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    if (!httpResp.isSuccess()) {
        response.success = false;
        response.errorMessage = "Ollama API Error (HTTP " + std::to_string(httpResp.statusCode) + 
                                "). Ensure Ollama is running at " + getBaseUrl();
        LOG_WARN(response.errorMessage);
        return response;
    }

    auto jsonOpt = httpResp.asJson();
    if (!jsonOpt || !jsonOpt->is_object()) {
        response.success = false;
        response.errorMessage = "Failed to parse Ollama JSON response";
        return response;
    }

    const auto& j = *jsonOpt;
    try {
        if (j.contains("message") && j["message"].contains("content")) {
            response.content = j["message"]["content"].get<std::string>();
            response.success = true;
        }
        if (j.contains("eval_count")) {
            response.tokensUsed = j["eval_count"].get<int>();
        }
    } catch (const std::exception& e) {
        response.success = false;
        response.errorMessage = "Exception parsing Ollama message: " + std::string(e.what());
    }

    return response;
}

} // namespace crypto
