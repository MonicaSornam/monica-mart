# ============================================================
# MONICA MART - Multi-Stage Dockerfile
# Backend: C++20 + Drogon Framework + SQLite3
# Frontend: Vanilla HTML5 / CSS3 / JavaScript
# Author: Monica Sornam (College Capstone Project)
# ============================================================

# Stage 1: Build Environment
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

# Install build tools and Drogon dependencies
RUN apt-get update && apt-get install -y \
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

# Clone and build Drogon Framework (v1.9.3)
WORKDIR /build
RUN git clone --depth 1 --branch v1.9.3 https://github.com/drogonframework/drogon.git && \
    cd drogon && \
    git submodule update --init && \
    mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=OFF -DBUILD_TESTING=OFF .. && \
    make -j$(nproc) && \
    make install && \
    ldconfig

# Copy MonicaMart Source Code
WORKDIR /app
COPY . .

# Build MonicaMart C++20 application
RUN mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    make -j$(nproc)

# Stage 2: Minimal Production Runtime
FROM ubuntu:22.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

# Install runtime shared libraries only
RUN apt-get update && apt-get install -y \
    libsqlite3-0 \
    libjsoncpp25 \
    libuuid1 \
    zlib1g \
    libssl3 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy Drogon shared libraries from builder
COPY --from=builder /usr/local/lib/libdrogon* /usr/local/lib/
COPY --from=builder /usr/local/lib/libtrantor* /usr/local/lib/
RUN ldconfig

# Copy compiled binary and application assets
COPY --from=builder /app/build/MonicaMart /app/MonicaMart
COPY --from=builder /app/database /app/database
COPY --from=builder /app/frontend /app/frontend
COPY --from=builder /app/backend/config.json /app/config.json

# Expose HTTP port
EXPOSE 8080

# Launch MonicaMart server
CMD ["/app/MonicaMart"]
