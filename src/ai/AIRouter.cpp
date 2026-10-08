#include "ai/AIRouter.hpp"
#include "core/Config.hpp"
#include "core/Logger.hpp"
#include "core/ThreadPool.hpp"
#include "core/SystemMetrics.hpp"
#include <sstream>

namespace crypto {

AIRouter& AIRouter::instance() {
    static AIRouter s_instance;
    return s_instance;
}

AIRouter::AIRouter() {
    m_groqProvider = std::make_unique<GroqProvider>();
    m_ollamaProvider = std::make_unique<OllamaProvider>();
}

void AIRouter::init() {
    LOG_INFO("AIRouter initialized. Active strategy: AUTO (Groq -> Ollama -> Offline fallback).");
}

std::string AIRouter::getActiveProviderStatus() const {
    auto preferred = Config::instance().getAIProvider();
    if (preferred == AIProviderType::GROQ) {
        return m_groqProvider->isAvailable() ? "GROQ" : "GROQ (NO KEY)";
    }
    if (preferred == AIProviderType::OLLAMA) {
        return m_ollamaProvider->isAvailable() ? "OLLAMA" : "OLLAMA (DISCONNECTED)";
    }

    // AUTO
    if (m_groqProvider->isAvailable()) return "GROQ";
    if (m_ollamaProvider->isAvailable()) return "OLLAMA";
    return "AI OFFLINE";
}

std::vector<std::string> AIRouter::getModelsForProvider(AIProviderType provider) {
    if (provider == AIProviderType::GROQ) {
        return m_groqProvider->getAvailableModels();
    }
    if (provider == AIProviderType::OLLAMA) {
        return m_ollamaProvider->getAvailableModels();
    }
    // AUTO
    if (m_groqProvider->isAvailable()) return m_groqProvider->getAvailableModels();
    return m_ollamaProvider->getAvailableModels();
}

AIResponse AIRouter::generateSync(const AIRequest& request, AIProviderType providerOverride) {
    AIProviderType strategy = (providerOverride != AIProviderType::AUTO) ? providerOverride : Config::instance().getAIProvider();

    AIResponse resp;

    // 1. Explicit Groq selection
    if (strategy == AIProviderType::GROQ) {
        if (m_groqProvider->isAvailable()) {
            resp = m_groqProvider->generate(request);
            if (resp.success) {
                SystemMetricsCollector::instance().setAILatency(resp.latencyMs);
                SystemMetricsCollector::instance().setActiveAI("GROQ", resp.modelUsed);
                return resp;
            }
        } else {
            resp.success = false;
            resp.errorMessage = "Groq selected but GROQ_API_KEY is not configured.";
            return resp;
        }
    }

    // 2. Explicit Ollama selection
    if (strategy == AIProviderType::OLLAMA) {
        resp = m_ollamaProvider->generate(request);
        if (resp.success) {
            SystemMetricsCollector::instance().setAILatency(resp.latencyMs);
            SystemMetricsCollector::instance().setActiveAI("OLLAMA", resp.modelUsed);
            return resp;
        }
        return resp;
    }

    // 3. Explicit OFFLINE selection
    if (strategy == AIProviderType::OFFLINE) {
        resp = generateOfflineSynthesis(request);
        SystemMetricsCollector::instance().setAILatency(resp.latencyMs);
        SystemMetricsCollector::instance().setActiveAI("OFFLINE_SYNTHESIS", "native-quant-rules");
        return resp;
    }

    // 3. AUTO Mode: Groq -> Ollama -> Offline Fallback
    if (m_groqProvider->isAvailable()) {
        LOG_DEBUG("AIRouter AUTO: Attempting Groq Cloud AI...");
        resp = m_groqProvider->generate(request);
        if (resp.success) {
            SystemMetricsCollector::instance().setAILatency(resp.latencyMs);
            SystemMetricsCollector::instance().setActiveAI("GROQ", resp.modelUsed);
            return resp;
        }
        LOG_WARN("Groq failed (" + resp.errorMessage + "). Falling back to Ollama...");
    }

    if (m_ollamaProvider->isAvailable()) {
        LOG_DEBUG("AIRouter AUTO: Attempting Ollama Local AI...");
        resp = m_ollamaProvider->generate(request);
        if (resp.success) {
            SystemMetricsCollector::instance().setAILatency(resp.latencyMs);
            SystemMetricsCollector::instance().setActiveAI("OLLAMA", resp.modelUsed);
            return resp;
        }
        LOG_WARN("Ollama failed (" + resp.errorMessage + ").");
    }

    // 4. Offline Synthesis Fallback
    LOG_INFO("AIRouter: No external AI reachable, providing local analytical synthesis.");
    resp = generateOfflineSynthesis(request);
    SystemMetricsCollector::instance().setAILatency(resp.latencyMs);
    SystemMetricsCollector::instance().setActiveAI("OFFLINE_SYNTHESIS", "native-quant-rules");
    return resp;
}

void AIRouter::generateAsync(const AIRequest& request, 
                             std::function<void(const AIResponse&)> callback,
                             AIProviderType providerOverride) {
    ThreadPool::instance().enqueue([this, request, callback, providerOverride]() {
        AIResponse resp = this->generateSync(request, providerOverride);
        if (callback) {
            ThreadPool::instance().postToMainThread([callback, resp]() {
                callback(resp);
            });
        }
    });
}

AIResponse AIRouter::generateOfflineSynthesis(const AIRequest& request) {
    AIResponse resp;
    resp.success = true;
    resp.providerName = "CRYPTØ Native Offline Engine";
    resp.modelUsed = "quant-rules-v1.0";
    resp.latencyMs = 5;

    std::stringstream ss;
    ss << "### ⚡ CRYPTØ AI Research Analysis (Offline Native Synthesis)\n\n";

    if (!request.structuredContext.empty()) {
        const auto& ctx = request.structuredContext;
        std::string sym = ctx.value("symbol", "ASSET");
        double price = ctx.value("price", 0.0);
        double change = ctx.value("change24h", 0.0);
        double rsi = ctx.value("rsi", 50.0);
        double ema20 = ctx.value("ema20", 0.0);
        double ema50 = ctx.value("ema50", 0.0);
        std::string trend = ctx.value("trend", "NEUTRAL");

        ss << "#### 1. Observed Market Data\n";
        ss << "- **Asset:** " << sym << "\n";
        ss << "- **Current Spot Price:** $" << price << " (" << (change >= 0 ? "+" : "") << change << "% 24h)\n";
        ss << "- **Relative Strength Index (RSI-14):** " << rsi << "\n";
        ss << "- **EMA Structure (20/50):** EMA20 = $" << ema20 << " | EMA50 = $" << ema50 << "\n";
        ss << "- **Calculated Trend Regime:** **" << trend << "**\n\n";

        ss << "#### 2. Technical Interpretation\n";
        if (rsi < 30.0) {
            ss << "- **Momentum:** RSI indicates an **oversold condition** (< 30). Mean-reversion buyers often inspect this zone, though strong downtrends can sustain oversold levels.\n";
        } else if (rsi > 70.0) {
            ss << "- **Momentum:** RSI indicates an **overbought condition** (> 70). Upward momentum is elevated; consolidation or pullback risks increase.\n";
        } else {
            ss << "- **Momentum:** RSI is in a **neutral range** (" << rsi << "), indicating balanced buying and selling pressure.\n";
        }

        if (price > ema20 && ema20 > ema50) {
            ss << "- **Moving Averages:** Price is trading above both EMA20 and EMA50, showing constructive short-to-medium term moving average alignment.\n";
        } else if (price < ema20 && ema20 < ema50) {
            ss << "- **Moving Averages:** Price is compressed below descending EMAs, showing continued resistance on rallies.\n";
        }

        ss << "\n#### 3. Potential Scenarios\n";
        ss << "- **Bullish Scenario:** Sustained volume above $" << (price * 1.02) << " could target the next resistance level near $" << (price * 1.05) << ".\n";
        ss << "- **Bearish Scenario:** Inability to hold support at $" << (price * 0.98) << " may invite a test of deeper demand around $" << (price * 0.95) << ".\n\n";

        ss << "#### 4. Risk & Uncertainty Disclosure\n";
        ss << "- *Notice:* Crypto markets exhibit extreme volatility and order book slippage. Technical indicators reflect past price action and do not guarantee future trajectory. Exercise strict position sizing and stop-loss risk management.\n";
    } else {
        ss << "You asked: *" << request.prompt << "*\n\n";
        ss << "Groq API key is not configured and Ollama is currently offline. You can configure `GROQ_API_KEY` in Settings or start Ollama locally at `http://127.0.0.1:11434` for full unrestricted generative AI reasoning.\n";
    }

    resp.content = ss.str();
    return resp;
}

} // namespace crypto
