# ==============================================================================
# CRYPTØ AI TERMINAL - Containerized Zero-Install Desktop Environment
# ==============================================================================
FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libssl-dev \
    libcurl4-openssl-dev \
    libx11-dev \
    libxrandr-dev \
    libxinerama-dev \
    libxcursor-dev \
    libxi-dev \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    libsqlite3-dev \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTS=OFF \
    && cmake --build build --config Release -j$(nproc)

# ------------------------------------------------------------------------------
# Runtime Image
# ------------------------------------------------------------------------------
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    libssl3 \
    libcurl4 \
    libx11-6 \
    libxrandr2 \
    libxinerama1 \
    libxcursor1 \
    libxi6 \
    libgl1-mesa-glx \
    libglu1-mesa \
    libsqlite3-0 \
    ca-certificates \
    fonts-noto \
    fonts-noto-cjk \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /app/build/crypto_ai_terminal /app/crypto_ai_terminal
COPY --from=builder /app/assets /app/assets
COPY --from=builder /app/config /app/config
COPY --from=builder /app/.env.example /app/.env.example

ENV DISPLAY=:0

ENTRYPOINT ["/app/crypto_ai_terminal"]
