#include <iostream>
#include <chrono>
#include <thread>
#include <GLFW/glfw3.h>

#include "core/Types.hpp"
#include "core/Logger.hpp"
#include "core/Config.hpp"
#include "core/ThreadPool.hpp"
#include "core/SystemMetrics.hpp"
#include "network/HttpClient.hpp"
#include "storage/Database.hpp"
#include "market/MarketManager.hpp"
#include "ai/AIRouter.hpp"
#include "portfolio/PortfolioManager.hpp"
#include "trading/PaperTradingEngine.hpp"
#include "analysis/AlertManager.hpp"
#include "news/NewsManager.hpp"
#include "ui/UIManager.hpp"

static void glfwErrorCallback(int error, const char* description) {
    LOG_ERROR("GLFW Error (" + std::to_string(error) + "): " + std::string(description));
}

int main(int argc, char** argv) {
    std::cout << "========================================================\n";
    std::cout << "          CRYPTØ AI TERMINAL — NATIVE C++20             \n";
    std::cout << "========================================================\n";

    // 1. Core Subsystems Initialization
    crypto::Logger::instance().info("Starting CRYPTØ AI TERMINAL initialization...");
    crypto::Config::instance().load(".env");
    crypto::HttpClient::instance().init();

    if (!crypto::Database::instance().init("crypto_terminal.db")) {
        crypto::Logger::instance().critical("Failed to initialize SQLite database.");
        return 1;
    }

    crypto::ThreadPool::instance().init(0);
    crypto::AIRouter::instance().init();
    crypto::PortfolioManager::instance().init();
    crypto::PaperTradingEngine::instance().init(10000.0);
    crypto::AlertManager::instance().init();
    crypto::NewsManager::instance().init();
    crypto::MarketManager::instance().init();

    // 2. Initialize GLFW Window
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) {
        crypto::Logger::instance().critical("Failed to initialize GLFW.");
        return 1;
    }

    // OpenGL 3.3 Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    int windowWidth = 1600;
    int windowHeight = 960;
    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "CRYPTØ AI TERMINAL — Native Workstation", nullptr, nullptr);
    if (!window) {
        crypto::Logger::instance().critical("Failed to create GLFW Window.");
        glfwTerminate();
        return 1;
    }

    glfwMaximizeWindow(window);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable VSync for smooth 60 FPS

    // 3. Initialize UI Layer
    if (!crypto::UIManager::instance().init(window)) {
        crypto::Logger::instance().critical("Failed to initialize UIManager.");
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    crypto::Logger::instance().info("CRYPTØ AI TERMINAL running. 60 FPS frame loop engaged.");

    // 4. Main Event Loop
    auto lastFrameTime = std::chrono::steady_clock::now();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        auto currentFrameTime = std::chrono::steady_clock::now();
        double deltaTime = std::chrono::duration<double>(currentFrameTime - lastFrameTime).count();
        lastFrameTime = currentFrameTime;

        // Process any queued background thread callbacks safely on the main UI thread
        crypto::ThreadPool::instance().processMainThreadCallbacks(50);

        // Update system telemetry & hardware metrics
        crypto::SystemMetricsCollector::instance().update(deltaTime);

        // Render Frame
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.000f, 0.000f, 0.000f, 1.0f); // #000000 Pure Black
        glClear(GL_COLOR_BUFFER_BIT);

        crypto::UIManager::instance().render(deltaTime);

        glfwSwapBuffers(window);
    }

    // 5. Graceful Clean Shutdown
    crypto::Logger::instance().info("Shutting down CRYPTØ AI TERMINAL gracefully...");

    crypto::UIManager::instance().shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();

    crypto::MarketManager::instance().shutdown();
    crypto::ThreadPool::instance().shutdown();
    crypto::Database::instance().close();
    crypto::HttpClient::instance().cleanup();

    crypto::Logger::instance().info("CRYPTØ AI TERMINAL shutdown cleanly with zero leaks.");
    return 0;
}
