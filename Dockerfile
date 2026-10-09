# ============================================================
# MONICA MART - Multi-Stage Dockerfile
# Backend: C++20 + Drogon Framework + SQLite3
# Frontend: Vanilla HTML5 / CSS3 / JavaScript
# ============================================================

# Stage 1: Build Environment
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

# Install build tools and Drogon dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    gcc \
    g++ \
    libsqlite3-dev \
    libjsoncpp-dev \
    uuid-dev \
    zlib1g-dev \
    libssl-dev \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Build Drogon Framework
WORKDIR /build

RUN git clone --depth 1 --branch v1.9.3 \
    https://github.com/drogonframework/drogon.git && \
    cd drogon && \
    git submodule update --init && \
    mkdir build && \
    cd build && \
    cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_EXAMPLES=OFF \
        -DBUILD_TESTING=OFF \
        .. && \
    cmake --build . --parallel 1 && \
    cmake --install . && \
    ldconfig

# Copy MonicaMart source code
WORKDIR /app
COPY . .

# Build MonicaMart C++20 application
RUN cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build --parallel 1

# Stage 2: Production Runtime
FROM ubuntu:22.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

# Install runtime libraries
RUN apt-get update && apt-get install -y --no-install-recommends \
    libsqlite3-0 \
    libjsoncpp25 \
    libuuid1 \
    zlib1g \
    libssl3 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy Drogon and Trantor shared libraries
COPY --from=builder /usr/local/lib/ /usr/local/lib/

RUN ldconfig

# Copy compiled application
COPY --from=builder /app/build/MonicaMart /app/MonicaMart

# Copy application assets
COPY --from=builder /app/database /app/database
COPY --from=builder /app/frontend /app/frontend
COPY --from=builder /app/backend/config.json /app/config.json

# Railway provides PORT dynamically
EXPOSE 8080

# Start MonicaMart
CMD ["/app/MonicaMart"]