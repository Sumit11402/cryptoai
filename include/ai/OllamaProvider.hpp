#pragma once

#include "ai/IAIProvider.hpp"
#include <string>
#include <vector>

namespace crypto {

class OllamaProvider : public IAIProvider {
public:
    OllamaProvider() = default;
    ~OllamaProvider() override = default;

    AIResponse generate(const AIRequest& request) override;
    bool isAvailable() const override;
    std::string getProviderName() const override { return "Ollama Local AI (Private)"; }
    std::vector<std::string> getAvailableModels() override;

private:
    std::string getBaseUrl() const;
};

} // namespace crypto
