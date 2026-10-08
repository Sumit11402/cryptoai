# CRYPTØ AI TERMINAL

> **Production-Grade Native C++20 Crypto Research, Technical Analysis & AI Workstation**  
> *Engineered for high performance, zero mandatory API cost (₹0 budget), real-time WebSocket feeds, native quantitative analysis, and non-blocking 60 FPS Dear ImGui interface.*

---

## 🚀 Key Highlights & Philosophy

* **₹0 Mandatory API Budget**: Powered by **Groq Cloud AI** (free tier OpenAI-compatible endpoint) and **Ollama Local AI** (100% private, zero API key needed), with automatic failover to a **Native Offline Quantitative Analysis Engine**.
* **Public Market Data**: Live Binance REST API & Multi-Stream WebSockets (`wss://stream.binance.com:9443`). No exchange API keys or paid market data subscriptions required.
* **Anti-Gravity Non-Blocking Architecture**: 60 FPS UI thread. Zero blocking calls—HTTP queries, WebSocket handshakes, AI generation, database queries, and indicator evaluations run asynchronously on a dedicated worker `ThreadPool` with thread-safe UI task dispatching.
* **High-Fidelity Candlestick Workstation**: Native GPU-accelerated charting with ImPlot, multi-timeframe candle parsing (1m, 5m, 15m, 30m, 1h, 4h, 1D, 1W), crosshair tracking, and live price overlays.
* **Native C++ Quantitative Analysis**: Local calculation of Simple Moving Average (SMA), Exponential Moving Average (EMA), Relative Strength Index (Wilder's RSI), MACD (12, 26, 9), Bollinger Bands (20, 2.0σ), and Average True Range (ATR).
* **Paper Trading Simulator**: Virtual execution engine initialized with **$10,000 virtual USD**, real-time position tracking, commission modeling, and historical audit ledger.
* **Algorithmic Strategy Backtesting**: Historical simulation framework supporting SMA/EMA crossovers, RSI extremes, MACD signals, and Bollinger breakouts with equity curve plotting, win rate, max drawdown, and profit factor telemetry.
* **Production Security & Safety**: Zero hardcoded secrets, regex credential masking across logs and UI, persistent SQLite storage, and safety safeguards (autonomous real-money execution is permanently disabled by architecture).

---

## ⚡ Instant Run (Zero Installation Required)

You can launch the full institutional desktop terminal with **one single command** without cloning or installing dependencies manually:

```bash
curl -fsSL https://raw.githubusercontent.com/Sumit11402/cryptoai/main/run.sh | bash
```

*Or if you have Docker:*
```bash
docker run -it --rm --net=host -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix ghcr.io/sumit11402/cryptoai
```

---

## 🖥️ Application Architecture & Layout

```
┌──────────────────────────────────────────────────────────────────────────────────────────┐
│ CRYPTØ AI TERMINAL                    BTC/USDT  $64,250.00 (+3.45%)          ● LIVE STREAM │
├─────────────────┬────────────────────────────────────────────────────────────────────────┤
│                 │                                                                        │
│ 1. Dashboard    │                       MAIN WORKSPACE VIEW                              │
│ 2. Markets      │                                                                        │
│ 3. Chart        │   ┌──────────────────────────────────────────────────────────────┐     │
│ 4. AI Research  │   │  Candlestick Chart + Overlays (SMA 20/50, EMA 20, BB 20)     │     │
│ 5. Portfolio    │   ├──────────────────────────────────────────────────────────────┤     │
│ 6. Watchlist    │   │  RSI Subplot (14) [Overbought: 70 | Oversold: 30]            │     │
│ 7. Screener     │   ├──────────────────────────────────────────────────────────────┤     │
│ 8. Paper Trade  │   │  MACD Subplot (12, 26, 9) [Signal + Histogram]               │     │
│ 9. Alerts       │   └──────────────────────────────────────────────────────────────┘     │
│ 10. News        │                                                                        │
│ 11. Backtest    │                                                                        │
│ 12. Sys Monitor │                                                                        │
│ 13. Settings    │                                                                        │
│                 │                                                                        │
├─────────────────┴────────────────────────────────────────────────────────────────────────┤
│ ● MARKET LIVE   ● WS CONNECTED   AI: GROQ (llama-3.3-70b)   FPS: 60   CPU: 8%   RAM: 64MB│
└──────────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 📦 Project Structure

```
crypto-ai-terminal/
├── CMakeLists.txt              # Modern CMake build definition (FetchContent for dependencies)
├── README.md                   # Complete architectural & operational guide
├── .env.example                # Configuration template
├── .gitignore                  # Git ignore rules (secrets, build artifacts, db files)
├── assets/                     # UI fonts and graphical resources
├── config/                     # Default application configuration
├── include/
│   ├── ai/                     # AI provider abstraction, Groq, Ollama, AIRouter, Context builder
│   ├── analysis/               # SMA, EMA, RSI, MACD, BB, ATR, Alerts, Screener, Backtesting
│   ├── core/                   # ThreadPool, EventBus, Config, Logger, SystemMetrics, Types
│   ├── market/                 # IMarketDataProvider, Binance REST/WS, DemoMarketProvider, MarketManager
│   ├── network/                # HttpClient (libcurl), WebSocketClient (IXWebSocket)
│   ├── news/                   # NewsManager and sentiment aggregator
│   ├── portfolio/              # PortfolioManager, PnL engine, SQLite persistence
│   ├── storage/                # Database manager (SQLite3 prepared statements)
│   ├── trading/                # ITradingProvider, PaperTradingEngine
│   └── ui/                     # Dear ImGui views, UIManager, Theme, ImPlot custom renderers
├── src/
│   ├── main.cpp                # Clean application entrypoint & lifecycle orchestrator
│   ├── ai/                     # GroqProvider, OllamaProvider, AIRouter, AIContextBuilder, AICommandEngine
│   ├── analysis/               # TechnicalAnalysisEngine, AlertManager, MarketScreener, BacktestingEngine
│   ├── core/                   # ThreadPool, Config, Logger, SystemMetrics
│   ├── market/                 # BinanceProvider, DemoMarketProvider, MarketManager
│   ├── network/                # HttpClient, WebSocketClient
│   ├── news/                   # NewsManager
│   ├── portfolio/              # PortfolioManager
│   ├── storage/                # Database
│   ├── trading/                # PaperTradingEngine
│   └── ui/                     # UIManager, Views
└── tests/
    └── test_main.cpp           # Complete unit test suite (TA indicators, Paper trading, AI routing)
```

---

## 🛠️ Prerequisites & Dependencies

### System Packages (Linux / Ubuntu / Debian)
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake git libcurl4-openssl-dev libsqlite3-dev \
                        libgl1-mesa-dev libglu1-mesa-dev xorg-dev libxrandr-dev \
                        libxinerama-dev libxcursor-dev libxi-dev libssl-dev
```

### System Packages (Fedora / RHEL)
```bash
sudo dnf install -y gcc-c++ cmake git libcurl-devel sqlite-devel \
                    mesa-libGL-devel libXrandr-devel libXinerama-devel \
                    libXcursor-devel libXi-devel openssl-devel
```

### System Packages (Arch Linux)
```bash
sudo pacman -S base-devel cmake git curl sqlite mesa libxrandr libxinerama libxcursor libxi openssl
```

### System Packages (macOS via Homebrew)
```bash
brew install cmake curl sqlite openssl
```

> **Note**: Dear ImGui, ImPlot, GLFW 3.4, IXWebSocket, and nlohmann/json are automatically fetched and compiled at configure time via modern `CMake FetchContent`.

---

## ⚙️ Compilation & Build

```bash
# 1. Clone or navigate to the repository
cd /path/to/crypto-ai-terminal

# 2. Generate build system
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 3. Compile the application and test suite in parallel
cmake --build build -j$(nproc)
```

---

## 🧪 Running the Unit Test Suite

The terminal includes a comprehensive automated test suite validating:
- Technical indicator mathematics against reference datasets
- Paper trading position sizing, balance mutation, and realized PnL calculations
- Algorithmic backtesting trade executions and drawdown limits
- AI provider fallback cascade and structured context serialization

```bash
./build/crypto_terminal_tests
```

**Expected Output**:
```
====================================================
     CRYPTØ AI TERMINAL — UNIT TEST SUITE           
====================================================
[TEST] Running Technical Analysis Tests...
  ✓ SMA-20 calculated successfully.
  ✓ EMA-20 calculated successfully.
  ✓ RSI-14 calculated successfully.
  ✓ MACD calculated successfully.
  ✓ Bollinger Bands calculated.
  ✓ ATR-14 calculated.
[TEST] Running Portfolio & Paper Trading Tests...
  ✓ Paper Market BUY executed. New Balance: $6,996.40
  ✓ Paper Market SELL executed. Realized PnL: +$245.50
[TEST] Running Backtesting Simulation Tests...
  ✓ Backtest executed. Trades: 1 Final Capital: $11,643.40
[TEST] Running AI Context & Routing Tests...
  ✓ AI Market Context JSON constructed properly.
  ✓ AI Router generated valid synthesis response.

>>> ALL UNIT TESTS PASSED SUCCESSFULLY! <<<
```

---

## 🚀 Running CRYPTØ AI TERMINAL

```bash
./build/crypto_ai_terminal
```

---

## 🔑 AI Configuration Guide (₹0 Zero-Cost Setup)

The terminal requires **$0.00 / ₹0.00** mandatory spend. Configure your environment using `.env`:

```bash
cp .env.example .env
```

### Option A: Cloud AI via Groq (Free Tier)
1. Register for a free API key at [console.groq.com/keys](https://console.groq.com/keys).
2. Set your key in `.env`:
   ```ini
   GROQ_API_KEY=gsk_your_groq_api_key_here
   ```
3. Supported models: `llama-3.3-70b-versatile`, `mixtral-8x7b-32768`, `deepseek-r1-distill-llama-70b`.

### Option B: Local Private AI via Ollama (Zero Key Required)
1. Install Ollama from [ollama.com](https://ollama.com).
2. Pull and start your preferred model:
   ```bash
   ollama pull llama3
   # or
   ollama pull deepseek-r1:8b
   ```
3. Set in `.env`:
   ```ini
   OLLAMA_BASE_URL=http://127.0.0.1:11434
   ```

### Option C: Intelligent Fallback & Offline Synthesis
If neither cloud nor local endpoints respond, the built-in **AIRouter** automatically generates a deterministic quantitative market synthesis based on observed technical indicators (RSI momentum, MACD cross, Bollinger volatility) without crashing or blocking.

---

## 📊 Workstation Views & Capabilities

| View | Description |
| :--- | :--- |
| **1. Dashboard** | Real-time ticker cards (BTC, ETH, SOL, BNB, XRP, DOGE, ADA), market statistics, live terminal status. |
| **2. Markets** | Multi-asset sorting, 24h volume/change filter, top gainers/losers, click-to-chart navigation. |
| **3. Advanced Chart** | High-performance candlestick renderer with multi-timeframe klines (1m - 1W) and selectable overlays. |
| **4. AI Research** | Quantitative copilot chat with natural language command parsing (`/analyze`, `/rsi`, `/portfolio`, `/backtest`). |
| **5. Portfolio** | Holdings ledger, allocation percentage breakdown, unrealized/realized PnL calculation. |
| **6. Watchlist** | Customized symbol monitoring table with live price deltas and quick action triggers. |
| **7. Screener** | Technical condition scanner (RSI oversold/overbought, Bollinger breakout, 24h momentum). |
| **8. Paper Trading** | $10,000 starting virtual simulator with market Buy/Sell, position sizing, and transaction logs. |
| **9. Alerts** | Local asynchronous rule triggers (Price threshold, RSI levels, Moving average crosses). |
| **10. News** | Categorized crypto news headlines with sentiment tagging (Bullish / Bearish / Neutral). |
| **11. Backtest** | Strategy evaluation engine with equity curve rendering, win rate, and risk-adjusted metrics. |
| **12. Sys Monitor** | Telemetry dashboard showing CPU %, RAM usage, UI FPS, WebSocket latency, and active threads. |
| **13. Settings** | AI provider preference, active model selection, theme tuning, and database status. |

---

## 🔒 Security & Safe Quant Principles

1. **No Real-Money Orders**: All trading features are strictly sandboxed in the `PaperTradingEngine`. The live trading interface (`ITradingProvider`) is permanently disabled by default.
2. **Credential Sanitization**: The `Logger` and UI mask API keys and secrets matching sensitive entropy patterns.
3. **Prepared Statements**: SQLite interactions use strictly parameterized queries to prevent injection vulnerabilities.
4. **Resilient Offline Mode**: If network connectivity is lost, the terminal seamlessly transitions to offline mode, rendering cached candlesticks, local portfolio data, and local indicators without crashing.

---

## 📜 License

Distributed under the MIT License. See `LICENSE` for more information.
