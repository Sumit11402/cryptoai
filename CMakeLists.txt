cmake_minimum_required(VERSION 3.20)
project(CryptoAITerminal VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Optimization & Compiler Flags
add_compile_options(-O2 -Wall -Wextra)

include(FetchContent)

# 1. GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)

# 2. nlohmann_json
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)

# 3. ImGui (docking branch)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
)

# 4. ImPlot
FetchContent_Declare(
    implot
    GIT_REPOSITORY https://github.com/epezent/implot.git
    GIT_TAG master
)

# 5. IXWebSocket
set(USE_TLS ON CACHE BOOL "" FORCE)
set(USE_OPENSSL ON CACHE BOOL "" FORCE)
FetchContent_Declare(
    ixwebsocket
    GIT_REPOSITORY https://github.com/machinezone/IXWebSocket.git
    GIT_TAG v11.4.5
)

message(STATUS "Fetching external dependencies...")
FetchContent_MakeAvailable(glfw nlohmann_json imgui implot ixwebsocket)

find_package(OpenGL REQUIRED)
find_package(CURL REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# ImGui library target (lean without heavy demo suites)
set(IMGUI_DIR ${imgui_SOURCE_DIR})
set(IMPLOT_DIR ${implot_SOURCE_DIR})

add_library(imgui_lib STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    ${IMPLOT_DIR}/implot.cpp
    ${IMPLOT_DIR}/implot_items.cpp
)

target_include_directories(imgui_lib PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${IMPLOT_DIR}
)
target_link_libraries(imgui_lib PUBLIC glfw OpenGL::GL)

# Core Application Include directories
include_directories(
    include
    ${nlohmann_json_SOURCE_DIR}/include
    ${CURL_INCLUDE_DIRS}
    ${SQLite3_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
)

# Project Core Library
file(GLOB_RECURSE CORE_SOURCES 
    "src/core/*.cpp"
    "src/network/*.cpp"
    "src/ai/*.cpp"
    "src/market/*.cpp"
    "src/analysis/*.cpp"
    "src/portfolio/*.cpp"
    "src/trading/*.cpp"
    "src/news/*.cpp"
    "src/storage/*.cpp"
    "src/ui/*.cpp"
)

add_library(crypto_terminal_core STATIC ${CORE_SOURCES})
target_link_libraries(crypto_terminal_core PUBLIC
    imgui_lib
    ixwebsocket
    nlohmann_json::nlohmann_json
    ${CURL_LIBRARIES}
    SQLite3::SQLite3
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
    OpenGL::GL
)

# Main Executable
add_executable(crypto_ai_terminal src/main.cpp)
target_link_libraries(crypto_ai_terminal PRIVATE crypto_terminal_core)

# Tests Executable
add_executable(crypto_terminal_tests tests/test_main.cpp)
target_link_libraries(crypto_terminal_tests PRIVATE crypto_terminal_core)
enable_testing()
add_test(NAME AllTests COMMAND crypto_terminal_tests)

message(STATUS "CRYPTØ AI TERMINAL configured successfully.")
