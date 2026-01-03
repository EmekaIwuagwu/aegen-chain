# Aegen L2 Node - Production Dockerfile
# Multi-stage build for optimal image size

FROM ubuntu:22.04 AS builder

# Prevent interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libssl-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /build

# Copy source code
COPY . .

# Create build directory and build
RUN mkdir -p build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    cmake --build . --config Release -j$(nproc)

# Runtime image
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    libssl3 \
    curl \
    && rm -rf /var/lib/apt/lists/* \
    && apt-get clean

WORKDIR /app

# Copy binary from builder
COPY --from=builder /build/build/aegen-node /app/aegen-node
COPY --from=builder /build/explorer /app/explorer

# Make executable
RUN chmod +x /app/aegen-node

# Create data directory
RUN mkdir -p /app/data /app/logs

# Expose ports
# 8545 - RPC Server
# 30303 - P2P Protocol
EXPOSE 8545
EXPOSE 30303

# Environment variables
ENV AEGEN_NODE_ID=node-1
ENV AEGEN_RPC_PORT=8545
ENV AEGEN_P2P_PORT=30303
ENV AEGEN_IS_VALIDATOR=true
ENV AEGEN_NETWORK_ID=aegen-l2
ENV AEGEN_L1_NETWORK=testnet04
ENV AEGEN_L1_CHAIN=0
ENV AEGEN_DATA_DIR=/app/data
ENV AEGEN_LOG_DIR=/app/logs

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD curl -sf http://localhost:${AEGEN_RPC_PORT} -H "Content-Type: application/json" -d '{"jsonrpc":"2.0","method":"getChainInfo","params":{},"id":1}' || exit 1

# Run node
ENTRYPOINT ["/app/aegen-node"]
