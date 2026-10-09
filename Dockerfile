FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

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

WORKDIR /build

RUN git clone --depth 1 --branch v1.9.3 \
    https://github.com/drogonframework/drogon.git && \
    cd drogon && \
    git submodule update --init && \
    mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DBUILD_EXAMPLES=OFF \
          -DBUILD_TESTING=OFF .. && \
    cmake --build . --parallel 1 && \
    cmake --install . && \
    ldconfig

WORKDIR /app
COPY . .

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build --parallel 1

FROM ubuntu:22.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    libsqlite3-0 \
    libjsoncpp25 \
    libuuid1 \
    zlib1g \
    libssl3 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /usr/local/lib/ /usr/local/lib/
RUN ldconfig

COPY --from=builder /app/build/MonicaMart /app/MonicaMart

# Application assets
COPY --from=builder /app/public /app/public
COPY --from=builder /app/frontend /app/frontend
COPY --from=builder /app/sql /app/sql
COPY --from=builder /app/database /app/database
COPY --from=builder /app/config/config.json /app/config.json

CMD ["/app/MonicaMart"]