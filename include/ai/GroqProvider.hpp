#pragma once

#include "ai/IAIProvider.hpp"
#include <string>
#include <vector>

namespace crypto {

class GroqProvider : public IAIProvider {
public:
    GroqProvider() = default;
    ~GroqProvider() override = default;

    AIResponse generate(const AIRequest& request) override;
    bool isAvailable() const override;
    std::string getProviderName() const override { return "Groq Cloud AI"; }
    std::vector<std::string> getAvailableModels() override;

private:
    std::string m_endpoint{"https://api.groq.com/openai/v1/chat/completions"};
};

} // namespace crypto
