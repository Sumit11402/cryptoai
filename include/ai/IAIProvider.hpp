#pragma once

#include "core/Types.hpp"
#include <string>
#include <vector>

namespace crypto {

class IAIProvider {
public:
    virtual ~IAIProvider() = default;

    virtual AIResponse generate(const AIRequest& request) = 0;
    virtual bool isAvailable() const = 0;
    virtual std::string getProviderName() const = 0;
    virtual std::vector<std::string> getAvailableModels() = 0;
};

} // namespace crypto
