#!/usr/bin/env bash
# ==============================================================================
# CRYPTØ AI TERMINAL - Zero-Install One-Liner Launcher
# https://github.com/Sumit11402/cryptoai
# ==============================================================================
set -e

REPO_OWNER="Sumit11402"
REPO_NAME="cryptoai"
GITHUB_REPO="https://github.com/${REPO_OWNER}/${REPO_NAME}"
RELEASE_API="https://api.github.com/repos/${REPO_OWNER}/${REPO_NAME}/releases/latest"

COLOR_RESET="\033[0m"
COLOR_BOLD="\033[1m"
COLOR_EMERALD="\033[38;2;73;184;154m"
COLOR_GRAY="\033[38;2;120;120;120m"
COLOR_RED="\033[38;2;220;80;80m"
COLOR_YELLOW="\033[38;2;240;180;60m"

echo -e "${COLOR_EMERALD}"
cat << 'EOF'
 ▄████▄   ██░ ██  ▄▄▄       ██▓ ███▄    █  ▄▄▄▄    ██▓     ▒█████   ▄████▄   ██ ▄█▀
▒██▀ ▀█  ▓██░ ██▒▒████▄    ▓██▒ ██ ▀█   █ ▓█████▄ ▓██▒    ▒██▒  ██▒▒██▀ ▀█   ██▄█▒ 
▒▓█    ▄ ▒██▀▀██░▒██  ▀█▄  ▒██▒▓██  ▀█ ██▒▒██▒ ▄██▒██░    ▒██░  ██▒▒▓█    ▄ ▓███▄░ 
▒▓▓▄ ▄██▒░▓█ ░██ ░██▄▄▄▄██ ░██░▓██▒  ▐▌██▒▒██░█▀  ▒██░    ▒██   ██░▒▓▓▄ ▄██▒▓██ █▄  
▒ ▓███▀ ░░▓█▒░██▓ ▓█   ▓██▒░██░▒██░   ▓██░░▓█  ▀█▓░██████▒░ ████▓▒░▒ ▓███▀ ░▒██▒ █▄ 
░ ░▒ ▒  ░ ▒ ░░▒░▒ ▒▒   ▓▒█░░▓  ░ ▒░   ▒ ▒ ░▒▓███▀▒░ ▒░▓  ░░ ▒░▒░▒░ ░ ░▒ ▒  ░▒ ▒▒ ▓▒ 
  ░  ▒    ▒ ░▒░ ░  ▒   ▒▒ ░ ▒ ░░ ░░   ░ ▒░▒░▒   ░ ░ ░ ▒  ░  ░ ▒ ▒░   ░  ▒   ░ ░▒ ▒░ 
░         ░  ░░ ░  ░   ▒    ▒ ░   ░   ░ ░  ░    ░   ░ ░   ░ ░ ░ ▒  ░        ░ ░░ ░  
░ ░       ░  ░  ░      ░  ░ ░           ░  ░          ░  ░    ░ ░  ░ ░       ░  ░    
░                                               ░                  ░                 
EOF
echo -e "${COLOR_RESET}${COLOR_BOLD}   Institutional Crypto AI Desktop Trading Terminal (100% Free & Zero-Install)${COLOR_RESET}\n"

# 1. Check if running inside already-built source directory
if [ -f "./build/crypto_ai_terminal" ]; then
    echo -e "${COLOR_EMERALD}● Found local compiled binary. Launching immediately...${COLOR_RESET}"
    exec ./build/crypto_ai_terminal "$@"
fi

# 2. Check if running inside repository without build
if [ -f "./CMakeLists.txt" ] && command -v cmake >/dev/null 2>&1; then
    echo -e "${COLOR_YELLOW}● Local source detected without binary. Building quickly...${COLOR_RESET}"
    mkdir -p build
    cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTS=OFF
    cmake --build build -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)
    echo -e "${COLOR_EMERALD}✓ Build complete! Starting terminal...${COLOR_RESET}"
    exec ./build/crypto_ai_terminal "$@"
fi

# 3. Running standalone via curl / bash - Detect OS & Architecture
OS="$(uname -s)"
ARCH="$(uname -m)"
CACHE_DIR="${HOME}/.cache/cryptoai"
mkdir -p "${CACHE_DIR}"

echo -e "${COLOR_GRAY}● Detected Platform: ${OS} (${ARCH})${COLOR_RESET}"

if [ "${OS}" = "Linux" ]; then
    ASSET_NAME="crypto-ai-terminal-linux-x86_64.tar.gz"
    BIN_NAME="crypto_ai_terminal"
elif [ "${OS}" = "Darwin" ]; then
    ASSET_NAME="crypto-ai-terminal-macos-universal.zip"
    BIN_NAME="crypto_ai_terminal"
