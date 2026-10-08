#pragma once

#include "ai/IAIProvider.hpp"
#include "ai/GroqProvider.hpp"
#include "ai/OllamaProvider.hpp"
#include "core/Types.hpp"
#include <memory>
#include <mutex>
#include <functional>

namespace crypto {

class AIRouter {
public:
    static AIRouter& instance();

    void init();

    AIResponse generateSync(const AIRequest& request, AIProviderType providerOverride = AIProviderType::AUTO);
    
    void generateAsync(const AIRequest& request, 
                        std::function<void(const AIResponse&)> callback,
                        AIProviderType providerOverride = AIProviderType::AUTO);

    std::string getActiveProviderStatus() const;
    std::vector<std::string> getModelsForProvider(AIProviderType provider);

    GroqProvider* getGroqProvider() { return m_groqProvider.get(); }
    OllamaProvider* getOllamaProvider() { return m_ollamaProvider.get(); }

private:
    AIRouter();
    ~AIRouter() = default;

    AIResponse generateOfflineSynthesis(const AIRequest& request);

    std::unique_ptr<GroqProvider> m_groqProvider;
    std::unique_ptr<OllamaProvider> m_ollamaProvider;
    mutable std::mutex m_mutex;
};

} // namespace crypto