else
    echo -e "${COLOR_RED}✗ Unsupported OS: ${OS}. Please build from source via CMake.${COLOR_RESET}"
    exit 1
fi

DEST_DIR="${CACHE_DIR}/bin"
mkdir -p "${DEST_DIR}"

RUN_BIN="${DEST_DIR}/${BIN_NAME}"

# 4. Check if cached binary is present and executable
if [ -x "${RUN_BIN}" ]; then
    echo -e "${COLOR_EMERALD}● Found cached executable at ${RUN_BIN}. Launching...${COLOR_RESET}"
    cd "${DEST_DIR}"
    exec "${RUN_BIN}" "$@"
fi

# 5. Fetch latest release from GitHub
echo -e "${COLOR_EMERALD}● Downloading latest prebuilt binary from GitHub...${COLOR_RESET}"
DOWNLOAD_URL="https://github.com/${REPO_OWNER}/${REPO_NAME}/releases/latest/download/${ASSET_NAME}"

TEMP_FILE="${CACHE_DIR}/${ASSET_NAME}"

if command -v curl >/dev/null 2>&1; then
    if ! curl -fsSL "${DOWNLOAD_URL}" -o "${TEMP_FILE}"; then
        # Fallback to direct tag release download if latest release tag redirect isn't populated yet
        DOWNLOAD_URL="https://github.com/${REPO_OWNER}/${REPO_NAME}/releases/download/v1.0.0/${ASSET_NAME}"
        curl -fsSL "${DOWNLOAD_URL}" -o "${TEMP_FILE}" 2>/dev/null || true
    fi
elif command -v wget >/dev/null 2>&1; then
    wget -q -O "${TEMP_FILE}" "${DOWNLOAD_URL}" || true
fi

# 6. If archive downloaded, extract and run
if [ -s "${TEMP_FILE}" ]; then
    echo -e "${COLOR_EMERALD}● Extracting package...${COLOR_RESET}"
    if [[ "${ASSET_NAME}" == *.tar.gz ]]; then
        tar -xzf "${TEMP_FILE}" -C "${CACHE_DIR}"
        # Copy contents to bin dir
        cp -rf "${CACHE_DIR}"/crypto-ai-terminal-linux/* "${DEST_DIR}/" 2>/dev/null || true
    elif [[ "${ASSET_NAME}" == *.zip ]]; then
        unzip -q -o "${TEMP_FILE}" -d "${CACHE_DIR}"
        cp -rf "${CACHE_DIR}"/crypto-ai-terminal-macos/* "${DEST_DIR}/" 2>/dev/null || true
    fi
    chmod +x "${RUN_BIN}" 2>/dev/null || true
    if [ "${OS}" = "Darwin" ]; then
        xattr -d com.apple.quarantine "${RUN_BIN}" 2>/dev/null || true
    fi
fi

# 7. Verify executable or fallback to automatic quick-clone & compile
if [ -x "${RUN_BIN}" ]; then
    echo -e "${COLOR_EMERALD}✓ Terminal ready! Launching...${COLOR_RESET}\n"
    cd "${DEST_DIR}"
    exec "${RUN_BIN}" "$@"
else
    echo -e "${COLOR_YELLOW}● Release package not yet published or direct download unavailable.${COLOR_YELLOW}"
    echo -e "${COLOR_EMERALD}● Performing automated fast build in ${CACHE_DIR}/src...${COLOR_RESET}"
    
    # Check dependencies
    if ! command -v git >/dev/null 2>&1 || ! command -v cmake >/dev/null 2>&1; then
        echo -e "${COLOR_RED}✗ Git and CMake are required to build. Please install build-essential/cmake or download from https://github.com/${REPO_OWNER}/${REPO_NAME}/releases${COLOR_RESET}"
        exit 1
    fi
    
    SRC_DIR="${CACHE_DIR}/src"
    if [ ! -d "${SRC_DIR}/.git" ]; then
        git clone --depth 1 "${GITHUB_REPO}.git" "${SRC_DIR}"
    else
        cd "${SRC_DIR}" && git pull --ff-only 2>/dev/null || true
    fi
    
    cd "${SRC_DIR}"
    mkdir -p build
    cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTS=OFF
    cmake --build build -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)
    
    cp build/crypto_ai_terminal "${RUN_BIN}"
    cp -r assets config .env.example "${DEST_DIR}/" 2>/dev/null || true
    chmod +x "${RUN_BIN}"
    
    echo -e "${COLOR_EMERALD}✓ Binary compiled and ready! Launching...${COLOR_RESET}\n"
    cd "${DEST_DIR}"
    exec "${RUN_BIN}" "$@"
fi
